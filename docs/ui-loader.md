# UI loader

This page covers `UI_Loader`, which makes a widget tree from a JSON description and writes a widget tree back to JSON.

## UI_Loader

`#include <QtTUI/UI_Loader.h>` · Inherits: `QObject`

The loader makes widgets from JSON. It does not read Qt Designer `.ui` files (XML), and it is not the same as `QUiLoader`. The format has no layouts: each widget gets its position from `"geometry"`.

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
| `void registerWidget(const QString &typeName, WidgetFactory factory)` | Adds a type for the `"type"` key, or replaces the factory of a type. |
| `QStringList availableWidgets() const` | Returns the registered type names, in no specified order. |

`WidgetFactory` is `std::function<QTuiWidget *(QTuiWidget *parent)>`. The factory makes a widget with the given parent. If the factory returns `nullptr`, the loader shows a warning and skips the object and its children.

## JSON format

Each object in the file describes one widget.

| Key | Value | Description |
| --- | --- | --- |
| `type` | string | The registered type name, for example `"QTuiPushButton"`. It is necessary. |
| `objectName` | string | Sets `QObject::objectName`. Use it with `findChild()`. |
| `geometry` | object | `{ "x", "y", "width", "height" }` in screen cells. A missing field is 0. |
| `visible` | bool | Calls `setVisible()`. |
| `children` | array | The child widgets. The loader makes each child with this widget as its parent. The loader ignores values that are not objects. |
| any other key | any | Sets the writable `Q_PROPERTY` with that name. |

Property keys:

- Each key that is not in the table above is a `Q_PROPERTY` name, for example `"text"`, `"checked"`, `"readOnly"`, `"placeholderText"` or `"enabled"`. The loader converts the JSON value to the property type.
- For an enum property, give the name of the value as a string, for example `"echoMode": "Password"`. For a flag property, join the names with `|`, for example `"alignment": "AlignRight|AlignVCenter"`.
- An unknown or read-only property, an unknown enum name, or a value that does not convert gives a warning. The loader skips that key and continues.
- The order of the keys in an object has no effect on the result.

Other rules:

- An object without `"type"`, or with an unknown type, gives a warning. The loader skips it and all its children.
- A child is not added to a layout or to a container page. For example, a child of a `QTuiTabWidget` does not become a tab. Use C++ for layouts and pages (see [widgets](widget.md)).
- A widget without `"geometry"` has an empty geometry, so it does not show.
- The `"text"` key works for each widget with a `text` property, for example `QTuiLabel`, the buttons and `QTuiLineEdit`.

### Built-in types

`QTuiWidget`, `QTuiFrame`, `QTuiMainWindow`, `QTuiLabel`, `QTuiLineEdit`, `QTuiTextEdit`, `QTuiPlainTextEdit`, `QTuiPushButton`, `QTuiToolButton`, `QTuiCheckBox`, `QTuiRadioButton`, `QTuiSpinBox`, `QTuiDoubleSpinBox`, `QTuiSlider`, `QTuiDial`, `QTuiComboBox`, `QTuiProgressBar`, `QTuiDateTimeEdit`, `QTuiDateEdit`, `QTuiTimeEdit`, `QTuiLCDNumber`, `QTuiScrollBar`, `QTuiScrollArea`, `QTuiStackedWidget`, `QTuiSplitter`, `QTuiGroupBox`, `QTuiCollapsible`, `QTuiTabBar`, `QTuiTabWidget`, `QTuiToolBox`, `QTuiFocusFrame`, `QTuiSizeGrip`, `QTuiCalendarWidget`, `QTuiHeaderView`, `QTuiTableView`, `QTuiListView`, `QTuiTreeView`, `QTuiColumnView`, `QTuiUndoView`, `QTuiListWidget`, `QTuiTableWidget`, `QTuiTreeWidget`, `QTuiMenu`, `QTuiMenuBar`, `QTuiToolBar`, `QTuiStatusBar`, `QTuiDockWidget`, `QTuiMdiArea`, `QTuiMdiSubWindow`.

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

## Serialization

`serialize()` writes the same format. For each widget, it writes these keys:

- `type`: the class name without the `QtTUI::` namespace. A subclass without `Q_OBJECT` gets the name of its base class.
- `objectName`: only if it is not empty.
- `geometry`: always.
- The writable properties that are different from a new widget of the same type. The loader makes the new widget with the factory of the type. For an unregistered type, it writes all writable properties.
- Enum and flag properties as names. The loader does not write values that JSON cannot hold directly, for example `QRect`.
- `visible`: only if it is false.
- `children`: all child widgets, if the widget has children.

The keys in the output are in alphabetical order.

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

`children` includes the widgets that a widget makes itself. For example, a `QTuiTabWidget` makes a `QTuiTabBar`, and a `QTuiScrollArea` makes two `QTuiScrollBar` widgets. If you load the result again, the loader makes these widgets a second time as extra children. A round trip is correct only for widgets without such internal children.
