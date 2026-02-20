QtTUI 🖥️✨
==========

**QtTUI** is a modern, cross-platform Text User Interface (TUI) framework built seamlessly on top of the Qt framework. It brings the immense power of Qt's Model/View architecture, the event loop, and Signals & Slots directly to your terminal.

Build highly responsive, data-driven terminal applications using the Qt APIs you already know and love, without sacrificing the modern aesthetics of today's best CLI tools.

🚀 Features
-----------

*   **Native Qt Model/View Architecture:** Fully compatible with QAbstractItemModel, QStandardItemModel, QFileSystemModel, and custom models. If it works in standard Qt, it works in QtTUI.
    
*   **Modern TUI Capabilities:** Supports 24-bit TrueColor, mouse events (click, scroll, drag), and terminal resizing out of the box.
    
*   **Signals & Slots:** Utilize Qt's robust event-driven communication system for intuitive widget interactions.
    
*   **Familiar Widget API:** Includes terminal-optimized equivalents of standard Qt widgets (e.g., QTuiTableView, QTuiListView, QTuiLineEdit).
    
*   **Cross-Platform:** Runs flawlessly on Linux, macOS, and Windows terminals.
    

🏗️ Architecture
----------------

QtTUI acts as a rendering and event-handling bridge between your terminal emulator and Qt's core logic. It delegates the data management strictly to Qt's underlying models, ensuring perfect separation of concerns.

When your QAbstractItemModel updates, it emits standard Qt signals (dataChanged, rowsInserted, etc.). QtTUI's view classes (like QTuiTableView) listen to these signals and automatically invalidate only the necessary terminal screen regions, redrawing them efficiently.

📦 Installation
---------------

### Prerequisites

*   CMake (3.16+)
    
*   Qt 6.x (Core module is required; GUI is not required)
    
*   A modern C++17 compatible compiler
    

### Build via CMake

```bash
git clone https://github.com/aedrax/QtTUI.git
cd QtTUI
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

To use QtTUI in your own project, simply find the package in your CMakeLists.txt:

```CMake
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(QtTUI REQUIRED)

add_executable(my_tui_app main.cpp)
target_link_libraries(my_tui_app PRIVATE Qt6::Core QtTUI::QtTUI)
```

💻 Quick Start
--------------

Here is a simple example demonstrating how to use a standard QStandardItemModel with a QtTUI table view.

```C++
#include <QCoreApplication>
#include <QStandardItemModel>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiMainWindow.h>

int main(int argc, char *argv[]) {
    // Use QTuiApplication instead of QApplication/QCoreApplication
    QTuiApplication app(argc, argv);

    // Set up standard Qt Model
    QStandardItemModel model(4, 2);
    model.setHorizontalHeaderLabels({"Name", "Role"});
    model.setItem(0, 0, new QStandardItem("Alice"));
    model.setItem(0, 1, new QStandardItem("Admin"));
    model.setItem(1, 0, new QStandardItem("Bob"));
    model.setItem(1, 1, new QStandardItem("User"));

    // Set up TUI View
    QTuiMainWindow mainWindow;
    QTuiTableView *tableView = new QTuiTableView(&mainWindow);
    
    tableView->setModel(&model);
    mainWindow.setCentralWidget(tableView);
    
    mainWindow.show();

    return app.exec();
}
```

🤝 Contributing
---------------

Contributions are welcome! Whether you're adding new widgets, fixing bugs, or improving documentation, your help makes QtTUI better.

1.  Fork the Project
    
2.  Create your Feature Branch (git checkout -b feature/AmazingFeature)
    
3.  Commit your Changes (git commit -m 'Add some AmazingFeature')
    
4.  Push to the Branch (git push origin feature/AmazingFeature)
    
5.  Open a Pull Request
    

📄 License
----------

Distributed under the MIT License. See LICENSE for more information.

Would you like me to help you draft the C++ header files for some of these core classes (like QTuiApplication or QTuiTableView) to get the actual implementation started?