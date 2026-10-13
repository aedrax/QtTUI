# Main window, menus and actions

This page covers the main window and the parts around it: the menu bar,
menus, actions, shortcuts, tool bars, the status bar and tooltips.

## QTuiMainWindow

`#include <QtTUI/QTuiMainWindow.h>` · Inherits: `QTuiWidget`

The top-level widget of a program. It fills the terminal and puts a menu bar,
tool bars, dock widgets, a status bar and a central widget in it. It also
routes all keys and mouse events: it owns the focus chain, the shortcuts, the
context menus and the modality of dialogs. Give it to
`QTuiApplication::setMainWindow()`, because menus, dialogs and tooltips use
the main window of the application.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QCoreApplication>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *menuBar = new QTuiMenuBar;
    QTuiMenu *fileMenu = menuBar->addMenu(QStringLiteral("&File"));
    QTuiAction *quit = fileMenu->addAction(QStringLiteral("Quit"), QStringLiteral("Ctrl+Q"));
    QObject::connect(quit, &QTuiAction::triggered, &app, &QCoreApplication::quit);
    window.setMenuBar(menuBar);

    auto *statusBar = new QTuiStatusBar;
    statusBar->showMessage(QStringLiteral("Ready"));
    window.setStatusBar(statusBar);

    auto *editor = new QTuiTextEdit;
    editor->setToolTip(QStringLiteral("Type your notes here"));
    window.setCentralWidget(editor);

    editor->setFocus();
    window.show();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiMainWindow(QTuiWidget *parent = nullptr)` | Makes an empty main window. |
| `void setCentralWidget(QTuiWidget *widget)` | Sets the widget that fills the area that remains. The window becomes its parent. The old central widget gets no parent, and the window does not delete it. |
| `QTuiWidget *centralWidget() const` | Returns the central widget, or `nullptr`. |
| `void setLayout(QTuiLayout *layout)` | Sets a layout for the central area. If a layout is set, it gets the central area and the central widget does not. |
| `QTuiLayout *layout() const` | Returns the layout, or `nullptr`. |
| `void setMenuBar(QTuiMenuBar *menuBar)` | Sets the menu bar. It gets the top row. The old menu bar gets no parent. |
| `QTuiMenuBar *menuBar() const` | Returns the menu bar, or `nullptr`. |
| `void addToolBar(QTuiToolBar *toolBar)` | Adds a tool bar. Each tool bar gets one row below the menu bar, in the order that you add them. |
| `void removeToolBar(QTuiToolBar *toolBar)` | Removes a tool bar. The tool bar gets no parent. |
| `QList<QTuiToolBar *> toolBars() const` | Returns the tool bars. |
| `void setStatusBar(QTuiStatusBar *statusBar)` | Sets the status bar. It gets the bottom row. The old status bar gets no parent. |
| `QTuiStatusBar *statusBar() const` | Returns the status bar, or `nullptr`. |
| `void addDockWidget(Qt::DockWidgetArea area, QTuiDockWidget *dockWidget)` | Adds a dock widget to an edge. If the dock is in a different area, it moves. See [containers](containers.md). |
| `void removeDockWidget(QTuiDockWidget *dockWidget)` | Removes a dock widget. The dock gets no parent. |
| `void show()` | Shows the window. If the geometry is empty, the window takes the size of the terminal. Then it calls the layout. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints nothing. The children paint themselves. |
| `void resizeEvent(TuiResizeEvent *event)` (protected) | Takes the new terminal size and does the layout again. |
| `void keyPressEvent(TuiKeyEvent *event)` (protected) | Handles shortcuts and window keys, then sends the key to the focus widget. See "Keys". |
| `void pasteEvent(TuiPasteEvent *event)` (protected) | Sends pasted text to the focus widget. |
| `void mousePressEvent(TuiMouseEvent *event)` (protected) | Finds the widget under the mouse and sends the event to it. See "Mouse". |

### Layout of the window

The window puts its parts in this order:

1. The menu bar: one row at the top.
2. The tool bars: one row each, below the menu bar.
3. The status bar: one row at the bottom.
4. The dock widgets, in the order top, bottom, left, right. A top or bottom
   dock is as high as its `minimumSize().height()`, or 3 rows if that is 0. A
   left or right dock is as wide as its `minimumSize().width()`, or 10 columns
   if that is 0.
5. The layout, or else the central widget: the area that remains.

Hidden parts take no space. The window does the layout again when the
terminal size changes and when you add or remove a part.

### Focus chain and Tab

The focus chain is the list of the widgets that take Tab focus. A widget is in
the chain if it is visible, enabled, and its `focusPolicy()` has the
`Qt::TabFocus` bit. The order is the order of the widget tree: a parent comes
before its children. Tab moves to the next widget in the chain and Shift+Tab
moves to the previous one. At the end of the chain, focus goes back to the
start. When a widget gets focus from Tab or a click, the window shows the
tooltip of that widget.

While focus is in an open popup (for example a menu), Tab does not move focus
out of the popup.

### Shortcuts

Each `QTuiAction` with a shortcut is a global shortcut. The window looks at
the shortcuts before all other keys. An action takes the key only if all of
these conditions are true:

- The action is enabled.
- The key and the modifiers are the same as the shortcut. Ctrl+S does not
  match Ctrl+Shift+S.
- The widgets that own the action are visible. A closed menu does not count
  as hidden. An action without a widget owner is always available.
- The focus widget does not take the key for text. If the focus widget
  accepts text input, it keeps bare text keys (for example `Q` or `Shift+Q`)
  and the edit keys Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y, Ctrl+C, Ctrl+X, Ctrl+V
  and Ctrl+A.
- If a modal dialog is open, the action belongs to the dialog or to a widget
  in it.

If no action takes the key, Alt+letter (without Ctrl) opens the menu with that
mnemonic in the menu bar. This works also when the menu bar does not have
focus, but not while a modal dialog is open.

### Context menus

A right click on a widget, or Shift+F10 or the Menu key while the widget has
focus, asks for a context menu. The window looks at the
`contextMenuPolicy()` of the widget:

- `Qt::DefaultContextMenu`: the window asks the parent widget.
- `Qt::CustomContextMenu`: the widget emits
  `customContextMenuRequested(pos)`. The position is relative to the
  top-left corner of the widget.
- `Qt::ActionsContextMenu`: the window shows a menu with the `actions()` of
  the widget. The menu deletes itself when it closes. If the widget has no
  actions, nothing occurs.
- `Qt::NoContextMenu` or `Qt::PreventContextMenu`: no menu, and the window
  does not ask the parent.

For Shift+F10, the menu opens one row below the top-left corner of the focus
widget. For a right click, it opens at the mouse position.

### Modal dialogs

A [dialog](dialogs.md) is a popup in the main window. While a dialog is
visible, the window is modal:

- Keys go only to widgets in the dialog.
- Tab and Shift+Tab move focus only between the widgets of the dialog.
- Only the shortcuts of actions in the dialog work. F10 and the Alt menu
  mnemonics do not work.
- Escape rejects the dialog. If a popup in the dialog is open (for example a
  completer or a dropdown), or an item view edits an item, Escape goes to
  that popup or editor first.
- Mouse events outside the dialog do nothing.

If more than one dialog is open, the dialog that opened last is the modal one.

### Keys

The window examines a key in the order of this table. The first row that
matches takes the key.

| Key | Action |
| --- | --- |
| A shortcut of an action | Triggers the action. See "Shortcuts". |
| Alt+letter | Opens the menu bar menu with that mnemonic. |
| Shift+F10, Menu | Asks for the context menu of the focus widget. |
| F10 | Opens the first menu of the menu bar. Not while a modal dialog is open. |
| F1 | Shows the tooltip of the focus widget again, if it has one. |
| Ctrl+Z | Suspends the program (`QTuiApplication::suspend()`), as in a shell. Not if the focus widget accepts text input. |
| Tab, Shift+Tab (in a modal dialog) | Moves focus to the next or previous widget in the dialog. |
| Escape (in a modal dialog) | Rejects the dialog. |
| Tab | Moves focus to the next widget in the focus chain. Not while focus is in a popup. |
| Shift+Tab | Moves focus to the previous widget in the focus chain. Not while focus is in a popup. |
| All other keys | Go to the focus widget. |

Each key and each mouse press also hides the tooltip.

### Mouse

- A press goes to the deepest visible widget under the mouse. Open popups are
  above the other widgets, and the popup that opened last is on top.
- A press outside all open popups closes them, and the press goes to no
  widget.
- A press moves focus to the widget if its `focusPolicy()` has the
  `Qt::ClickFocus` bit and the widget is enabled. Then the tooltip of the
  widget shows.
- A press in an MDI sub-window makes that sub-window active.
- After a left press, the widget gets all drag events and the release, also
  outside its geometry.
- A right press asks for a context menu.
- A second press of the same button on the same cell in 400 ms or less also
  sends a double-click event.
- Movement without a button only moves the hover. If the mouse stays on a
  widget for 700 ms, the tooltip of the widget shows.

## QTuiMenuBar

`#include <QtTUI/QTuiMenuBar.h>` · Inherits: `QTuiWidget`

A one-row bar with menu titles. Give it to `QTuiMainWindow::setMenuBar()`.
An `&` in a title marks the mnemonic letter. The bar does not show the `&`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void buildMenus(QTuiMainWindow *window)
{
    auto *bar = new QTuiMenuBar;
    QTuiMenu *file = bar->addMenu(QStringLiteral("&File"));
    file->addAction(QStringLiteral("&Open"), QStringLiteral("Ctrl+O"));
    file->addSeparator();
    file->addAction(QStringLiteral("&Quit"), QStringLiteral("Ctrl+Q"));

    auto *help = new QTuiMenu(QStringLiteral("&Help"));
    help->addAction(QStringLiteral("&About"));
    bar->addMenu(help);

    window->setMenuBar(bar);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiMenuBar(QTuiWidget *parent = nullptr)` | Makes an empty menu bar. It takes strong focus. |
| `QTuiMenu *addMenu(const QString &title)` | Makes a menu with the title, adds it and returns it. The bar owns the menu. |
| `void addMenu(QTuiMenu *menu)` | Adds a menu that you made. |
| `QList<QTuiMenu *> menus() const` | Returns the menus. |
| `bool triggerMnemonic(QChar ch)` | Opens the menu with the mnemonic `ch`. If no `&` mnemonic matches, the first letter of a title can match. Case is not important. Returns `false` if no menu matches. |
| `bool openFirstMenu()` | Opens the first menu. Returns `false` if there are no menus. The main window calls it for F10. |
| `QSize minimumSize() const` | Returns the width of all titles (each title with one space on each side) and a height of 1. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the titles. The active title has inverted colors and bold text. |
| `QRect hoverRect() const` (protected) | Returns the title under the mouse. |

### Keys

| Key | Action |
| --- | --- |
| Left | Makes the previous title active. If a menu is open, opens the previous menu. |
| Right | Makes the next title active. If a menu is open, opens the next menu. |
| Enter, Return, Down | Opens the menu of the active title. |
| Escape | Closes the open menu. |
| Alt+letter | Opens the menu with that mnemonic. |

The bar gets these keys only while it has focus. An open menu has focus, so
Left and Right do not go to the bar while a menu is open.

### Mouse

- A click on a title opens that menu.

## QTuiMenu

`#include <QtTUI/QTuiMenu.h>` · Inherits: `QTuiWidget`

A popup list of actions in a border, with separators and submenus. A menu is
always a popup, and it is hidden until you call `popup()`. The menu shows the
shortcut text of each action on the right, and `>` for a submenu. Menus have
no check marks or icons.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void addViewMenu(QTuiMenuBar *bar)
{
    QTuiMenu *view = bar->addMenu(QStringLiteral("&View"));
    QTuiAction *refresh = view->addAction(QStringLiteral("Refresh"), QStringLiteral("F5"));
    QObject::connect(refresh, &QTuiAction::triggered, [] {
        // Load the data again
    });

    QTuiMenu *zoom = view->addMenu(QStringLiteral("Zoom"));
    zoom->addAction(QStringLiteral("Larger"));
    zoom->addAction(QStringLiteral("Smaller"));
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiMenu(const QString &title = QString(), QTuiWidget *parent = nullptr)` | Makes an empty, hidden menu. |
| `QString title() const` | Returns the title. |
| `void setTitle(const QString &title)` | Sets the title. The property is `title`. The menu bar or the parent menu shows the new title. |
| `QTuiAction *addAction(const QString &text)` | Makes an action with the text, adds it and returns it. The menu owns the action. |
| `QTuiAction *addAction(const QString &text, const QString &shortcut)` | Makes an action with the text and a shortcut, for example `"Ctrl+S"`. The menu owns the action. |
| `void addAction(QTuiAction *action)` | Adds an action that you made. The menu does not take ownership. If the action is deleted, the menu removes its item. |
| `void addSeparator()` | Adds a line between items. |
| `QTuiMenu *addMenu(const QString &title)` | Makes a submenu, adds it and returns it. |
| `void addMenu(QTuiMenu *menu)` | Adds a menu that you made as a submenu. |
| `void popup(const QPoint &pos)` | Shows the menu with its top-left corner at `pos` (screen cells). The menu moves to stay in the main window. It highlights the first item, gets focus and emits `aboutToShow()`. |
| `void close()` | Closes open submenus, emits `aboutToHide()` and hides the menu. Focus goes back to the widget that had it before. |
| `QList<QTuiAction *> actions() const` | Returns the actions of the items, also the actions of the submenu items. |
| `QSize minimumSize() const` | Returns the size for the widest item and one row for each item, with the border. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the border and the items. The highlighted item has inverted colors and bold text. |
| `void popupOutsideClick()` | Closes the menu. The main window calls it for a click outside the menu. |
| `QRect hoverRect() const` (protected) | Returns the item under the mouse, but not a separator. |

### Signals

| Signal | Description |
| --- | --- |
| `aboutToShow()` | The menu opened. |
| `aboutToHide()` | The menu closes. |

### Keys

| Key | Action |
| --- | --- |
| Up | Highlights the previous item. Separators are skipped. At the top, it goes to the bottom. |
| Down | Highlights the next item. Separators are skipped. At the bottom, it goes to the top. |
| Enter, Return | Triggers the highlighted action and closes the menu. On a submenu item, opens the submenu at the right of the menu. |
| Right | Opens the submenu of the highlighted item. |
| Left | Closes a submenu. |
| Escape | Closes the menu. |

A disabled action shows in the disabled color. It does not trigger, and the
menu stays open.

### Mouse

- A click on an item triggers its action and closes the menu. A click on a disabled action does nothing.
- A click on a submenu item opens the submenu.
- A click on a separator does nothing.

## QTuiAction

`#include <QtTUI/QTuiAction.h>` · Inherits: `QObject`

A command that menus, tool bars, context menus and shortcuts use. Each live
action is in a global list, and the main window uses that list to find
shortcuts. The shortcut is a string that `QTuiKeySequence` parses. Unlike
`QAction`, an action has no check state, icon or tooltip.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void addSaveAction(QTuiMainWindow *window, QTuiToolBar *toolBar)
{
    auto *save = new QTuiAction(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), window);
    QObject::connect(save, &QTuiAction::triggered, [] {
        // Write the file
    });
    toolBar->addAction(save);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiAction(QObject *parent = nullptr)` | Makes an action without text. |
| `QTuiAction(const QString &text, QObject *parent = nullptr)` | Makes an action with the text. |
| `QTuiAction(const QString &text, const QString &shortcut, QObject *parent = nullptr)` | Makes an action with the text and the shortcut. |
| `QString text() const` | Returns the text. An `&` in the text marks a mnemonic; menus do not show it. |
| `void setText(const QString &text)` | Sets the text. The property is `text`. |
| `QString shortcut() const` | Returns the shortcut string. |
| `void setShortcut(const QString &shortcut)` | Sets the shortcut, for example `"Ctrl+Shift+Z"`. The property is `shortcut`. |
| `QTuiKeySequence shortcutSequence() const` | Returns the parsed shortcut. It is not valid if the string is empty or the parser does not know it. |
| `static QList<QTuiAction *> registeredActions()` | Returns all live actions, in the order that they were made. |
| `bool isEnabled() const` | Returns `true` if the action is enabled. The default is `true`. |
| `void setEnabled(bool enabled)` | Enables or disables the action. The property is `enabled`. |
| `void trigger()` | Emits `triggered()` if the action is enabled. |

### Signals

| Signal | Description |
| --- | --- |
| `triggered()` | The action triggered. |
| `textChanged(const QString &text)` | The text changed. |
| `enabledChanged(bool enabled)` | The action became enabled or disabled. |

## QTuiWidgetAction

`#include <QtTUI/QTuiWidgetAction.h>` · Inherits: `QTuiAction`

An action that makes widgets, like `QWidgetAction`. Override `createWidget()`
in a subclass. The action keeps a list of the widgets that it made and deletes
them when it is deleted. Menus and tool bars do not call `requestWidget()`, so
you call it yourself.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class SearchAction : public QTuiWidgetAction {
public:
    using QTuiWidgetAction::QTuiWidgetAction;

    QTuiWidget *createWidget(QTuiWidget *parent) override
    {
        auto *edit = new QTuiLineEdit(parent);
        edit->setPlaceholderText(QStringLiteral("Search"));
        return edit;
    }
};

void addSearchField(QTuiToolBar *toolBar)
{
    auto *action = new SearchAction(QStringLiteral("Search"), toolBar);
    toolBar->addWidget(action->requestWidget(toolBar));
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiWidgetAction(QObject *parent = nullptr)` | Makes an action without text. |
| `QTuiWidgetAction(const QString &text, QObject *parent = nullptr)` | Makes an action with the text. |
| `virtual QTuiWidget *createWidget(QTuiWidget *parent)` | Makes a new widget for `parent`. The default returns `nullptr`. |
| `virtual void deleteWidget(QTuiWidget *widget)` | Deletes the widget if the action made it. |
| `QTuiWidget *requestWidget(QTuiWidget *parent)` | Calls `createWidget()`, keeps the widget in the list and returns it. |
| `void releaseWidget(QTuiWidget *widget)` | Calls `deleteWidget()`. |
| `QList<QTuiWidget *> createdWidgets() const` | Returns the widgets that the action made and did not delete. |

## QTuiKeySequence

`#include <QtTUI/QTuiKeySequence.h>` · Inherits: none

One key with modifiers, for shortcuts. QtTUI uses it because `QKeySequence`
is in Qt GUI. It holds one key only: there are no multi-key sequences such as
`Ctrl+K, Ctrl+C`.

The string format is zero or more modifiers and one key, with `+` between
them. Names are not case-sensitive, and spaces around each part are ignored.

- Modifiers: `Ctrl` or `Control`, `Alt`, `Shift`, `Meta`.
- Keys: the letters `A`-`Z`, the digits `0`-`9`, `F1`-`F35`, `Escape` or
  `Esc`, `Tab`, `Backspace`, `Delete` or `Del`, `Enter` or `Return`, `Space`,
  `Home`, `End`, `PgUp` or `PageUp`, `PgDown` or `PageDown`, `Insert` or
  `Ins`, `Up`, `Down`, `Left`, `Right`.

All other strings give a sequence that is not valid. A sequence that is not
valid never matches.

### Example

```cpp
#include <QtTUI/QTuiKeySequence.h>
#include <QDebug>

using namespace QtTUI;

void checkKey(int key, Qt::KeyboardModifiers modifiers)
{
    const QTuiKeySequence undo = QTuiKeySequence::fromString(QStringLiteral("ctrl+z"));
    if (undo.matches(key, modifiers))
        qDebug() << "Undo";
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiKeySequence()` | Makes a sequence that is not valid. |
| `QTuiKeySequence(int key, Qt::KeyboardModifiers modifiers)` | Makes a sequence from a `Qt::Key` value and modifiers. |
| `static QTuiKeySequence fromString(const QString &str)` | Parses a string such as `"Ctrl+S"`. |
| `bool isValid() const` | Returns `true` if the sequence has a key. |
| `int key() const` | Returns the key, or 0. |
| `Qt::KeyboardModifiers modifiers() const` | Returns the modifiers. |
| `bool matches(int key, Qt::KeyboardModifiers modifiers) const` | Returns `true` if the key and all modifiers are the same. Enter and Return match each other. |
| `bool isBareTextKey() const` | Returns `true` if the key types a character and the only modifier is Shift or none. |
| `bool operator==(const QTuiKeySequence &other) const` | Compares the key and the modifiers. |

## QTuiToolBar

`#include <QtTUI/QTuiToolBar.h>` · Inherits: `QTuiWidget`

A one-row bar of actions, separators and widgets. Give it to
`QTuiMainWindow::addToolBar()`. Each action shows as its text with one space
on each side. A separator shows as ` | `.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void addEditToolBar(QTuiMainWindow *window)
{
    auto *toolBar = new QTuiToolBar(QStringLiteral("Edit"));
    auto *cut = new QTuiAction(QStringLiteral("Cut"), toolBar);
    auto *copy = new QTuiAction(QStringLiteral("Copy"), toolBar);
    auto *find = new QTuiAction(QStringLiteral("Find"), toolBar);
    toolBar->addAction(cut);
    toolBar->addAction(copy);
    toolBar->addSeparator();
    toolBar->addAction(find);
    window->addToolBar(toolBar);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiToolBar(const QString &title = QString(), QTuiWidget *parent = nullptr)` | Makes an empty tool bar. It takes strong focus. |
| `void addAction(QTuiAction *action)` | Adds an action. The tool bar does not take ownership. |
| `void addSeparator()` | Adds a separator. |
| `void addWidget(QTuiWidget *widget)` | Adds a widget. The tool bar puts it at its place in the row, as wide as its `minimumSize().width()`. The tool bar becomes the parent of the widget. |
| `bool isMovable() const` | Returns the `movable` property. |
| `void setMovable(bool movable)` | Sets the `movable` property. The value has no effect: a tool bar cannot move. |
| `QString title() const` | Returns the title. The tool bar does not show it. |
| `QSize minimumSize() const` | Returns the width of all items and a height of 1. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the items. While the tool bar has focus, the current item has inverted colors and bold text. |
| `QRect hoverRect() const` (protected) | Returns the item under the mouse, but not a separator. |

### Keys

| Key | Action |
| --- | --- |
| Left | Makes the previous item current. At the start, it goes to the end. |
| Right | Makes the next item current. At the end, it goes to the start. |
| Enter, Return | Triggers the action of the current item. |

Separators are not items for the keys. Widgets are items, but Enter does
nothing on a widget.

### Mouse

- A click on an action makes it current and triggers it.
- A click on a separator does nothing.

## QTuiStatusBar

`#include <QtTUI/QTuiStatusBar.h>` · Inherits: `QTuiWidget`

A one-row bar for messages, with permanent widgets on the right. Give it to
`QTuiMainWindow::setStatusBar()`. The status bar does not take focus.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void setUpStatusBar(QTuiMainWindow *window)
{
    auto *statusBar = new QTuiStatusBar;
    statusBar->addPermanentWidget(new QTuiLabel(QStringLiteral("Line 1")));
    statusBar->showMessage(QStringLiteral("File saved"), 3000);
    window->setStatusBar(statusBar);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiStatusBar(QTuiWidget *parent = nullptr)` | Makes an empty status bar. |
| `void showMessage(const QString &text, int timeout = 0)` | Shows the text on the left. If `timeout` is more than 0, the message goes away after `timeout` ms. |
| `void clearMessage()` | Removes the message. |
| `QString currentMessage() const` | Returns the message. |
| `void addPermanentWidget(QTuiWidget *widget, int stretch = 0)` | Adds a widget on the right side and shows it. The status bar becomes its parent. The widget is as wide as its `minimumSize().width()`. The status bar ignores `stretch`. |
| `void removeWidget(QTuiWidget *widget)` | Removes the widget from the list of permanent widgets and hides it, as in Qt. The parent of the widget does not change. |
| `QSize minimumSize() const` | Returns 1 x 1. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the message and puts the permanent widgets on the right. |

### Signals

| Signal | Description |
| --- | --- |
| `messageChanged(const QString &text)` | The message changed. The text is empty after the message goes away. |

## QTuiToolTip

`#include <QtTUI/QTuiToolTip.h>` · Inherits: none

A one-row tip with inverted theme colors, like `QToolTip`. It is a popup in
the main window and never takes focus. All functions are static. You do not
usually call them: the main window shows the `toolTip()` of a widget when the
widget gets focus from Tab or a click, when you press F1, and when the mouse
stays on the widget for 700 ms. The tip hides on the next key or mouse press,
or after 4 seconds.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void explain(QTuiLineEdit *edit)
{
    edit->setToolTip(QStringLiteral("Type a name of 3 to 20 letters"));
    QTuiToolTip::showFor(edit);
}
```

### Functions

| Function | Description |
| --- | --- |
| `static void showText(const QPoint &pos, const QString &text)` | Shows the text with its top-left corner at `pos` (screen cells). The tip moves to stay in the main window. An empty text hides the tip. If there is no main window, nothing occurs. |
| `static void showFor(const QTuiWidget *widget)` | Shows the `toolTip()` of the widget below the widget, or above it if there is no space below. If the widget has no tooltip, the tip hides. |
| `static void hideText()` | Hides the tip. |
| `static bool isVisible()` | Returns `true` if the tip is visible. |
| `static QString text()` | Returns the text of the visible tip, or an empty string. |
