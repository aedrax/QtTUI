# Containers

This page covers the widgets that hold other widgets: frames, group boxes, tab widgets, stacks, splitters, scroll areas, MDI windows, dock widgets and the size grip. All sizes and positions are in character cells. For the base class, see [QTuiWidget](widget.md). For layouts inside containers, see [layouts](layouts.md).

## QTuiFrame

`#include <QtTUI/QTuiFrame.h>` · Inherits: QTuiWidget

QTuiFrame is the base class of widgets that can have a frame of one cell on each side. QTuiFrame itself does not paint the frame. A subclass uses `frameWidth()` and `contentsRect()` to find its contents area.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *frame = new QTuiFrame(parent);
    frame->setGeometry(QRect(0, 0, 20, 5));
    frame->setFrameShape(QTuiFrame::Box);
    const QRect inside = frame->contentsRect(); // QRect(1, 1, 18, 3)
    Q_UNUSED(inside);
}
```

### Enums

| Enum | Values |
| --- | --- |
| `Shape` | `NoFrame` (default): no frame. `Box`: a frame of 1 cell on each side. |

### Functions

| Function | Description |
| --- | --- |
| `QTuiFrame(QTuiWidget *parent = nullptr)` | Makes a frame with the shape `NoFrame`. |
| `Shape frameShape() const` | Returns the frame shape. Property: `frameShape`. |
| `void setFrameShape(Shape shape)` | Sets the frame shape and marks the widget dirty. |
| `int frameWidth() const` | Returns the cells of the frame on each side: 1 for `Box`, 0 for `NoFrame`. |
| `QRect contentsRect() const` | Returns the geometry without the frame. |

## QTuiGroupBox

`#include <QtTUI/QTuiGroupBox.h>` · Inherits: QTuiWidget

QTuiGroupBox draws a border with a title in the top border. The children go in the area inside the border. A checkable group box shows `[x]` or `[ ]` before the title. If you clear the check, the group box disables its children. The children stay visible, as in Qt.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *box = new QTuiGroupBox(QStringLiteral("Options"), parent);
    box->setCheckable(true);

    auto *layout = createVBoxLayout();
    layout->addWidget(new QTuiCheckBox(QStringLiteral("Wrap lines"), box));
    layout->addWidget(new QTuiCheckBox(QStringLiteral("Show tabs"), box));
    box->setLayout(layout);

    QObject::connect(box, &QTuiGroupBox::toggled, [](bool checked) {
        qDebug() << "options enabled:" << checked;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiGroupBox(const QString &title = QString(), QTuiWidget *parent = nullptr)` | Makes a group box with a title. |
| `QString title() const` | Returns the title. Property: `title`. |
| `void setTitle(const QString &title)` | Sets the title. |
| `bool isCheckable() const` | Returns true if the group box has a check box. Property: `checkable`. The default is false. |
| `void setCheckable(bool checkable)` | Adds or removes the check box. A checkable group box takes the focus (`Qt::StrongFocus`). A plain group box has `Qt::NoFocus`, so Tab skips it. |
| `bool isChecked() const` | Returns the check state. Property: `checked`. The default is true. |
| `void setChecked(bool checked)` | Sets the check state. If the group box is checkable, this enables or disables all children. Emits `toggled()`. |
| `void setLayout(QTuiLayout *layout)` | Sets the layout and makes the group box its parent. The group box applies the layout to `contentArea()` when it paints. |
| `QTuiLayout *layout() const` | Returns the layout, or nullptr. |
| `QRect contentArea() const` | Returns the area inside the border (1 cell on each side). Returns an empty rect if the group box is smaller than 3 x 3 cells. |
| `QSize minimumSize() const` | Returns the space for the border and the title, and the minimum size of the layout plus 2 cells on each axis. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the border and the title, applies the layout, and paints the children. It clips the child geometries to the content area. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |

### Signals

| Signal | Description |
| --- | --- |
| `toggled(bool checked)` | The check state changed. |

### Keys

| Key | Action |
| --- | --- |
| Space, Enter | Toggles the check state (only if the group box is checkable). |

### Mouse

- A click on the `[x] Title` text in the top border toggles the check state (only if the group box is checkable).

## QTuiCollapsible

`#include <QtTUI/QTuiCollapsible.h>` · Inherits: QTuiWidget

QTuiCollapsible has a title row with `▼` (expanded) or `▶` (collapsed). When it is expanded, it shows its children in rows below the title. Each child gets its minimum height, and the last child gets the remaining rows. Qt has no equivalent class.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *section = new QTuiCollapsible(QStringLiteral("Details"), parent);
    new QTuiLabel(QStringLiteral("Size: 4 KB"), section);
    new QTuiLabel(QStringLiteral("Owner: root"), section);
    section->setCollapsed(true);

    QObject::connect(section, &QTuiCollapsible::toggled, [](bool expanded) {
        qDebug() << "expanded:" << expanded;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiCollapsible(const QString &title = QString(), QTuiWidget *parent = nullptr)` | Makes an expanded collapsible section. It takes the focus (`Qt::StrongFocus`). |
| `QString title() const` | Returns the title. Property: `title`. |
| `void setTitle(const QString &title)` | Sets the title. |
| `bool isCollapsed() const` | Returns true if the section is collapsed. Property: `collapsed`. The default is false. |
| `void setCollapsed(bool collapsed)` | Collapses or expands the section. It hides or shows all children and emits `toggled()`. |
| `bool isExpanded() const` | Returns `!isCollapsed()`. |
| `void setExpanded(bool expanded)` | Calls `setCollapsed(!expanded)`. |
| `QSize minimumSize() const` | Collapsed: 1 row. Expanded: the title row plus the sum of the child minimum heights (at least 1 row). |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the title row, sets the child geometries and paints the children. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |

### Signals

| Signal | Description |
| --- | --- |
| `toggled(bool expanded)` | The state changed. The argument is the expanded state, not the collapsed state. |

### Keys

| Key | Action |
| --- | --- |
| Space, Enter | Collapses or expands the section. |

### Mouse

- A click on the title row collapses or expands the section.

## QTuiFocusFrame

`#include <QtTUI/QTuiFocusFrame.h>` · Inherits: QTuiWidget

QTuiFocusFrame draws a border 1 cell outside a target widget while the target has the focus. It follows the geometry of the target. Put the focus frame in the same parent as the target, with space around the target for the border.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *edit = new QTuiLineEdit(parent);
    edit->setGeometry(QRect(2, 2, 20, 1));

    auto *frame = new QTuiFocusFrame(parent);
    frame->setWidget(edit); // the border shows at QRect(1, 1, 22, 3)
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiFocusFrame(QTuiWidget *parent = nullptr)` | Makes a hidden focus frame. |
| `void setWidget(QTuiWidget *widget)` | Sets the target widget. The frame shows only while the target has the focus. Use nullptr to remove the target. |
| `QTuiWidget *widget() const` | Returns the target widget, or nullptr. |
| `void paintEvent(ScreenBuffer &buffer)` | Draws the border in the focus color of the theme. |

## QTuiTabWidget

`#include <QtTUI/QTuiTabWidget.h>` · Inherits: QTuiWidget

QTuiTabWidget shows a row of tabs, a separator row and one page below them. The first row is a [QTuiTabBar](#qtuitabbar) and the pages start on row 3. The tab bar takes the keys and the mouse clicks.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *tabs = new QTuiTabWidget(parent);
    tabs->addTab(new QTuiLabel(QStringLiteral("General settings")), QStringLiteral("General"));
    tabs->addTab(new QTuiLabel(QStringLiteral("Network settings")), QStringLiteral("Network"));
    tabs->setCurrentIndex(1);

    QObject::connect(tabs, &QTuiTabWidget::currentChanged, [](int index) {
        qDebug() << "tab" << index;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTabWidget(QTuiWidget *parent = nullptr)` | Makes an empty tab widget with its tab bar. |
| `int addTab(QTuiWidget *page, const QString &label)` | Adds a page with a tab label and makes the tab widget its parent. Returns the index, or -1 if `page` is nullptr. The first page becomes current. |
| `void removeTab(int index)` | Removes the page at `index` from the tab widget and hides it. It does not delete the page. |
| `int count() const` | Returns the number of pages. |
| `QTuiWidget *widget(int index) const` | Returns the page at `index`, or nullptr. |
| `int indexOf(QTuiWidget *widget) const` | Returns the index of a page, or -1. |
| `int currentIndex() const` | Returns the index of the current page, or -1. Property: `currentIndex`. |
| `void setCurrentIndex(int index)` | Shows the page at `index`. An index out of range does nothing. |
| `QTuiWidget *currentWidget() const` | Returns the current page, or nullptr. |
| `QTuiTabBar *tabBar() const` | Returns the tab bar. |
| `QRect contentArea() const` | Returns the page area: the geometry without the first 2 rows. Returns an empty rect if the height is less than 3. |
| `QSize minimumSize() const` | Returns the width of the tab bar or of the widest page, and 2 rows plus the height of the tallest page (at least 1). |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the tab bar, the separator and the current page. |
| `void resizeEvent(TuiResizeEvent *event)` | Protected. Sets the geometry of the tab bar and the pages again. |

### Signals

| Signal | Description |
| --- | --- |
| `currentChanged(int index)` | The current page changed. |

### Keys

The keys go to the tab bar. See [QTuiTabBar](#qtuitabbar).

## QTuiTabBar

`#include <QtTUI/QTuiTabBar.h>` · Inherits: QTuiWidget

QTuiTabBar shows tab labels in one row, as `Tab1 | Tab2 | Tab3`. The current tab uses the focus colors in bold. If the labels are wider than the bar, the bar scrolls to keep the current tab visible and shows `‹` and `›` at the edges.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *bar = new QTuiTabBar(parent);
    bar->setGeometry(QRect(0, 0, 40, 1));
    bar->addTab(QStringLiteral("Files"));
    bar->addTab(QStringLiteral("Search"));
    bar->setTabText(1, QStringLiteral("Find"));

    QObject::connect(bar, &QTuiTabBar::currentChanged, [bar](int index) {
        qDebug() << "current tab:" << bar->tabText(index);
    });
}
```

### Enums

| Enum | Values |
| --- | --- |
| `TabPosition` | `North` (default), `South`. |

### Functions

| Function | Description |
| --- | --- |
| `QTuiTabBar(QTuiWidget *parent = nullptr)` | Makes an empty tab bar. It takes the focus (`Qt::StrongFocus`). |
| `int addTab(const QString &label)` | Adds a tab and returns its index. The first tab becomes current. |
| `void removeTab(int index)` | Removes a tab. Emits `currentChanged()` if the current index changes, or if the current tab is removed. |
| `int count() const` | Returns the number of tabs. |
| `QString tabText(int index) const` | Returns the label of a tab, or an empty string. |
| `void setTabText(int index, const QString &text)` | Sets the label of a tab. |
| `int currentIndex() const` | Returns the current tab, or -1. Property: `currentIndex`. |
| `void setCurrentIndex(int index)` | Sets the current tab. -1 is valid and means no tab. Emits `currentChanged()`. |
| `TabPosition tabPosition() const` | Returns the tab position. |
| `void setTabPosition(TabPosition position)` | Keeps the tab position. The tab bar does not use this value when it paints. |
| `QSize minimumSize() const` | Returns the width of all labels and separators, and 1 row. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the tab row. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |
| `QRect hoverRect() const` | Protected. Returns the cells of the tab or scroll indicator below the mouse, so that only that part shows the hover. |

### Signals

| Signal | Description |
| --- | --- |
| `currentChanged(int index)` | The current tab changed, or the user pressed Enter. |

### Keys

| Key | Action |
| --- | --- |
| Left | Selects the previous tab. |
| Right | Selects the next tab. |
| Enter | Emits `currentChanged()` again for the current tab. |

### Mouse

- A click on a tab makes it the current tab.
- A click on `‹` or `›` selects the previous or next tab.

## QTuiStackedWidget

`#include <QtTUI/QTuiStackedWidget.h>` · Inherits: QTuiWidget

QTuiStackedWidget shows one of its widgets at a time. The current widget fills all of the stacked widget. It has no keys and no visible chrome. To get the same behavior in a layout, use [QTuiStackedLayout](layouts.md#qtuistackedlayout).

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *stack = new QTuiStackedWidget(parent);
    auto *first = new QTuiLabel(QStringLiteral("Step 1"));
    auto *second = new QTuiLabel(QStringLiteral("Step 2"));
    stack->addWidget(first);
    stack->addWidget(second);
    stack->setCurrentWidget(second);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiStackedWidget(QTuiWidget *parent = nullptr)` | Makes an empty stacked widget. |
| `int addWidget(QTuiWidget *widget)` | Adds a widget, makes the stacked widget its parent and returns the index. The first widget becomes current. Other widgets are hidden. Returns -1 for nullptr. |
| `void removeWidget(QTuiWidget *widget)` | Removes a widget and hides it. It does not delete the widget. If it was current, the next widget at that index (or the last widget) becomes current. |
| `int count() const` | Returns the number of widgets. |
| `QTuiWidget *widget(int index) const` | Returns the widget at `index`, or nullptr. |
| `int indexOf(QTuiWidget *widget) const` | Returns the index of a widget, or -1. |
| `int currentIndex() const` | Returns the current index, or -1. Property: `currentIndex`. |
| `void setCurrentIndex(int index)` | Shows the widget at `index` and hides the old one. |
| `QTuiWidget *currentWidget() const` | Returns the current widget, or nullptr. |
| `void setCurrentWidget(QTuiWidget *widget)` | Shows `widget` if it is in the stack. |
| `QSize minimumSize() const` | Returns the largest minimum size of all widgets. |
| `void paintEvent(ScreenBuffer &buffer)` | Clears the area, gives the current widget the full geometry and paints it. |

### Signals

| Signal | Description |
| --- | --- |
| `currentChanged(int index)` | The current widget changed. The index is -1 if the stack is empty. |

## QTuiToolBox

`#include <QtTUI/QTuiToolBox.h>` · Inherits: QTuiFrame

QTuiToolBox shows a column of item headers. Only the current item is open: its widget shows below its header and takes the free rows. An open header has `▼` and a closed header has `▶`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *toolBox = new QTuiToolBox(parent);
    toolBox->addItem(new QTuiLabel(QStringLiteral("Recent files")), QStringLiteral("Recent"));
    toolBox->addItem(new QTuiLabel(QStringLiteral("Bookmarks")), QStringLiteral("Bookmarks"));

    QObject::connect(toolBox, &QTuiToolBox::currentChanged, [toolBox](int index) {
        qDebug() << "open item:" << toolBox->itemText(index);
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiToolBox(QTuiWidget *parent = nullptr)` | Makes an empty tool box. It takes the focus (`Qt::StrongFocus`). |
| `int addItem(QTuiWidget *widget, const QString &label)` | Adds an item, makes the tool box the parent of `widget` and returns the index. The first item becomes current. Returns -1 for nullptr. |
| `void removeItem(int index)` | Removes an item and hides its widget. It does not delete the widget. |
| `int count() const` | Returns the number of items. |
| `QTuiWidget *widget(int index) const` | Returns the widget of an item, or nullptr. |
| `QString itemText(int index) const` | Returns the label of an item, or an empty string. |
| `int currentIndex() const` | Returns the open item, or -1. Property: `currentIndex`. |
| `void setCurrentIndex(int index)` | Opens the item at `index` and closes the old one. |
| `QSize minimumSize() const` | Returns the widest header and 1 row for each header plus 1 row. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the headers and the widget of the open item. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |

### Signals

| Signal | Description |
| --- | --- |
| `currentChanged(int index)` | A different item opened. |

### Keys

| Key | Action |
| --- | --- |
| Up | Moves the header focus to the previous header. |
| Down | Moves the header focus to the next header. |
| Enter, Space | Opens the item of the focused header. |

### Mouse

- A click on a header opens that item.

## QTuiSplitter

`#include <QtTUI/QTuiSplitter.h>` · Inherits: QTuiFrame

QTuiSplitter puts its widgets side by side (`Qt::Horizontal`) or one above the other (`Qt::Vertical`). A [QTuiSplitterHandle](#qtuisplitterhandle) of 1 cell goes between each pair of widgets. The user moves a handle to change the sizes. The splitter lays out its widgets again each time its geometry changes.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *splitter = new QTuiSplitter(Qt::Horizontal, parent);
    splitter->addWidget(new QTuiLabel(QStringLiteral("Files")));
    splitter->addWidget(new QTuiTextEdit);
    splitter->setGeometry(QRect(0, 0, 80, 20));
    splitter->setSizes({20, 59}); // 20 + 1 handle + 59 = 80

    QObject::connect(splitter, &QTuiSplitter::splitterMoved, [](int position, int index) {
        qDebug() << "handle" << index << "at" << position;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiSplitter(Qt::Orientation orientation = Qt::Horizontal, QTuiWidget *parent = nullptr)` | Makes an empty splitter. |
| `void addWidget(QTuiWidget *widget)` | Adds a widget at the end and makes the splitter its parent. From the second widget, it also adds a handle. |
| `int count() const` | Returns the number of widgets. |
| `QTuiWidget *widget(int index) const` | Returns the widget at `index`, or nullptr. |
| `QList<int> sizes() const` | Returns the size of each widget in cells, along the orientation. |
| `void setSizes(const QList<int> &sizes)` | Sets the widget sizes. The list must have one value for each widget, or the call does nothing. The splitter scales the sizes to fit the free space, and makes each size at least the minimum size of its widget. |
| `Qt::Orientation orientation() const` | Returns the orientation. Property: `orientation`. |
| `void setOrientation(Qt::Orientation orientation)` | Sets the orientation of the splitter and of all handles. |
| `QTuiSplitterHandle *handle(int index) const` | Returns the handle after the widget at `index`, or nullptr. Handle 0 is between widget 0 and widget 1. |
| `QSize minimumSize() const` | Returns the sum of the widget minimum sizes plus 1 cell for each handle along the orientation, and the largest minimum size across it. |
| `void paintEvent(ScreenBuffer &buffer)` | Clears the area and paints the widgets and the handles. |
| `void resizeEvent(TuiResizeEvent *event)` | Protected. Lays out the widgets again. |

If no sizes are set, the splitter gives each widget the same size. Handle indexes start at 0. In Qt, handle 0 is never shown and the first visible handle is 1.

### Signals

| Signal | Description |
| --- | --- |
| `splitterMoved(int position, int index)` | The user moved handle `index`. `position` is the offset of the handle from the start of the splitter, in cells. |

## QTuiSplitterHandle

`#include <QtTUI/QTuiSplitter.h>` · Inherits: QTuiWidget

QTuiSplitterHandle is the divider between two widgets of a [QTuiSplitter](#qtuisplitter). The splitter makes the handles. A handle is `│` in a horizontal splitter and `─` in a vertical splitter. It takes the focus (`Qt::StrongFocus`) and uses the focus colors when it has the focus.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiSplitter *splitter)
{
    // Give the focus to the first handle, so the arrow keys move it
    if (QTuiSplitterHandle *handle = splitter->handle(0))
        handle->setFocus();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiSplitterHandle(Qt::Orientation orientation, QTuiSplitter *parent)` | Makes a handle. Usually the splitter calls this. |
| `Qt::Orientation orientation() const` | Returns the orientation. |
| `QTuiSplitter *splitter() const` | Returns the splitter. |
| `QSize minimumSize() const` | Returns 1 x 1. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the divider line. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |

### Keys

| Key | Action |
| --- | --- |
| Left, Right | Horizontal splitter: moves the handle 1 cell. |
| Up, Down | Vertical splitter: moves the handle 1 cell. |

### Mouse

- Drag the handle with the left button to move it. The widgets on the two sides do not become smaller than their minimum sizes.

## QTuiAbstractScrollArea

`#include <QtTUI/QTuiAbstractScrollArea.h>` · Inherits: QTuiFrame

QTuiAbstractScrollArea is the base class of [QTuiScrollArea](#qtuiscrollarea) and of the item views. It keeps the vertical scroll offset: the first content row that shows. Do not make an instance of this class. Use a subclass.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

// A view of 100 rows that scrolls to keep the current row visible
class RowView : public QTuiAbstractScrollArea {
public:
    using QTuiAbstractScrollArea::QTuiAbstractScrollArea;

    void setCurrentRow(int row)
    {
        m_row = qBound(0, row, 99);
        scrollRowIntoView(m_row, geometry().height());
        markDirty();
    }

private:
    int m_row = 0;
};
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiAbstractScrollArea(QTuiWidget *parent = nullptr)` | Makes the base part of a scroll area. |
| `int scrollOffset() const` | Returns the first content row that shows. |
| `void scrollRowIntoView(int row, int visibleRows)` | Protected. Changes the scroll offset as little as possible, so that `row` shows in a viewport of `visibleRows` rows. |

## QTuiScrollArea

`#include <QtTUI/QTuiScrollArea.h>` · Inherits: QTuiAbstractScrollArea

QTuiScrollArea shows a part of a content widget that can be larger than the scroll area. A scroll bar shows on the right or at the bottom only if the content is larger than the viewport. Unlike Qt, the scroll area does not set the size of the content widget, and it has no `widgetResizable` property.

Call `setWidget()` to set the content. This makes the scroll area the parent of the content widget. Then set the geometry of the content widget yourself. The geometry is in content coordinates: (0, 0) is the top-left cell of the viewport. A layout on the content widget places the children in the same coordinates. When Tab moves the focus to a widget in the content, the scroll area scrolls to make that widget visible.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *scroll = new QTuiScrollArea(parent);
    scroll->setGeometry(QRect(0, 0, 30, 10));

    auto *content = new QTuiWidget;
    auto *layout = createVBoxLayout();
    for (int i = 0; i < 40; ++i)
        layout->addWidget(new QTuiCheckBox(QStringLiteral("Option %1").arg(i), content));
    content->setLayout(layout);

    scroll->setWidget(content);
    // The scroll area does not set the content size: set it yourself
    content->setGeometry(QRect(0, 0, 29, 40));

    scroll->ensureWidgetVisible(content->childWidgets().last());
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiScrollArea(QTuiWidget *parent = nullptr)` | Makes an empty scroll area with hidden scroll bars. It takes the focus (`Qt::StrongFocus`). |
| `void setWidget(QTuiWidget *widget)` | Sets the content widget, makes the scroll area its parent and shows it. Resets the scroll position to (0, 0). The old content widget stays a child of the scroll area. |
| `QTuiWidget *widget() const` | Returns the content widget, or nullptr. |
| `QTuiScrollBar *verticalScrollBar() const` | Returns the vertical scroll bar. |
| `QTuiScrollBar *horizontalScrollBar() const` | Returns the horizontal scroll bar. |
| `void ensureVisible(int x, int y)` | Scrolls as little as possible, so that the content cell (x, y) shows. |
| `void ensureWidgetVisible(QTuiWidget *child)` | Scrolls so that `child` shows, as in Qt. `child` must be the content widget or a widget in it. Else the call does nothing. |
| `QRect contentArea() const` | Returns the viewport: the geometry without the cells of the visible scroll bars. |
| `QSize minimumSize() const` | Returns 3 x 3. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the visible part of the content, the scroll bars, and the arrows `▲` `▼` `◄` `►` where there is more content. |
| `QRect hoverRect() const` | Returns the cells of the content widget below the mouse, if that widget takes clicks. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |
| `void resizeEvent(TuiResizeEvent *event)` | Protected. Updates the scroll bars. |
| `void focusEntered(QTuiWidget *focus)` | Protected. Calls `ensureWidgetVisible(focus)` when a widget in the content gets the focus. |

### Keys

The scroll area gets these keys only when it has the focus itself.

| Key | Action |
| --- | --- |
| Up, Down | Scrolls 1 row. |
| Left, Right | Scrolls 1 column. |
| Page Up, Page Down | Scrolls 1 viewport height. |
| Home | Scrolls to the top-left. |
| End | Scrolls to the bottom. The horizontal position does not change. |

### Mouse

- The wheel scrolls 3 rows for each step.
- A click in the viewport goes to the content widget below the mouse, in content coordinates. If that widget takes click focus, it gets the focus.

## QTuiMdiArea

`#include <QtTUI/QTuiMdiArea.h>` · Inherits: QTuiWidget

QTuiMdiArea holds [QTuiMdiSubWindow](#qtuimdisubwindow) windows that can overlap. The active sub-window is raised on top of the others. A click anywhere in a sub-window activates it, also on a child widget that does not take the focus. The [main window](main-window.md) does this in its mouse dispatch.

Sub-window geometry is in screen coordinates. When a parent layout moves the MDI area, the area moves its sub-windows by the same offset.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *mdi = new QTuiMdiArea;
    window.setCentralWidget(mdi);

    auto *notes = new QTuiTextEdit;
    notes->setObjectName(QStringLiteral("Notes")); // becomes the window title
    mdi->addSubWindow(notes);

    QTuiMdiSubWindow *log = mdi->addSubWindow(new QTuiPlainTextEdit);
    log->setWindowTitle(QStringLiteral("Log"));

    QObject::connect(mdi, &QTuiMdiArea::subWindowActivated, [](QTuiMdiSubWindow *sub) {
        if (sub)
            qDebug() << "active:" << sub->windowTitle();
    });

    window.show();       // gives the MDI area its size
    mdi->tileSubWindows(); // uses the size of the area
    app.triggerRender();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiMdiArea(QTuiWidget *parent = nullptr)` | Makes an empty MDI area. It takes the focus (`Qt::StrongFocus`). |
| `QTuiMdiSubWindow *addSubWindow(QTuiWidget *widget)` | Puts `widget` in a new sub-window and returns the sub-window. The title is the `objectName()` of the widget. The size is half of the area (at least 10 x 5). Each new sub-window is 2 columns right and 1 row down from the previous one. The new sub-window becomes active. |
| `void removeSubWindow(QTuiWidget *widget)` | Removes the sub-window that holds `widget`. `widget` can also be the sub-window. The sub-window is hidden, not deleted. If it was active, the last sub-window in the list becomes active. |
| `QList<QTuiMdiSubWindow *> subWindowList() const` | Returns the sub-windows in the order you added them. |
| `QTuiMdiSubWindow *activeSubWindow() const` | Returns the active sub-window, or nullptr. |
| `void setActiveSubWindow(QTuiMdiSubWindow *window)` | Raises `window` on top, emits `window->aboutToActivate()`, and then emits `subWindowActivated()`. |
| `void cascadeSubWindows()` | Uses the current geometry of the area. Call it after the area has a size. Puts all sub-windows at the same size, each 2 columns right and 1 row down from the previous one. |
| `void tileSubWindows()` | Puts the sub-windows in a grid that fills the current geometry of the area. Call it after the area has a size. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |

### Signals

| Signal | Description |
| --- | --- |
| `subWindowActivated(QTuiMdiSubWindow *window)` | A different sub-window became active. `window` can be nullptr. |

### Keys

These keys apply when the MDI area itself has the focus.

| Key | Action |
| --- | --- |
| Tab | Activates the next sub-window. In a `QTuiMainWindow`, the main window uses Tab for its focus chain, so Tab does not get to the MDI area. |
| Other keys | Go to the active sub-window. |

## QTuiMdiSubWindow

`#include <QtTUI/QTuiMdiSubWindow.h>` · Inherits: QTuiWidget

QTuiMdiSubWindow is a window in a [QTuiMdiArea](#qtuimdiarea). It draws a frame with the title and the buttons `[□]` (maximize) and `[X]` (close) in the top border. The content widget fills the area inside the frame. When the sub-window, or a widget in it, gets the focus, the sub-window becomes the active window.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiMdiArea *mdi)
{
    QTuiMdiSubWindow *sub = mdi->addSubWindow(new QTuiTextEdit);
    sub->setWindowTitle(QStringLiteral("Draft"));
    sub->showMaximized();

    QObject::connect(sub, &QTuiMdiSubWindow::windowStateChanged,
                     [](Qt::WindowStates, Qt::WindowStates newState) {
                         qDebug() << "maximized:" << bool(newState & Qt::WindowMaximized);
                     });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiMdiSubWindow(QTuiWidget *parent = nullptr)` | Makes an empty sub-window. It takes the focus (`Qt::StrongFocus`). Usually you call `QTuiMdiArea::addSubWindow()` instead. |
| `void setWidget(QTuiWidget *widget)` | Sets the content widget and makes the sub-window its parent. |
| `QTuiWidget *widget() const` | Returns the content widget, or nullptr. |
| `QString windowTitle() const` | Returns the title. Property: `windowTitle`. |
| `void setWindowTitle(const QString &title)` | Sets the title. |
| `bool isMaximized() const` | Returns true if the sub-window is maximized. |
| `void showMaximized()` | Slot. Keeps the normal geometry and fills the parent area. Emits `windowStateChanged()`. |
| `void showNormal()` | Slot. Sets the normal geometry again. Emits `windowStateChanged()`. |
| `void close()` | Slot. Hides the sub-window. It does not delete the sub-window. The sub-window stays in `subWindowList()` and can stay the `activeSubWindow()`. |
| `QSize minimumSize() const` | Returns 3 rows, and the width of the title and buttons or of the content widget plus the frame. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the frame and the content widget. A maximized sub-window shows `[▫]` in place of `[□]`. |
| `void focusEntered(QTuiWidget *focus)` | Protected. Makes this sub-window the active window of its MDI area. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |

### Signals

| Signal | Description |
| --- | --- |
| `windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)` | The sub-window was maximized (`Qt::WindowMaximized`) or set back to normal (`Qt::WindowNoState`). |
| `aboutToActivate()` | The sub-window is about to become the active window. |

### Keys

| Key | Action |
| --- | --- |
| M | Maximizes the sub-window, or sets it back to normal. |
| Ctrl+Left, Ctrl+Right, Ctrl+Up, Ctrl+Down | Moves the sub-window 1 cell. |
| Shift+Left, Shift+Up | Makes the sub-window 1 cell narrower or shorter, down to its minimum size. |
| Shift+Right, Shift+Down | Makes the sub-window 1 cell wider or taller. |
| Escape, Q | Closes the sub-window. |

### Mouse

- A left click anywhere in the sub-window activates it.
- A click on `[X]` closes the sub-window.
- A click on `[□]` maximizes the sub-window, or sets it back to normal.
- Drag the title bar to move the sub-window. The sub-window stays in the MDI area.
- Drag the bottom-right corner cell to resize the sub-window.
- A maximized sub-window does not move or resize with the mouse.

## QTuiDockWidget

`#include <QtTUI/QTuiDockWidget.h>` · Inherits: QTuiWidget

QTuiDockWidget is a panel with a frame, a title and a close button `[X]`. Add it to a main window with `QTuiMainWindow::addDockWidget()`. See [main window](main-window.md). The content widget fills the area inside the frame.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiMainWindow *window)
{
    auto *dock = new QTuiDockWidget(QStringLiteral("Output"));
    dock->setWidget(new QTuiPlainTextEdit);
    window->addDockWidget(Qt::BottomDockWidgetArea, dock);

    QObject::connect(dock, &QTuiDockWidget::visibilityChanged, [](bool visible) {
        qDebug() << "dock visible:" << visible;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiDockWidget(const QString &title = QString(), QTuiWidget *parent = nullptr)` | Makes a dock widget with a title. It takes the focus (`Qt::StrongFocus`). |
| `void setWidget(QTuiWidget *widget)` | Sets the content widget and makes the dock widget its parent. |
| `QTuiWidget *widget() const` | Returns the content widget, or nullptr. |
| `QString windowTitle() const` | Returns the title. Property: `windowTitle`. |
| `void setWindowTitle(const QString &title)` | Sets the title. |
| `Qt::DockWidgetAreas allowedAreas() const` | Returns the allowed areas. The default is `Qt::AllDockWidgetAreas`. |
| `void setAllowedAreas(Qt::DockWidgetAreas areas)` | Keeps the allowed areas. No other part of QtTUI reads this value. |
| `bool isFloatable() const` | Returns the floatable flag. Property: `floatable`. The default is true. |
| `void setFloatable(bool floatable)` | Keeps the floatable flag. A dock widget cannot float in QtTUI. |
| `QRect contentArea() const` | Returns the area inside the frame. Returns an empty rect if the dock widget is smaller than 3 x 3 cells. |
| `QSize minimumSize() const` | Returns 3 rows, and the width of the title and `[X]` or of the content widget plus the frame. |
| `void paintEvent(ScreenBuffer &buffer)` | Paints the frame and the content widget. |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |

### Signals

| Signal | Description |
| --- | --- |
| `visibilityChanged(bool visible)` | The user closed the dock widget. The value is false. |

### Keys

| Key | Action |
| --- | --- |
| Escape, Q | Hides the dock widget and emits `visibilityChanged(false)`. |

### Mouse

- A left click on `[X]` hides the dock widget and emits `visibilityChanged(false)`.

## QTuiSizeGrip

`#include <QtTUI/QTuiSizeGrip.h>` · Inherits: QTuiWidget

QTuiSizeGrip is one cell with `╱`. It resizes the nearest MDI sub-window, dialog or main window that contains it. If there is no such window, it resizes the topmost parent widget. The size stays at or above the minimum size of the window, and inside the parent of the window. For a sub-window, that parent is the MDI area.

The grip starts at `QRect(0, 0, 1, 1)`. Put it in a layout, or set its geometry yourself. In the example, the vertical layout puts the grip in the first cell of the last row.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiMdiArea *mdi)
{
    auto *content = new QTuiWidget;
    auto *layout = createVBoxLayout();
    layout->addWidget(new QTuiTextEdit(content), 1);
    auto *grip = new QTuiSizeGrip(content);
    grip->setSizePolicy(QTuiSizePolicy::Fixed, QTuiSizePolicy::Fixed);
    layout->addWidget(grip);
    content->setLayout(layout);

    mdi->addSubWindow(content); // the grip resizes this sub-window
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiSizeGrip(QTuiWidget *parent = nullptr)` | Makes a size grip. It takes the focus (`Qt::StrongFocus`). |
| `QSize minimumSize() const` | Returns 1 x 1. |
| `void paintEvent(ScreenBuffer &buffer)` | Clears the grip and draws `╱` in its bottom-right cell. It uses the focus colors when it has the focus. |
| `void keyPressEvent(TuiKeyEvent *event)` | Protected. See "Keys". |
| `void mousePressEvent(TuiMouseEvent *event)` | Protected. See "Mouse". |

### Keys

| Key | Action |
| --- | --- |
| Right, Left | Makes the window 1 cell wider or narrower. |
| Down, Up | Makes the window 1 cell taller or shorter. |

### Mouse

- Drag the grip with the left button. The bottom-right corner of the window moves with the mouse.
