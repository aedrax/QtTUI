# Layouts

This page covers the layout classes, the layout items and the size policy. A layout sets the geometry of widgets in an area. All sizes, margins and spacings are in character cells, not pixels.

Set a layout on a widget with `QTuiWidget::setLayout()` (see [QTuiWidget](widget.md)). The widget then becomes the parent of the layout and applies the layout to its full geometry each time the geometry changes. A layout does not change the parent of the widgets in it. Make each widget a child of the widget that has the layout, else the widget does not paint. Some containers, for example [QTuiGroupBox](containers.md#qtuigroupbox), apply the layout to the area inside their border.

## QTuiLayout

`#include <QtTUI/QTuiLayout.h>` · Inherits: QObject

QTuiLayout is the abstract base class of all layouts. A subclass implements `apply()`, which sets the geometry of the items in an area. Unlike Qt, a layout gets its area through `apply()`, not through `setGeometry()`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

// A layout that gives each widget one row, from the top
class RowLayout : public QTuiLayout {
public:
    using QTuiLayout::QTuiLayout;

    void apply(const QRect &availableArea) override
    {
        recordAppliedRect(availableArea);
        const QRect area = contentsRect(availableArea);
        for (int i = 0; i < count(); ++i)
            itemAt(i)->setGeometry(QRect(area.x(), area.y() + i, area.width(), 1));
    }
};

void example(QTuiWidget *parent)
{
    auto *layout = new RowLayout;
    layout->setContentsMargins(1, 1, 1, 1);
    layout->addWidget(new QTuiLabel(QStringLiteral("One"), parent));
    layout->addWidget(new QTuiLabel(QStringLiteral("Two"), parent));
    parent->setLayout(layout);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiLayout(QObject *parent = nullptr)` | Makes an empty layout. |
| `~QTuiLayout()` | Deletes the items of the base item list. It does not delete the widgets. |
| `void apply(const QRect &availableArea)` | Pure virtual. Sets the geometry of the items in `availableArea`. |
| `void addItem(QTuiLayoutItem *item)` | Adds an item to the base item list. The layout owns the item. |
| `void addWidget(QTuiWidget *widget)` | Adds a widget to the base item list in a [QTuiWidgetItem](#qtuiwidgetitem). If the widget is destroyed, the layout removes its item. |
| `QTuiLayoutItem *itemAt(int index) const` | Virtual. Returns the item at `index` in the base item list, or nullptr. |
| `int count() const` | Virtual. Returns the number of items. Each subclass returns the number of its own entries. |
| `QSize minimumSize() const` | Virtual. Returns the minimum size of the contents plus the margins. The base class uses the largest minimum size of the non-empty items. |
| `void invalidate()` | Applies the layout again to the last area, on the next event loop iteration. Several calls make one pass. Does nothing if the layout was never applied, or if there is no `QCoreApplication`. |
| `void setContentsMargins(int left, int top, int right, int bottom)` | Sets the empty cells around the contents. A negative value becomes 0. The default is 0 on all sides. |
| `void contentsMargins(int *left, int *top, int *right, int *bottom) const` | Writes the margins into the pointers that are not nullptr. |
| `QRect contentsRect(const QRect &availableArea) const` | Protected. Returns `availableArea` without the margins. |
| `QRect visualRect(const QRect &rect, const QRect &area) const` | Protected. Returns `rect` mirrored horizontally in `area` if the widget of the layout is right-to-left. Else it returns `rect`. The widget is the nearest `QTuiWidget` among the parents. Without one, the layout direction of the application is used. |
| `void recordAppliedRect(const QRect &availableArea)` | Protected. Keeps the area for `invalidate()`. Call it first in each `apply()`. |

## QTuiLayoutItem

`#include <QtTUI/QTuiLayoutItem.h>` · Inherits: none

QTuiLayoutItem is the abstract base class of the items that a layout arranges. QTuiLayoutItem is not a QObject. It keeps its own `QTuiSizePolicy`. That policy is not the size policy of a widget.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

// An item that always wants 10 x 1 cells
class FixedItem : public QTuiLayoutItem {
public:
    QSize sizeHint() const override { return QSize(10, 1); }
    QSize minimumSize() const override { return QSize(10, 1); }
    QSize maximumSize() const override { return QSize(10, 1); }
    void setGeometry(const QRect &rect) override { m_rect = rect; }
    QRect geometry() const override { return m_rect; }
    bool isEmpty() const override { return false; }

private:
    QRect m_rect;
};
```

### Functions

| Function | Description |
| --- | --- |
| `QSize sizeHint() const` | Pure virtual. Returns the preferred size. |
| `QSize minimumSize() const` | Pure virtual. Returns the minimum size. |
| `QSize maximumSize() const` | Pure virtual. Returns the maximum size. |
| `void setGeometry(const QRect &rect)` | Pure virtual. Sets the geometry of the item. |
| `QRect geometry() const` | Pure virtual. Returns the geometry of the item. |
| `bool isEmpty() const` | Pure virtual. Returns true if the item takes no space. |
| `QTuiSizePolicy sizePolicy() const` | Returns the size policy of the item. |
| `void setSizePolicy(const QTuiSizePolicy &policy)` | Sets the size policy of the item. |

## QTuiWidgetItem

`#include <QtTUI/QTuiLayoutItem.h>` · Inherits: QTuiLayoutItem

QTuiWidgetItem holds one widget in a layout. It keeps a guarded pointer, so it becomes empty if the widget is destroyed.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *label = new QTuiLabel(QStringLiteral("Name"), parent);
    QTuiWidgetItem item(label);
    item.setGeometry(QRect(0, 0, 10, 1)); // calls label->setGeometry()
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiWidgetItem(QTuiWidget *widget)` | Makes an item for `widget`. |
| `QSize sizeHint() const` | Returns the size hint of the widget, at least its minimum size. |
| `QSize minimumSize() const` | Returns the minimum size of the widget. |
| `QSize maximumSize() const` | Returns 16777215 x 16777215. |
| `void setGeometry(const QRect &rect)` | Sets the geometry of the widget. |
| `QRect geometry() const` | Returns the geometry of the widget. |
| `bool isEmpty() const` | Returns true if the widget is destroyed or hidden. |
| `QTuiWidget *widget() const` | Returns the widget, or nullptr if it is destroyed. |

## QTuiSpacerItem

`#include <QtTUI/QTuiLayoutItem.h>` · Inherits: QTuiLayoutItem

QTuiSpacerItem is empty space in a layout. Its policies set how its minimum and maximum sizes follow the width and height that you give.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

QSize example()
{
    // Minimum size 0 x 2, the maximum size is not limited
    QTuiSpacerItem spacer(0, 2, QTuiSizePolicy::Minimum, QTuiSizePolicy::Expanding);
    return spacer.minimumSize();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiSpacerItem(int width, int height, QTuiSizePolicy::Policy hPolicy = QTuiSizePolicy::Minimum, QTuiSizePolicy::Policy vPolicy = QTuiSizePolicy::Minimum)` | Makes a spacer with a preferred size and a policy for each direction. |
| `QSize sizeHint() const` | Returns (width, height). |
| `QSize minimumSize() const` | Returns the width (or height) for `Fixed`, `Minimum`, `Expanding` and `MinimumExpanding`. Returns 0 for the other policies. |
| `QSize maximumSize() const` | Returns the width (or height) for `Fixed`. Returns 16777215 for the other policies. |
| `void setGeometry(const QRect &rect)` | Keeps the geometry. |
| `QRect geometry() const` | Returns the geometry. |
| `bool isEmpty() const` | Returns false. |

## QTuiLayoutItemWrapper

`#include <QtTUI/QTuiLayoutItem.h>` · Inherits: QTuiLayoutItem

QTuiLayoutItemWrapper holds a nested layout as an item. `QTuiGridLayout::addLayout()` uses it. It keeps a guarded pointer to the layout.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *row = createHBoxLayout(parent);
    row->addWidget(new QTuiPushButton(QStringLiteral("OK"), parent));
    QTuiLayoutItemWrapper item(row);
    item.setGeometry(QRect(0, 0, 20, 1)); // calls row->apply()
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiLayoutItemWrapper(QTuiLayout *layout)` | Makes an item for `layout`. |
| `QSize sizeHint() const` | Returns the minimum size of the layout. |
| `QSize minimumSize() const` | Returns the minimum size of the layout. |
| `QSize maximumSize() const` | Returns 16777215 x 16777215. |
| `void setGeometry(const QRect &rect)` | Keeps the geometry and calls `apply(rect)` on the layout. |
| `QRect geometry() const` | Returns the geometry. |
| `bool isEmpty() const` | Returns true if the layout is destroyed or has no items. |
| `QTuiLayout *layout() const` | Returns the layout, or nullptr if it is destroyed. |

## QTuiBoxLayout

`#include <QtTUI/QTuiBoxLayout.h>` · Inherits: QTuiLayout

QTuiBoxLayout puts widgets in one column (`TopToBottom`) or one row (`LeftToRight`). `QTuiVBoxLayout` and `QTuiHBoxLayout` are aliases of the same class, so the constructor always takes a direction. Use `createVBoxLayout()` or `createHBoxLayout()` to make one. Unlike Qt, a box layout cannot hold a nested layout. Use a container widget with its own layout, or a [QTuiGridLayout](#qtuigridlayout).

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *layout = createVBoxLayout(&window);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(1);
    layout->addWidget(new QTuiLabel(QStringLiteral("Notes:"), &window));
    layout->addWidget(new QTuiTextEdit(&window), 1); // takes the free rows
    layout->addStretch();
    layout->addWidget(new QTuiPushButton(QStringLiteral("Save"), &window));
    window.setLayout(layout);

    window.show();
    app.triggerRender();
    return app.exec();
}
```

### Enums

| Enum | Values |
| --- | --- |
| `Direction` | `TopToBottom`: one column. `LeftToRight`: one row. |

### Functions

| Function | Description |
| --- | --- |
| `QTuiBoxLayout(Direction direction, QObject *parent = nullptr)` | Makes an empty box layout. |
| `void addWidget(QTuiWidget *widget, int stretch = 0)` | Adds a widget with a stretch factor. A nullptr, a widget that is already in the layout, and a negative stretch give a warning. A negative stretch becomes 0. |
| `void removeWidget(QTuiWidget *widget)` | Removes a widget from the layout. It does not delete or hide the widget. |
| `void addSpacing(int size)` | Adds empty space of `size` cells. |
| `void addStretch(int stretch = 0)` | Adds empty space that takes free space by its stretch factor. A stretch of 0 becomes 1, so it takes free space, as in Qt. |
| `int spacing() const` | Returns the cells between two items that follow each other. The default is 0. |
| `void setSpacing(int spacing)` | Sets the spacing. A negative value becomes 0. |
| `void apply(const QRect &availableArea)` | Sets the geometry of the widgets. See "How the space is divided". |
| `Direction direction() const` | Returns the direction. |
| `int count() const` | Returns the number of entries, with the spacings and stretches. |
| `QSize minimumSize() const` | Returns the sum of the minimum sizes and spacings along the direction, the largest minimum size across it, and the margins. |
| `const QList<Item> &items() const` | Returns the entries of the layout. Use this function, not `itemAt()`. |

The struct `QTuiBoxLayout::Item` has these fields:

| Field | Description |
| --- | --- |
| `QTuiWidget *widget` | The widget, or nullptr for an entry from `addSpacing()` or `addStretch()`. |
| `int stretch` | The stretch factor. |
| `int spacerSize` | The cells of an entry from `addSpacing()`. 0 for the other entries. |

These free functions and aliases are in the same header:

| Name | Description |
| --- | --- |
| `QTuiBoxLayout *createVBoxLayout(QObject *parent = nullptr)` | Returns `new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, parent)`. |
| `QTuiBoxLayout *createHBoxLayout(QObject *parent = nullptr)` | Returns `new QTuiBoxLayout(QTuiBoxLayout::LeftToRight, parent)`. |
| `QTuiVBoxLayout`, `QTuiHBoxLayout` | Aliases of `QTuiBoxLayout`. |

### How the space is divided

1. The layout skips each widget that you hid. It keeps no space for it and does not show it again.
2. An item gets stretch 0 if the size policy of its widget along the direction is `Fixed`. An `Expanding` or `MinimumExpanding` widget with stretch 0 gets stretch 1.
3. Each item with stretch 0 gets its size hint, if all of these size hints fit. Else it gets its minimum size.
4. The free space goes to the items with a stretch, in proportion to the stretch. An item never gets less than its minimum size.
5. Across the direction, a widget fills the area. A widget with the policy `Fixed` or `Maximum` across the direction gets its size hint (at least its minimum size).
6. If a widget does not fit, the layout hides it. The layout shows the widget again when there is space.

If the layout direction is right-to-left, the layout mirrors the positions horizontally.

`QTuiBoxLayout` does not override `itemAt()`, so `itemAt()` returns nullptr for each index. `apply()` does not use the items from the base `addItem()`.

## QTuiGridLayout

`#include <QtTUI/QTuiGridLayout.h>` · Inherits: QTuiLayout

QTuiGridLayout puts items in rows and columns. An item can span more than one row or column. A grid layout can hold nested layouts. The grid does not use the size policy of the widgets.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *grid = new QTuiGridLayout;
    grid->addWidget(new QTuiLabel(QStringLiteral("Host:"), parent), 0, 0);
    grid->addWidget(new QTuiLineEdit(parent), 0, 1);
    grid->addWidget(new QTuiLabel(QStringLiteral("Port:"), parent), 1, 0);
    grid->addWidget(new QTuiSpinBox(parent), 1, 1);

    auto *buttons = createHBoxLayout(grid);
    buttons->addStretch();
    buttons->addWidget(new QTuiPushButton(QStringLiteral("Connect"), parent));
    grid->addLayout(buttons, 2, 0, 1, 2);

    grid->setColumnMinimumWidth(0, 6);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(2, 1);
    parent->setLayout(grid);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiGridLayout(QObject *parent = nullptr)` | Makes an empty grid layout. |
| `~QTuiGridLayout()` | Deletes the items. It does not delete the widgets or the nested layouts. |
| `void addWidget(QTuiWidget *widget, int row, int column, int rowSpan = 1, int columnSpan = 1)` | Adds a widget in a cell. A span less than 1 becomes 1. A nullptr or a negative row or column does nothing. |
| `void removeWidget(QTuiWidget *widget)` | Removes a widget from the grid. It does not delete the widget. |
| `void addLayout(QTuiLayout *layout, int row, int column, int rowSpan = 1, int columnSpan = 1)` | Adds a nested layout in a cell, in a [QTuiLayoutItemWrapper](#qtuilayoutitemwrapper). |
| `void setRowStretch(int row, int stretch)` | Sets the stretch factor of a row. |
| `void setColumnStretch(int column, int stretch)` | Sets the stretch factor of a column. |
| `void setRowMinimumHeight(int row, int height)` | Sets the minimum height of a row. |
| `void setColumnMinimumWidth(int column, int width)` | Sets the minimum width of a column. |
| `int rowCount() const` | Returns the number of rows, including the rows with only a stretch or a minimum height. |
| `int columnCount() const` | Returns the number of columns, including the columns with only a stretch or a minimum width. |
| `int count() const` | Returns the number of items. |
| `QTuiLayoutItem *itemAt(int index) const` | Returns the item at `index`, or nullptr. |
| `QSize minimumSize() const` | Returns the sum of the row and column minimum sizes plus the margins. |
| `void apply(const QRect &availableArea)` | Sets the geometry of the items. See below. |

Each row gets the largest minimum height of its items that span one row, or the minimum height that you set. Then the free space goes to the rows with a stretch factor, in proportion to it. If no row has a stretch factor, all rows get equal parts of the free space. The columns work in the same way. Items that span more than one row or column do not change the minimum sizes.

## QTuiFormLayout

`#include <QtTUI/QTuiFormLayout.h>` · Inherits: QTuiLayout

QTuiFormLayout puts a label and a field in each row. The label column is as wide as the widest label. The field column gets the remaining width. Each row is as tall as the minimum height of its label or field (at least 1 row).

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *form = new QTuiFormLayout;
    form->setHorizontalSpacing(1);
    form->addRow(QStringLiteral("Name:"), new QTuiLineEdit(parent));
    form->addRow(QStringLiteral("Age:"), new QTuiSpinBox(parent));
    form->addRow(new QTuiLabel(QStringLiteral("Notes:"), parent), new QTuiTextEdit(parent));
    parent->setLayout(form);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiFormLayout(QObject *parent = nullptr)` | Makes an empty form layout. |
| `~QTuiFormLayout()` | Deletes the labels that the layout made. It does not delete the fields. |
| `void addRow(const QString &labelText, QTuiWidget *field)` | Makes a `QTuiLabel` with `labelText` and adds a row. The label gets the same parent as the field. If the field has no parent yet, `apply()` sets the parent of the label later. |
| `void addRow(QTuiWidget *label, QTuiWidget *field)` | Adds a row with your label widget. The layout does not own the label. |
| `void removeRow(int row)` | Removes a row. It deletes the label if the layout made it. It does not delete the field. |
| `int rowCount() const` | Returns the number of rows. |
| `int horizontalSpacing() const` | Returns the cells between the label column and the field column. The default is 0. |
| `void setHorizontalSpacing(int spacing)` | Sets the horizontal spacing. A negative value becomes 0. |
| `int count() const` | Returns the number of rows. |
| `QSize minimumSize() const` | Returns the widest label plus the spacing plus the widest field, the sum of the row heights, and the margins. |
| `void apply(const QRect &availableArea)` | Sets the geometry of the labels and fields. The field column always keeps at least 1 cell. |

If a field is destroyed, the layout removes its row. The form layout does not use the size policy of the widgets. `QTuiFormLayout` does not override `itemAt()`, so `itemAt()` returns nullptr for each index.

## QTuiStackedLayout

`#include <QtTUI/QTuiStackedLayout.h>` · Inherits: QTuiLayout

QTuiStackedLayout shows one of its widgets at a time. The current widget fills the area without the margins. The layout hides the other widgets. For a widget with the same behavior, see [QTuiStackedWidget](containers.md#qtuistackedwidget).

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *stack = new QTuiStackedLayout;
    stack->addWidget(new QTuiLabel(QStringLiteral("Page 1"), parent));
    stack->addWidget(new QTuiLabel(QStringLiteral("Page 2"), parent));
    parent->setLayout(stack);

    QObject::connect(stack, &QTuiStackedLayout::currentChanged, [](int index) {
        qDebug() << "page" << index;
    });
    stack->setCurrentIndex(1);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiStackedLayout(QObject *parent = nullptr)` | Makes an empty stacked layout. |
| `int addWidget(QTuiWidget *widget)` | Adds a widget and returns its index. The first widget becomes current and shows. The layout hides the other widgets. Returns -1 for nullptr. |
| `QTuiWidget *widget(int index) const` | Returns the widget at `index`, or nullptr. |
| `int count() const` | Returns the number of widgets. |
| `QSize minimumSize() const` | Returns the largest minimum size of all widgets, plus the margins. |
| `int currentIndex() const` | Returns the current index, or -1. Property: `currentIndex`. |
| `void setCurrentIndex(int index)` | Hides the old widget and shows the widget at `index`. The layout applies the last area again, so the new widget gets the current geometry. |
| `QTuiWidget *currentWidget() const` | Returns the current widget, or nullptr. |
| `void setCurrentWidget(QTuiWidget *widget)` | Makes `widget` current if it is in the layout. |
| `void apply(const QRect &availableArea)` | Gives the current widget the area without the margins and hides the other widgets. |

If the current widget is destroyed, the next widget at that index (or the last widget) becomes current. `QTuiStackedLayout` does not override `itemAt()`, so `itemAt()` returns nullptr for each index. Use `widget()`.

### Signals

| Signal | Description |
| --- | --- |
| `currentChanged(int index)` | The current widget changed. |

## QTuiSizePolicy

`#include <QtTUI/QTuiSizePolicy.h>` · Inherits: none

QTuiSizePolicy tells a layout how a widget can grow or shrink in each direction. Set it with `QTuiWidget::setSizePolicy()`. Only [QTuiBoxLayout](#qtuiboxlayout) reads the size policy of a widget. The grid, form and stacked layouts do not use it.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *button = new QTuiPushButton(QStringLiteral("OK"), parent);
    // Keep the size hint across a vertical box layout
    button->setSizePolicy(QTuiSizePolicy::Fixed, QTuiSizePolicy::Preferred);

    QTuiSizePolicy policy(QTuiSizePolicy::Expanding, QTuiSizePolicy::Expanding);
    policy.setVerticalStretch(2);
    Q_UNUSED(policy);
}
```

### Enums

| Value | Number | Effect in QTuiBoxLayout |
| --- | --- | --- |
| `Fixed` | 0 | Along the direction: the widget never grows (stretch 0). Across: the widget gets its size hint. |
| `Minimum` | 1 | No special effect. |
| `Maximum` | 2 | Across the direction: the widget gets its size hint. |
| `Preferred` | 3 | The default. No special effect. |
| `Expanding` | 4 | Along the direction: stretch 0 becomes stretch 1. |
| `MinimumExpanding` | 5 | Along the direction: stretch 0 becomes stretch 1. |
| `Ignored` | 6 | No special effect. |

`QTuiSpacerItem` also reads the policies for its minimum and maximum size. See [QTuiSpacerItem](#qtuispaceritem).

### Functions

| Function | Description |
| --- | --- |
| `QTuiSizePolicy()` | Makes a policy with `Preferred` in both directions and stretch 0. |
| `QTuiSizePolicy(Policy horizontal, Policy vertical)` | Makes a policy with the two policies and stretch 0. |
| `Policy horizontalPolicy() const` | Returns the horizontal policy. |
| `void setHorizontalPolicy(Policy policy)` | Sets the horizontal policy. |
| `Policy verticalPolicy() const` | Returns the vertical policy. |
| `void setVerticalPolicy(Policy policy)` | Sets the vertical policy. |
| `int horizontalStretch() const` | Returns the horizontal stretch. |
| `void setHorizontalStretch(int stretch)` | Sets the horizontal stretch, limited to 0 to 255. |
| `int verticalStretch() const` | Returns the vertical stretch. |
| `void setVerticalStretch(int stretch)` | Sets the vertical stretch, limited to 0 to 255. |

No layout reads the stretch values of a size policy. Give the stretch to `QTuiBoxLayout::addWidget()` or `QTuiGridLayout::setRowStretch()` instead.
