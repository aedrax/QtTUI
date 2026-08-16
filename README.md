QtTUI 🖥️✨
==========

**QtTUI** is a Text User Interface (TUI) framework built on Qt 6 Core. It brings Qt's Model/View architecture, the event loop, and signals and slots to the terminal, with widgets that follow the Qt Widgets API: `QTuiTableView`, `QTuiLineEdit`, `QTuiDialog`, and more, all in the `QtTUI` namespace.

QtTUI needs only the Qt Core module. It does not use Qt GUI.

🚀 Features
-----------

*   **Qt Model/View:** The views work with any `QAbstractItemModel`: the Qt Core models (`QStringListModel`, `QSortFilterProxyModel`, ...) and your own. A model from Qt GUI (such as `QStandardItemModel`) also works if you link `Qt6::Gui` yourself. The views read the model on each paint and do not copy its data.

*   **Widgets:** Buttons, check boxes, radio buttons, line edits (with validators, password mode, completion, undo), text edits, spin boxes, sliders, combo boxes, progress bars (also a busy bar), table, list, tree and column views (sorting, column resizing), tabs, splitters, scroll areas, group boxes, MDI windows, menus, tool bars, status bars, tooltips, and context menus.

*   **Dialogs:** `QTuiDialog` with `exec()`, `QTuiMessageBox`, `QTuiInputDialog`, `QTuiProgressDialog`, `QTuiFileDialog`, and `QTuiWizard`. Dialogs are modal.

*   **Layouts:** Box, grid, form and stacked layouts, with stretch, spacing, size hints and size policies. Any `QTuiWidget` can have a layout, as `QWidget` can. Right-to-left layout direction mirrors the layouts.

*   **Terminal support:** 24-bit color with 256-color, 16-color and no-color fallback (`NO_COLOR` is respected), mouse clicks, wheel, drag and hover, bracketed paste, copy to the system clipboard (OSC 52), the kitty keyboard protocol, Ctrl+Z suspend, and terminal resizing.

*   **Themes:** A default dark theme and a built-in light theme, or your own themes through `Theme_Engine`.

*   **Other:** Library text goes through `tr()` for translation. Labels can show a small subset of Markdown or HTML. `UI_Loader` builds widget trees from a JSON description.

**Platforms:** QtTUI is developed and tested on Linux. The Windows backend is not compiled or tested yet. macOS is not tested.

🏗️ Architecture
----------------

QtTUI is a rendering and event bridge between the terminal and Qt's core logic. The data stays in Qt models.

When a model changes, it emits the standard Qt signals (`dataChanged`, `rowsInserted`, ...). The QtTUI views listen to these signals and mark only the affected screen regions for a repaint. A render scheduler collects the changes of one event loop pass into one repaint, and the renderer writes only the cells that changed.

📦 Installation
---------------

### Prerequisites

*   CMake 3.16 or later
*   Qt 6 (only the Core module)
*   A C++17 compiler

### Build, test and install

```bash
git clone https://github.com/aedrax/QtTUI.git
cd QtTUI
cmake -B build
cmake --build build -j
ctest --test-dir build
sudo cmake --install build
```

The tests use Qt Test and RapidCheck. CMake fetches RapidCheck if it is not installed.

### Use QtTUI in your project

After you install QtTUI, find the package. Use the same Qt that QtTUI was built with; the package checks the Qt version.

```CMake
find_package(QtTUI REQUIRED)

add_executable(my_tui_app main.cpp)
target_link_libraries(my_tui_app PRIVATE QtTUI::QtTUI)
```

You can also add the source tree as a subproject. Then the QtTUI tests and demos do not build by default.

```CMake
add_subdirectory(QtTUI)
target_link_libraries(my_tui_app PRIVATE QtTUI::QtTUI)
```

💻 Quick Start
--------------

This example shows a `QStringListModel` (from Qt Core) in a list view.

```C++
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    // Use QTuiApplication instead of QApplication or QCoreApplication
    QTuiApplication app(argc, argv);

    // A standard Qt model
    QStringListModel model({"Alice", "Bob", "Carol"});

    // The main window owns the screen
    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *view = new QTuiListView(&window);
    view->setModel(&model);
    window.setCentralWidget(view);

    window.show();
    return app.exec();
}
```

The demos in `demos/` show more: `demo_hello`, `demo_todo`, `demo_table`, and `demo_gallery`, which shows every widget. After a build, run them from `build/demos/`.

⚙️ Environment variables
------------------------

| Variable | Values | Effect |
| --- | --- | --- |
| `QTTUI_COLOR` | `truecolor`, `256`, `16`, `none` | Sets the color depth. Without it, QtTUI uses `NO_COLOR`, `COLORTERM` and `TERM`. |
| `NO_COLOR` | any non-empty value | No colors. Focus and selection use reverse video. |
| `QTTUI_MOUSE` | `off`, `click`, `drag`, `all` (default) | Sets the mouse reporting level. Use `off` to keep the terminal's own text selection. |
| `QTTUI_REDUCE_MOTION` | `1` | No moving busy bar and no pressed-button flash. |
| `QTTUI_FOCUS_CURSOR` | `1` | The terminal cursor stays on the focused widget, so screen readers and magnifiers that follow the cursor follow the focus. |

Applications can set the same options in code, for example `AnsiRenderer::setColorMode()`, `QTuiApplication::setReducedMotion()` and `QTuiApplication::setFocusCursor()`.

**Not included:** bidirectional text reordering for right-to-left text, and a screen reader interface. Mouse dragging needs `QTTUI_MOUSE` set to `drag` or `all`, and hover needs `all`.

🤝 Contributing
---------------

Contributions are welcome: new widgets, bug fixes, or better documentation.

1.  Fork the project.
2.  Create a feature branch (`git checkout -b feature/AmazingFeature`).
3.  Commit your changes (`git commit -m 'Add some AmazingFeature'`).
4.  Push the branch (`git push origin feature/AmazingFeature`).
5.  Open a pull request.

The CI workflow (`.github/workflows/ci.yml`) builds QtTUI with the Qt 6 of Ubuntu 24.04 and runs all tests.

📄 License
----------

Distributed under the MIT License. See LICENSE for more information.
