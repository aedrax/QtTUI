# QtTUI

QtTUI is a terminal UI library for C++17 and Qt 6. It copies the Qt Widgets
API for the terminal: `QTuiWidget`, `QTuiLineEdit`, `QTuiTableView`,
`QTuiDialog` and more, in the `QtTUI` namespace. It uses only the Qt Core
module, not Qt GUI.

The views use standard Qt models (`QAbstractItemModel`) and
`QItemSelectionModel`. The views do not copy the model data. They read the
model when they paint, and they repaint only the rows that a model signal
changes.

## Contents

- Widgets: buttons, check boxes, radio buttons, line edits, text edits, spin
  boxes, sliders, combo boxes, progress bars, a calendar, list, table, tree
  and column views, tabs, splitters, scroll areas, group boxes, MDI windows,
  dock widgets, menus, tool bars, status bars, tooltips and context menus.
- Dialogs: `QTuiDialog` with `exec()`, `QTuiMessageBox`, `QTuiInputDialog`,
  `QTuiProgressDialog`, `QTuiFileDialog` and `QTuiWizard`.
- Layouts: box, grid, form and stacked layouts, with size policies and
  right-to-left direction.
- Terminal: 24-bit color, with 256-color, 16-color and no-color modes. Mouse
  clicks, drags, the wheel and the hover highlight. Bracketed paste, copy
  with OSC 52, the kitty keyboard protocol, Ctrl+Z and resize.
- Themes: a dark theme, a light theme, or your own theme through
  `Theme_Engine`.
- `UI_Loader` makes a widget tree from a JSON description.

QtTUI is tested on Linux only. The Windows backend is not compiled or tested.

The [documentation](docs/README.md) has a page for each part of the library,
with examples.

## Build

You must have CMake 3.16 or later, Qt 6 Core and a C++17 compiler.

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build
sudo cmake --install build
```

The tests use Qt Test and RapidCheck. If RapidCheck is not installed, CMake
downloads it.

## Use

Link the installed package:

```cmake
find_package(QtTUI REQUIRED)
target_link_libraries(my_app PRIVATE QtTUI::QtTUI)
```

Or add the source tree. The QtTUI tests and demos then do not build.

```cmake
add_subdirectory(QtTUI)
target_link_libraries(my_app PRIVATE QtTUI::QtTUI)
```

Use the same Qt version that QtTUI was built with.

## Example

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

    window.show();
    return app.exec();
}
```

The `demos/` directory has `demo_hello`, `demo_todo`, `demo_table` and
`demo_gallery`. The gallery shows all widgets. After a build, the demos are
in `build/demos/`.

## Keys

Each mouse action also has a key.

| Key | Action |
| --- | --- |
| Tab, Shift+Tab | Move the focus |
| F1 | Show the tooltip of the focus widget |
| F10 | Open the menu bar |
| Shift+F10, Menu | Open the context menu |
| Shift+arrows, Space, Ctrl+A | Select items in a view |
| F2 | Edit the current item |

Some terminals, for example Konsole, keep Shift+click for their own text
selection. In a view, drag with the mouse to select a range.

## Environment variables

| Variable | Values | Effect |
| --- | --- | --- |
| `QTTUI_COLOR` | `truecolor`, `256`, `16`, `none` | Sets the color mode. Without it, QtTUI reads `NO_COLOR`, `COLORTERM` and `TERM`. |
| `NO_COLOR` | a value that is not empty | Turns off color. The focus and the selection use reverse video. |
| `QTTUI_MOUSE` | `off`, `click`, `drag`, `all` (default) | Sets the mouse events that QtTUI gets. A drag needs `drag` or `all`. The hover needs `all`. Use `off` to keep the text selection of the terminal. |
| `QTTUI_REDUCE_MOTION` | `1` | Stops the busy bar animation and the button flash. |
| `QTTUI_FOCUS_CURSOR` | `1` | Keeps the terminal cursor on the focus widget, for screen magnifiers. |

An application can set the same options in code, for example with
`AnsiRenderer::setColorMode()`, `QTuiApplication::setReducedMotion()` and
`QTuiApplication::setFocusCursor()`.

QtTUI does not reorder bidirectional text and has no screen reader
interface.

## License

MIT. See `LICENSE`.
