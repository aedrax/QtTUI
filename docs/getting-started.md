# Getting started

This page tells how to build and install QtTUI, how to use it in a CMake
project, and how to write a first program. It also documents
`QTuiApplication`, the umbrella header and `QtTUIGlobal.h`.

## Build and install

You must have CMake 3.16 or later, Qt 6 Core and a C++17 compiler. QtTUI
does not use the Qt GUI module.

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build
sudo cmake --install build
```

The tests use Qt Test and RapidCheck. If RapidCheck is not installed, CMake
downloads it. The demos build to `build/demos/demo_hello`, `demo_todo`,
`demo_table` and `demo_gallery`.

Two CMake options control what builds. Both are `ON` only when QtTUI is the
top-level project.

| Option | Description |
| --- | --- |
| `QTTUI_BUILD_TESTS` | Builds the tests in `tests/`. |
| `QTTUI_BUILD_DEMOS` | Builds the demos in `demos/`. |

QtTUI is a static library only. Do not set `BUILD_SHARED_LIBS` for it: the
public headers have no export macro. QtTUI is tested on Linux only. The
Windows backend is not compiled or tested.

The install step puts the library in `lib/`, the headers in
`include/QtTUI/`, and the CMake package files in `lib/cmake/QtTUI/`.

## Use QtTUI in a CMake project

Link the installed package:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(QtTUI REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE QtTUI::QtTUI)
```

`find_package(QtTUI)` also finds Qt 6 Core. It asks for the Qt version that
QtTUI was built with, so CMake stops with an error if it finds an older
Qt. Use the same Qt version in your project: the static library does not
link with a different Qt.

You can also add the source tree. The QtTUI tests and demos then do not
build.

```cmake
add_subdirectory(QtTUI)
target_link_libraries(my_app PRIVATE QtTUI::QtTUI)
```

`QtTUI::QtTUI` links `Qt6::Core` for your target.

## A first program

```cpp
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QStringListModel model({"Alice", "Bob", "Carol"});

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *view = new QTuiListView(&window);
    view->setModel(&model);
    window.setCentralWidget(view);

    // Enter in the list stops the program
    QObject::connect(view, &QTuiListView::activated, &app,
                     [](const QModelIndex &) { QCoreApplication::quit(); });

    view->setFocus();
    window.show();
    return app.exec();
}
```

The program does these steps:

1. It makes a `QTuiApplication`. The application object takes control of
   the terminal.
2. It makes a `QTuiMainWindow` and gives it to the application with
   `setMainWindow()`. Only the widget tree of the main window gets painted
   and gets events.
3. It adds widgets as children of the main window.
4. It calls `show()` on the main window. If the window has no geometry,
   `show()` gives it the full terminal size.
5. It calls `exec()` to start the event loop.

## The main window

`QTuiMainWindow` is the root of the widget tree. It gets all key, mouse,
resize and paste events from the application, and it sends them to the
correct widget. It also keeps the Tab focus chain, the shortcuts and the
hover.

A minimal main window has a central widget, or a layout:

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *layout = createVBoxLayout(&window);
    auto *name = new QTuiLineEdit(&window);
    auto *quit = new QTuiPushButton(QStringLiteral("Quit"), &window);
    layout->addWidget(name);
    layout->addWidget(quit);
    layout->addStretch(1);
    window.setLayout(layout);

    QObject::connect(quit, &QTuiPushButton::clicked, &app, &QCoreApplication::quit);

    name->setFocus();
    window.show();
    return app.exec();
}
```

For the menu bar, the tool bars, the status bar, the dock widgets and the
full list of keys, see [Main window](main-window.md). For the layouts, see
[Layouts](layouts.md).

## The event loop

`QTuiApplication::exec()` is the Qt event loop of `QCoreApplication`.
Timers, signals and slots, and queued connections work as in any Qt
program.

The application reads the terminal input when stdin has data. The input
parser changes the bytes into key, mouse, resize and paste events. The
application sends these events to the main window. For the full input path,
see [Events and rendering](events-and-rendering.md).

The application does not paint each time through the loop. A widget calls
`markDirty()` when its look changes. The render scheduler then starts one
render pass from the event loop, and it paints only the dirty areas. If
many widgets change in one loop iteration, there is one render pass.

Call `QCoreApplication::quit()` or `exit()` to stop the loop. When the
application object is destroyed, it restores the terminal.

## QTuiApplication

`#include <QtTUI/QTuiApplication.h>` · Inherits: `QCoreApplication`

`QTuiApplication` owns the terminal backend, the input parser, the screen
buffer and the render scheduler. It is the QtTUI version of
`QApplication`, but it inherits from `QCoreApplication`, not from
`QGuiApplication`.

The constructor does these steps:

- It sets the color mode from the environment (`QTTUI_COLOR`, `NO_COLOR`,
  `COLORTERM`, `TERM`). Call `AnsiRenderer::setColorMode()` after the
  constructor to change it.
- It enters the alternate screen and raw mode.
- If the terminal starts correctly, it sends all Qt messages (`qDebug()`,
  `qWarning()` and the other functions) to a log file. Text on stderr
  would break the screen. The file is
  `$XDG_STATE_HOME/qttui/<application name>.log`, or
  `~/.local/state/qttui/<application name>.log`. Without an application
  name, the file name is `qttui.log`.
- If the terminal does not start (for example, with no terminal in a test),
  it writes one warning and continues. `terminalInitialized()` then returns
  `false`.

The destructor leaves the alternate screen and raw mode. A handler from
`std::atexit()` also restores the terminal if the program stops without the
destructor.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("notes"));

    // Set these before you make the widgets
    QTuiApplication::setLayoutDirection(Qt::LeftToRight);
    QTuiApplication::setReducedMotion(true);
    QTuiApplication::setFocusCursor(true);

    QTuiMainWindow window;
    app.setMainWindow(&window);
    auto *edit = new QTuiPlainTextEdit(&window);
    window.setCentralWidget(edit);

    // Copy the text to the clipboard when the program stops
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [edit]() {
        QTuiApplication::clipboard()->setText(edit->toPlainText());
    });

    edit->setFocus();
    window.show();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiApplication(int &argc, char **argv)` | Makes the application and takes control of the terminal. Make only one at a time. |
| `~QTuiApplication()` | Restores the terminal, puts back the previous Qt message handler, and deletes the QtTUI subsystems. |
| `static QTuiApplication *instance()` | Returns the application object, or `nullptr` if there is none. |
| `TerminalBackend *backend() const` | Returns the terminal backend (`PosixBackend` on Linux). |
| `ScreenBuffer *screenBuffer() const` | Returns the double-buffered screen buffer that the widgets paint into. |
| `RenderScheduler *scheduler() const` | Returns the render scheduler that collects the dirty areas. |
| `bool terminalInitialized() const` | Returns `true` if the alternate screen and raw mode both started. Returns `false` with no terminal, for example in a test. |
| `void setMainWindow(QTuiMainWindow *window)` | Sets the main window. The application sends all key, mouse, resize and paste events to it, and the render pass paints only its tree. The application does not take ownership. |
| `QTuiMainWindow *mainWindow() const` | Returns the main window, or `nullptr`. |
| `void triggerRender()` | Paints the full screen now, not from the event loop. Usually you do not need it: `markDirty()` schedules a render pass. |
| `static QTuiClipboard *clipboard()` | Returns the application clipboard. The function makes it on the first call. See [QTuiClipboard](widget.md#qtuiclipboard). |
| `static Qt::LayoutDirection layoutDirection()` | Returns the default layout direction of the widgets. The default is `Qt::LeftToRight`. |
| `static void setLayoutDirection(Qt::LayoutDirection direction)` | Sets the default layout direction. `Qt::LayoutDirectionAuto` becomes `Qt::LeftToRight`. Call it before you make the widgets. |
| `static bool reducedMotion()` | Returns `true` if reduced motion is on. The start value is `true` if `QTTUI_REDUCE_MOTION` is a number that is not 0. |
| `static void setReducedMotion(bool reduced)` | Turns reduced motion on or off. With reduced motion, the busy bar does not move and a pressed button does not flash. |
| `static bool focusCursor()` | Returns `true` if the focus cursor is on. The start value is `true` if `QTTUI_FOCUS_CURSOR` is a number that is not 0. |
| `static void setFocusCursor(bool on)` | Turns the focus cursor on or off. If it is on, the terminal cursor stays on the first cell of the focus widget, also if the widget is not editable. Screen magnifiers that follow the cursor then follow the focus. |
| `void suspend()` | Gives the terminal back to the shell and stops the process, as Ctrl+Z does. On `fg`, the terminal modes start again and the screen paints again. |

### Protected functions

| Function | Description |
| --- | --- |
| `bool event(QEvent *event) override` | Sends the key, mouse, resize and paste events to the main window. Without a main window, these events go to `QCoreApplication::event()`. |

`QTuiApplication` has no translation functions of its own. Use the
functions that it inherits from `QCoreApplication`, for example
`installTranslator()`. See [Translation](#translation).

### Ctrl+Z

The main window calls `suspend()` when the user pushes Ctrl+Z. It does not
call it in these cases:

- An enabled action that the user can reach has Ctrl+Z as its shortcut.
  The action gets the key.
- The focus widget accepts text input (a line edit, a text edit, a spin
  box). The widget gets the key, for undo.

### Translation

The QtTUI widgets mark their visible text with `tr()`, for example the
"OK" and "Cancel" buttons of the dialogs. The translation context is the
class name with the namespace, for example `QtTUI::QTuiFileDialog`. To
translate this text, load a `QTranslator` and install it before you make
the widgets.

```cpp
#include <QtTUI/QtTUI>
#include <QLocale>
#include <QTranslator>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("myapp"), QStringLiteral("_"),
                        QStringLiteral("translations")))
        QCoreApplication::installTranslator(&translator);

    QTuiMainWindow window;
    app.setMainWindow(&window);
    window.setCentralWidget(new QTuiPushButton(QObject::tr("Start"), &window));
    window.show();
    return app.exec();
}
```

## The umbrella header

`#include <QtTUI/QtTUI>` includes all public classes of QtTUI. The file has
no extension, as the Qt module headers have none.

These headers are not in the umbrella header. Include them alone if you
need them:

| Header | Contents |
| --- | --- |
| `QtTUI/RenderUtils.h` | The paint helpers: `cellStyle()`, `fillRect()`, `drawBorder()`, `drawWindowFrame()`, `renderText()`, `renderTextTruncated()`. |
| `QtTUI/RenderScheduler.h` | The render scheduler. |
| `QtTUI/QTuiKeySequence.h` | The key sequence of a shortcut. |
| `QtTUI/QTuiAbstractSlider.h` | The base class of the sliders. |
| `QtTUI/QTuiAbstractSpinBox.h` | The base class of the spin boxes. |

All public headers include each other as `<QtTUI/Name.h>`, and all classes
are in the `QtTUI` namespace.

## QtTUIGlobal.h

`#include <QtTUI/QtTUIGlobal.h>`

All public headers include this header first. It includes
`<QtCore/QtGlobal>` and defines nothing more. There is no `QTTUI_EXPORT`
macro, because QtTUI builds only as a static library.
