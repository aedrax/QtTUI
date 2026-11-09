# UI loader

This page covers `UI_Loader`, which makes a widget tree from a JSON description and writes a widget tree back to JSON.

## UI_Loader

`#include <QtTUI/UI_Loader.h>` · Inherits: `QObject`

The loader makes widgets from JSON. It does not read Qt Designer `.ui` files (XML), and it is not the same as `QUiLoader`. The format has the same parts as a `.ui` file: widgets with properties, layouts, and the pages of containers. A widget gets its position from `"geometry"` or from a layout.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

static const char kForm[] = R"({
    "type": "QTuiMainWindow",
    "objectName": "main",
    "geometry": { "x": 0, "y": 0, "width": 80, "height": 24 },
    "children": [
        { "type": "QTuiLabel", "text": "Name:",
          "geometry": { "x": 2, "y": 1, "width": 6, "height": 1 } },
        { "type": "QTuiLineEdit", "objectName": "name",
          "placeholderText": "Your name", "maxLength": 40,
          "geometry": { "x": 9, "y": 1, "width": 30, "height": 1 } },
        { "type": "QTuiLineEdit", "objectName": "password", "echoMode": "Password",
          "geometry": { "x": 9, "y": 3, "width": 30, "height": 1 } },
        { "type": "QTuiCheckBox", "objectName": "remember", "text": "Remember me",
          "checked": true,
          "geometry": { "x": 9, "y": 5, "width": 20, "height": 1 } },
        { "type": "QTuiPushButton", "objectName": "ok", "text": "OK",
          "geometry": { "x": 9, "y": 7, "width": 8, "height": 1 } }
    ]
})";

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    UI_Loader loader;
    auto *window = qobject_cast<QTuiMainWindow *>(loader.loadFromData(QByteArray(kForm)));
    if (!window)
        return 1;
    app.setMainWindow(window);

    // Find the widgets by their objectName
    auto *name = window->findChild<QTuiLineEdit *>(QStringLiteral("name"));
    auto *ok = window->findChild<QTuiPushButton *>(QStringLiteral("ok"));
    QObject::connect(ok, &QTuiPushButton::clicked, &app, &QCoreApplication::quit);

    name->setFocus();
    window->show();
    app.triggerRender();
    const int result = app.exec();
    delete window; // a widget without a parent belongs to the caller
    return result;
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit UI_Loader(QObject *parent = nullptr)` | Makes a loader. It registers all built-in widget types. |
| `QTuiWidget *load(const QString &filePath, QTuiWidget *parent = nullptr)` | Reads the file and calls `loadFromData()`. If the file cannot be opened, it shows a warning and returns `nullptr`. |
| `QTuiWidget *loadFromData(const QByteArray &data, QTuiWidget *parent = nullptr)` | Makes the widget tree and returns the root widget, with `parent` as its parent. If `parent` is null, the caller owns the root. It returns `nullptr` if the JSON is not valid, if the root is not an object, or if the root type is missing or unknown. |
| `QByteArray serialize(const QTuiWidget *root) const` | Returns the widget tree as indented JSON. A null root gives an empty array. See [Serialization](#serialization). |
| `void registerWidget(const QString &typeName, WidgetFactory factory, ChildAdopter adopter = nullptr)` | Adds a type for the `"type"` key, or replaces the factory and the adopter of a type. A type with an adopter can have `"pages"`. |
| `QStringList availableWidgets() const` | Returns the registered type names, in no specified order. |

`WidgetFactory` is `std::function<QTuiWidget *(QTuiWidget *parent)>`. The factory makes a widget with the given parent. If the factory returns `nullptr`, the loader shows a warning and skips the object and its children.

`ChildAdopter` is `std::function<void(QTuiWidget *container, QTuiWidget *page, const QJsonObject &entry)>`. The loader calls it for each entry in `"pages"`, after it makes the page as a child of the container. See [Custom types](#custom-types).

## JSON format

Each object in the file describes one widget.

| Key | Value | Description |
| --- | --- | --- |
| `type` | string | The registered type name, for example `"QTuiPushButton"`. It is necessary. |
| `objectName` | string | Sets `QObject::objectName`. Use it with `findChild()`. |
| `geometry` | object | `{ "x", "y", "width", "height" }` in screen cells. A missing field is 0. A widget in a layout and a page of a container do not need it. |
| `visible` | bool | Calls `setVisible()`. |
| `children` | array | The child widgets. The loader makes each child with this widget as its parent, and does nothing more with it. The loader ignores values that are not objects. |
| `layout` | object | The layout of the widget. See [Layouts](#layouts). |
| `pages` | array | The widgets that a container holds, for example the tabs of a `QTuiTabWidget`. See [Pages](#pages). |
| any other key | any | Sets the writable `Q_PROPERTY` with that name. |

One widget object can have `"children"`, `"layout"` and `"pages"` together.

Property keys:

- Each key that is not in the table above is a `Q_PROPERTY` name, for example `"text"`, `"checked"`, `"readOnly"`, `"placeholderText"` or `"enabled"`. The loader converts the JSON value to the property type.
- For an enum property, give the name of the value as a string, for example `"echoMode": "Password"`. For a flag property, join the names with `|`, for example `"alignment": "AlignRight|AlignVCenter"`.
- An unknown or read-only property, an unknown enum name, or a value that does not convert gives a warning. The loader skips that key and continues.
- The order of the keys in an object has no effect on the result.

Other rules:

- An object without `"type"`, or with an unknown type, gives a warning. The loader skips it and all its children.
- A widget in `"children"` is not in a layout and is not a page. For example, a child of a `QTuiTabWidget` does not become a tab. Use `"layout"` and `"pages"` for that.
- A widget in `"children"` without `"geometry"` has an empty geometry, so it does not show.
- The `"text"` key works for each widget with a `text` property, for example `QTuiLabel`, the buttons and `QTuiLineEdit`.
- A file is application input. A wrong key or a wrong value gives a warning, and the loader continues. See [Limits](#limits).

### Layouts

The `"layout"` key of a widget object makes a layout, fills it and calls `setLayout()`. See [layouts](layouts.md) for the layout classes.

| Key | Value | Description |
| --- | --- | --- |
| `type` | string | `"QTuiVBoxLayout"`, `"QTuiHBoxLayout"`, `"QTuiGridLayout"`, `"QTuiFormLayout"` or `"QTuiStackedLayout"`. It is necessary. |
| `items` | array | The items, in their order in the layout. |
| `contentsMargins` | array | `[left, top, right, bottom]` in cells. |
| `spacing` | int | Box layouts only. The cells between two items. |
| `horizontalSpacing` | int | `QTuiFormLayout` only. The cells between the label column and the field column. |
| `currentIndex` | int | `QTuiStackedLayout` only. The index of the widget that shows. |

An item is one of these objects:

| Item | Description |
| --- | --- |
| a widget object | An object with `"type"`, in the same format as each other widget. It can have its own `"layout"`, `"pages"` and `"children"`. |
| `{ "layout": { ... } }` | A nested layout. A `QTuiStackedLayout` cannot hold one. |
| `{ "spacer": N }` | Box layouts only. `N` empty cells, as `addSpacing()`. |
| `{ "stretch": N }` | Box layouts only. Empty space that takes free space, as `addStretch()`. |

A widget item and a nested layout item can have more keys. The keys are different for each layout type:

| Layout | Item key | Description |
| --- | --- | --- |
| box | `stretch` | The stretch factor of the item. The default is 0. |
| grid | `row`, `column` | The cell of the item. The default is 0. |
| grid | `rowSpan`, `columnSpan` | The number of rows and columns that the item covers. The default is 1. |
| form | `label` | The label of the row: a text, or a widget object. Without it, the row has no label. |

The widget with the layout is the parent of each widget in the layout, also of the widgets in a nested layout. The layout sets the geometry of its widgets, so they need no `"geometry"`.

```json
{
    "type": "QTuiWidget",
    "geometry": { "x": 0, "y": 0, "width": 40, "height": 10 },
    "layout": {
        "type": "QTuiVBoxLayout", "spacing": 1, "contentsMargins": [1, 0, 1, 0],
        "items": [
            { "type": "QTuiLabel", "text": "Notes" },
            { "type": "QTuiPlainTextEdit", "objectName": "notes", "stretch": 1 },
            { "layout": { "type": "QTuiHBoxLayout", "spacing": 1, "items": [
                { "stretch": 1 },
                { "type": "QTuiPushButton", "objectName": "ok", "text": "OK" },
                { "type": "QTuiPushButton", "objectName": "cancel", "text": "Cancel" }
            ] } }
        ]
    }
}
```

### Pages

The `"pages"` key gives widgets to a container with the call of that container. Each entry is an object with a `"widget"` key, which holds a widget object. Some containers read more keys from the entry.

| Container | Call | Entry keys |
| --- | --- | --- |
| `QTuiTabWidget` | `addTab()` | `title`: the text of the tab. |
| `QTuiToolBox` | `addItem()` | `title`: the text of the item. |
| `QTuiStackedWidget` | `addWidget()` | |
| `QTuiSplitter` | `addWidget()` | |
| `QTuiScrollArea` | `setWidget()` | One page only. |
| `QTuiDockWidget` | `setWidget()` | One page only. |
| `QTuiMainWindow` | See below. | `area`: the area of a dock. |

A `QTuiMainWindow` finds the role of a page from the type of the widget:

- A `QTuiMenuBar` is the menu bar and a `QTuiStatusBar` is the status bar. The window holds one of each.
- A `QTuiToolBar` is a tool bar. The window can hold more than one.
- A `QTuiDockWidget` is a dock. The `"area"` key of the entry is `"LeftDockWidgetArea"`, `"RightDockWidgetArea"`, `"TopDockWidgetArea"` or `"BottomDockWidgetArea"`. The default is `"LeftDockWidgetArea"`.
- Each other widget is the central widget. The window holds one.

If a container already holds its one page, a second page gives a warning and the loader deletes it.

The loader sets the `"currentIndex"` property of a container after it adds the pages. A `QTuiGroupBox` uses `"layout"`: it puts the layout inside its border. A `QTuiCollapsible` uses `"children"`: it puts its children in rows itself.

This example has a form in a tab page:

```json
{
    "type": "QTuiMainWindow",
    "geometry": { "x": 0, "y": 0, "width": 80, "height": 24 },
    "pages": [
        { "widget": {
            "type": "QTuiTabWidget", "objectName": "tabs",
            "pages": [
                { "title": "Account", "widget": {
                    "type": "QTuiWidget",
                    "layout": {
                        "type": "QTuiFormLayout", "horizontalSpacing": 1, "contentsMargins": [1, 1, 1, 1],
                        "items": [
                            { "type": "QTuiLineEdit", "objectName": "name", "label": "Name:" },
                            { "type": "QTuiLineEdit", "objectName": "password", "label": "Password:",
                              "echoMode": "Password" },
                            { "type": "QTuiCheckBox", "objectName": "remember", "text": "Remember me" },
                            { "layout": { "type": "QTuiHBoxLayout", "items": [
                                { "stretch": 1 },
                                { "type": "QTuiPushButton", "objectName": "ok", "text": "OK" }
                            ] } }
                        ]
                    }
                } },
                { "title": "Log", "widget": { "type": "QTuiPlainTextEdit", "objectName": "log", "readOnly": true } }
            ]
        } },
        { "widget": { "type": "QTuiStatusBar", "objectName": "status" } }
    ]
}
```

The tab widget is the central widget of the window, so it needs no `"geometry"`. Load the file and call `show()` on the window as in the first example.

### Limits

- The loader makes at most 64 levels of widgets and layouts. A widget and a layout each count as one level. Deeper objects give a warning and the loader skips them.
- A grid position, a span, a spacing, a margin, a spacer and a stretch factor are integers from 0 to 1000 (a span starts at 1). A different value gives a warning. The loader then skips a grid item, and uses the default for the other keys.
- An unknown layout type, an unknown key in a layout, an item that is not an object, and `"pages"` on a type that has no pages give a warning. The loader skips that part and continues.
- The loader does not examine the unknown keys of an entry in `"pages"`, because the adopter of an application type can read its own keys.
- The format has no row and column stretch for a grid layout, no size policy and no minimum, maximum or fixed size. Set them in C++ after the load.
- A `QTuiMenuBar` and a `QTuiToolBar` from a file are empty. Add the menus and the actions in C++.

### Built-in types

`QTuiWidget`, `QTuiFrame`, `QTuiMainWindow`, `QTuiLabel`, `QTuiLineEdit`, `QTuiKeySequenceEdit`, `QTuiTextEdit`, `QTuiPlainTextEdit`, `QTuiPushButton`, `QTuiToolButton`, `QTuiCheckBox`, `QTuiRadioButton`, `QTuiSpinBox`, `QTuiDoubleSpinBox`, `QTuiSlider`, `QTuiDial`, `QTuiComboBox`, `QTuiProgressBar`, `QTuiDateTimeEdit`, `QTuiDateEdit`, `QTuiTimeEdit`, `QTuiLCDNumber`, `QTuiScrollBar`, `QTuiScrollArea`, `QTuiStackedWidget`, `QTuiSplitter`, `QTuiGroupBox`, `QTuiCollapsible`, `QTuiTabBar`, `QTuiTabWidget`, `QTuiToolBox`, `QTuiFocusFrame`, `QTuiSizeGrip`, `QTuiCalendarWidget`, `QTuiHeaderView`, `QTuiTableView`, `QTuiListView`, `QTuiTreeView`, `QTuiColumnView`, `QTuiUndoView`, `QTuiListWidget`, `QTuiTableWidget`, `QTuiTreeWidget`, `QTuiMenu`, `QTuiMenuBar`, `QTuiToolBar`, `QTuiStatusBar`, `QTuiDockWidget`, `QTuiMdiArea`, `QTuiMdiSubWindow`.

Some types start with a fixed value: `QTuiSlider` and `QTuiSplitter` are horizontal, `QTuiScrollBar` is vertical, `QTuiHeaderView` is horizontal, and `QTuiLCDNumber` has 5 digits. The loader does not register the dialogs, because a dialog always attaches to the main window.

## Custom types

Register a factory to use your own widget class in a file. The name in `registerWidget()` is the name for the `"type"` key.

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class Clock : public QTuiWidget {
public:
    using QTuiWidget::QTuiWidget;
};

QTuiWidget *loadDashboard(QTuiWidget *parent)
{
    UI_Loader loader;
    loader.registerWidget(QStringLiteral("Clock"), [](QTuiWidget *p) -> QTuiWidget * {
        return new Clock(p);
    });

    const QByteArray json = R"({
        "type": "QTuiWidget",
        "geometry": { "x": 0, "y": 0, "width": 40, "height": 10 },
        "children": [
            { "type": "Clock", "objectName": "clock", "toolTip": "Local time",
              "geometry": { "x": 1, "y": 1, "width": 8, "height": 1 } }
        ]
    })";
    return loader.loadFromData(json, parent);
}
```

A class without `Q_OBJECT` has only the properties of its nearest base class with `Q_OBJECT`. To set your own properties from JSON, declare them with `Q_PROPERTY` in a class with `Q_OBJECT`.

### Containers

If your type holds pages, give `registerWidget()` an adopter. The loader makes each page as a child of the container, and then calls the adopter with the container, the page and the entry from `"pages"`. The adopter can read its own keys from the entry.

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

class Deck : public QTuiWidget {
public:
    using QTuiWidget::QTuiWidget;
    void addCard(QTuiWidget *card, const QString &suit) { m_cards.append(card); m_suits.append(suit); }

private:
    QList<QTuiWidget *> m_cards;
    QStringList m_suits;
};

void registerDeck(UI_Loader &loader)
{
    loader.registerWidget(QStringLiteral("Deck"),
        [](QTuiWidget *p) -> QTuiWidget * { return new Deck(p); },
        [](QTuiWidget *container, QTuiWidget *page, const QJsonObject &entry) {
            static_cast<Deck *>(container)->addCard(page, entry.value(QStringLiteral("suit")).toString());
        });
}
```

```json
{
    "type": "Deck",
    "pages": [
        { "suit": "hearts", "widget": { "type": "QTuiLabel", "text": "Ace" } },
        { "suit": "spades", "widget": { "type": "QTuiLabel", "text": "King" } }
    ]
}
```

The adopter works in one direction. `serialize()` writes the pages of your type in `"children"`, without the keys of the entry.

## Serialization

`serialize()` writes the same format. For each widget, it writes these keys:

- `type`: the class name without the `QtTUI::` namespace. A subclass without `Q_OBJECT` gets the name of its base class.
- `objectName`: only if it is not empty.
- `geometry`: always, but not for a widget in a layout and not for a page. The layout or the container sets that geometry. The content of a `QTuiScrollArea` has its own size, so it keeps its `geometry`.
- The writable properties that are different from a new widget of the same type. The loader makes the new widget with the factory of the type. For an unregistered type, it writes all writable properties.
- Enum and flag properties as names. The loader does not write values that JSON cannot hold directly, for example `QRect`.
- `visible`: only if it is false. A page of a `QTuiTabWidget`, a `QTuiToolBox` or a `QTuiStackedWidget` and a widget in a `QTuiStackedLayout` do not get it, because the container shows and hides them.
- `layout`: the layout of the widget with its items, if the widget has one of the four built-in layout classes. A form label from a text is a text. A different label is a widget object.
- `pages`: the pages of the built-in containers in [Pages](#pages), in the order of the container, with `title` and `area`.
- `children`: the other child widgets, if there are any. The widgets in the layout, the pages, the widgets that a widget makes itself, the handles of a `QTuiSplitter` and popups are not included.

The keys in the output are in alphabetical order.

`loadFromData(serialize(tree))` gives an equal tree: the same widgets, layouts, items and pages in the same order. These parts are not in the output:

- The row and column stretch and the minimum sizes of a `QTuiGridLayout`, the size policy and the size limits of a widget, the sizes of a `QTuiSplitter`, and each other value that is not a `Q_PROPERTY`.
- A layout of a class that is not one of the four built-in classes. Its widgets are in `"children"` with their geometry.
- The pages of an application type. They are in `"children"`.
- The contents of a menu bar and a tool bar, and the models of the item views.

A `QTuiBoxLayout` hides a widget that does not fit. `serialize()` writes `"visible": false` for that widget, as for a widget that the application hides.

```cpp
#include <QtTUI/QtTUI>
#include <QFile>

using namespace QtTUI;

bool saveForm(const QTuiWidget *root, const QString &filePath)
{
    UI_Loader loader;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(loader.serialize(root)) >= 0;
}
```

`children` does not include the widgets that a widget makes itself. For example, a `QTuiTabWidget` makes a `QTuiTabBar`, and a `QTuiScrollArea` makes two `QTuiScrollBar` widgets. A new widget of the same type has these children too, so the loader makes them again. `serialize()` finds them by type: it skips the first children of a widget, one for each child of a new widget of that type.
