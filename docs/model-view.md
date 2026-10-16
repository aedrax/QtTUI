# Model/view

This page tells how the views show a Qt model, how selection, editing and
context menus work, and documents the view, header, delegate and mapper
classes. For views that keep their own items, see [item widgets](item-widgets.md).

## How the views use a model

A view does not copy the data of the model. Each paint reads the model again
with `QAbstractItemModel::data()`. A view shows the `Qt::DisplayRole` text
of each item. It does not show icons, check states or other roles.

`setModel()` connects the view to the model signals in the table below. When
a signal comes, the view marks only the screen rows that changed as dirty,
and the next render pass paints them. See
[events and rendering](events-and-rendering.md) for the render pass.

| Model signal | What the view repaints |
| --- | --- |
| `dataChanged` | The changed rows. `QTuiColumnView` repaints all. |
| `rowsInserted`, `rowsRemoved` | The rows from the first changed row to the bottom of the view. |
| `columnsInserted`, `columnsRemoved` | All (`QTuiTableView` only). |
| `headerDataChanged` | The header row (`QTuiTableView` only). |
| `layoutChanged`, `modelReset` | All. |

The current item follows its item through inserts, removes, moves and
sorts. A model reset makes the first item current and closes the editor
without a change. The view does not take ownership of the model. If the
model is deleted, the view no longer uses it.

Use any `QAbstractItemModel`. QtTUI uses Qt Core only, so
`QStandardItemModel` is not available. Use `QStringListModel`, a
`QSortFilterProxyModel`, or your own model.

## Selection

Each view has a `QItemSelectionModel`, as in Qt. `setModel()` makes a new
selection model for the new model and deletes the old one that the view
made. `setSelectionModel()` sets a selection model that you own. It must use
the same model as the view.

The selection mode tells which keys and clicks select items. The default is
`SingleSelection`.

| Mode | Behavior |
| --- | --- |
| `NoSelection` | Nothing is selected. |
| `SingleSelection` | The current item is the selected item. |
| `MultiSelection` | A click or Space toggles an item. The arrow keys move the current item only. |
| `ExtendedSelection` | A click or an arrow key selects one item. Ctrl+click and Ctrl+Space toggle an item. Shift+click and Shift+arrow select a range. Ctrl+arrow moves the current item only. |
| `ContiguousSelection` | As `ExtendedSelection`, but Ctrl does not toggle. Ctrl+click and Ctrl+arrow select one item. |

In the three multiple modes (`MultiSelection`, `ExtendedSelection`,
`ContiguousSelection`), Ctrl+A selects all items. A range starts at the
anchor. The anchor is the last item that a click, a key without Shift, or
Space selected.

In `QTuiListView`, `QTuiTableView` and `QTuiTreeView`, a left drag selects a
range, as Shift+click does. Use the drag in terminals that keep Shift+click
for their own text selection, for example Konsole.

The selection behavior tells what one selection step selects:
`SelectItems` (one cell, the default), `SelectRows` or `SelectColumns`.

A selected item has the focus colors. If the view does not have focus, a
selected item has a dimmer background and bold text. In the multiple modes,
the current item has an underline, because it can be current but not
selected.

## Editing

An editor opens on an item that has `Qt::ItemIsEditable` in its flags. The
edit triggers tell which actions open it. The default is
`DoubleClicked | EditKeyPressed`, as in Qt.

| Edit trigger | Action that opens the editor |
| --- | --- |
| `DoubleClicked` | A double click on the item. |
| `EditKeyPressed` | F2. |
| `AnyKeyPressed` | A printable character other than Space, without Ctrl, Alt or Meta. The character replaces the text of the item. |

You can also call `edit()`. The item delegate makes the editor. If you do
not set a delegate, the view uses a `QTuiStyledItemDelegate`. The editor
covers the item, and its text is selected, so that typing replaces it.

Enter writes the value to the model and closes the editor. Escape closes it
without a change. If the focus leaves the editor (Tab or a click), the
editor writes the value. If an editor has an open popup (the list of a
combo box), the popup gets Enter and Escape first.

## Context menus

Shift+F10 or the Menu key asks for the context menu of the focus widget. A
right click asks for the context menu of the widget under the mouse. The
`contextMenuPolicy` of the widget tells what happens. See
[main window](main-window.md) and [widget basics](widget.md).

A click with any button makes the item under the mouse current. A button
other than the left button keeps the selection if the selection contains
the item. If not, the click selects the item. Thus the menu acts on the
selected items.

```cpp
#include <QtTUI/QtTUI>
#include <QStringListModel>
#include <algorithm>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *model = new QStringListModel({"alpha", "beta", "gamma"}, parent);
    auto *view = new QTuiListView(parent);
    view->setModel(model);
    view->setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
    view->setContextMenuPolicy(Qt::ActionsContextMenu);

    auto *remove = new QTuiAction(QStringLiteral("Remove"), view);
    view->addAction(remove);
    QObject::connect(remove, &QTuiAction::triggered, view, [view, model]() {
        QModelIndexList rows = view->selectionModel()->selectedRows();
        std::sort(rows.begin(), rows.end());
        for (int i = rows.size() - 1; i >= 0; --i)
            model->removeRow(rows.at(i).row());
    });
}
```

## QTuiAbstractItemView

`#include <QtTUI/QTuiAbstractItemView.h>` · Inherits: `QTuiAbstractScrollArea`

The base class of the views. It holds the model, the current index, the
selection model, the edit triggers and the item delegate. You cannot make
an instance; use one of the views below.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QTuiMainWindow window;
    app.setMainWindow(&window);

    QStringListModel model({"Apples", "Bread", "Cheese", "Dates"});
    auto *view = new QTuiListView(&window);
    view->setModel(&model);
    view->setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
    view->setEditTriggers(QTuiAbstractItemView::AllEditTriggers);

    QObject::connect(view, &QTuiAbstractItemView::activated,
                     [](const QModelIndex &index) {
        qDebug() << "activated" << index.data().toString();
    });

    auto *layout = createVBoxLayout(&window);
    layout->addWidget(view, 1);
    window.setLayout(layout);

    view->setFocus();
    window.show();
    app.triggerRender();
    return app.exec();
}
```

### Enums

| Enum | Values |
| --- | --- |
| `SelectionMode` | `NoSelection`, `SingleSelection`, `MultiSelection`, `ExtendedSelection`, `ContiguousSelection` |
| `SelectionBehavior` | `SelectItems`, `SelectRows`, `SelectColumns` |
| `EditTrigger` (flags `EditTriggers`) | `NoEditTriggers`, `DoubleClicked`, `EditKeyPressed`, `AnyKeyPressed`, `AllEditTriggers`. The values are the same as in Qt. |
| `ScrollHint` | `EnsureVisible`, `PositionAtTop`, `PositionAtBottom`, `PositionAtCenter` |

### Properties

`selectionMode`, `selectionBehavior`, `editTriggers`.

### Functions

| Function | Description |
| --- | --- |
| `QTuiAbstractItemView(QTuiWidget *parent = nullptr)` | Makes the base of a view. Call it from the constructor of your view. |
| `virtual void setModel(QAbstractItemModel *model)` | Sets the model and makes a new selection model. Each view implements it. |
| `QAbstractItemModel *model() const` | Returns the model, or `nullptr`. |
| `virtual QModelIndex currentIndex() const` | Returns the current item. |
| `virtual void setCurrentIndex(const QModelIndex &index)` | Makes the item current and scrolls to it. It also selects the item, but not in `NoSelection` or `MultiSelection`. |
| `QItemSelectionModel *selectionModel() const` | Returns the selection model. |
| `void setSelectionModel(QItemSelectionModel *selectionModel)` | Sets a selection model. The view does not take ownership. A selection model for a different model gives a warning, and the view ignores it. |
| `SelectionMode selectionMode() const` | Returns the selection mode. |
| `void setSelectionMode(SelectionMode mode)` | Sets the selection mode. |
| `SelectionBehavior selectionBehavior() const` | Returns the selection behavior. |
| `void setSelectionBehavior(SelectionBehavior behavior)` | Sets the selection behavior. |
| `void clearSelection()` | Clears the selection. |
| `virtual void selectAll()` | Selects all items in the three multiple modes. In the other modes, it does nothing. |
| `EditTriggers editTriggers() const` | Returns the edit triggers. |
| `void setEditTriggers(EditTriggers triggers)` | Sets the actions that open an editor. |
| `void setItemDelegate(QTuiItemDelegate *delegate)` | Sets the delegate that makes the editors. The view does not take ownership. An open editor closes without a change. |
| `QTuiItemDelegate *itemDelegate() const` | Returns the delegate that you set, or `nullptr`. |
| `bool edit(const QModelIndex &index)` | Opens an editor on the item. Returns `false` if the item is not editable, not visible, or the delegate makes no editor. |
| `bool isEditing() const` | Returns `true` if an editor is open. |
| `QTuiWidget *editor() const` | Returns the open editor, or `nullptr`. |
| `void closeEditor(bool commit = true)` | Closes the editor. With `commit`, the editor writes its value to the model first. |
| `virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Scrolls the view so that the item is visible. |
| `virtual QRect visualRect(const QModelIndex &index) const` | Returns the screen area of the item, or an empty rectangle if the item is not visible. |

### Protected functions

Use these functions when you write a view.

| Function | Description |
| --- | --- |
| `void resetSelectionModel()` | Makes a new selection model. Call it at the end of `setModel()`, after you connect to the model signals. |
| `void selectCurrent(Qt::KeyboardModifiers modifiers, bool click)` | Updates the selection for the mode and the modifiers. Call it after a key or a click makes a different item current. |
| `bool selectionKeyPressEvent(TuiKeyEvent *event)` | Handles Space, Ctrl+Space and Ctrl+A. Returns `true` if the key changed the selection. |
| `bool editKeyPressEvent(TuiKeyEvent *event)` | Handles F2 and typed characters. Returns `true` if the key opened an editor. |
| `void scrollToRow(int row, int visibleRows, int rowCount, ScrollHint hint)` | Moves the scroll offset to show the row, as the hint tells. |
| `void markRowsDirty(int first, int last, int top = 0)` | Marks the visible rows from `first` to `last` as dirty. `top` is the number of lines above the first row (for example a header). |
| `void updateEditorGeometry()` | Moves the editor to its item. If the item is not visible, it closes the editor and writes the value. |
| `bool eventFilter(QObject *watched, QEvent *event) override` | Gives Enter and Escape in the editor to the view. |
| `bool isSelected(const QModelIndex &index) const` | Returns `true` if the item is selected. |
| `Cell itemStyle(const QModelIndex &index, bool isCurrent, const ThemeProperties &theme) const` | Returns the colors and attributes of an item, as the selection section above tells. |
| `virtual QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const` | Returns the items from `from` to `to`. The default is the rectangle between the two items if they have the same parent. |

### Signals

| Signal | Description |
| --- | --- |
| `clicked(const QModelIndex &index)` | A left click on an item. A drag does not emit it. |
| `activated(const QModelIndex &index)` | Enter on an item, or a click or double click. Each view section tells which. |
| `doubleClicked(const QModelIndex &index)` | A double click on an item. |

### Keys

These keys work in all views.

| Key | Action |
| --- | --- |
| Space | Selects the current item. In `MultiSelection`, toggles it. |
| Ctrl+Space | Toggles the current item in `ExtendedSelection`. |
| Ctrl+A | Selects all items in the three multiple modes. |
| F2 | Opens the editor (`EditKeyPressed`). |
| A printable character | Opens the editor and types the character (`AnyKeyPressed`). |
| Enter (in the editor) | Writes the value and closes the editor. |
| Escape (in the editor) | Closes the editor without a change. |
| Shift+F10, Menu | Asks for the context menu. |

## QTuiListView

`#include <QtTUI/QTuiListView.h>` · Inherits: `QTuiAbstractItemView`

Shows column 0 of the top-level rows of a model, one row for each line.
Different from Qt: each left click emits `activated()`, and a double click
emits only `doubleClicked()`.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *model = new QStringListModel({"red", "green", "blue"}, parent);
    auto *view = new QTuiListView(parent);
    view->setModel(model);
    view->setCurrentIndex(model->index(1, 0));

    QObject::connect(view, &QTuiAbstractItemView::activated,
                     [](const QModelIndex &index) {
        qDebug() << index.row() << index.data().toString();
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiListView(QTuiWidget *parent = nullptr)` | Makes a list view. It takes the focus with Tab and with a click. |
| `void setModel(QAbstractItemModel *model)` | Sets the model. The first row becomes current. |
| `QModelIndex currentIndex() const` | Returns the current row in column 0, or an invalid index if the model is empty. |
| `void setCurrentIndex(const QModelIndex &index)` | Makes the row current. A row out of range goes to the nearest row. |
| `void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Scrolls to a top-level row. |
| `QRect visualRect(const QModelIndex &index) const` | Returns the line of the row. |
| `QRect hoverRect() const override` | Returns the row under the mouse, for the hover highlight. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the visible rows. Long text is truncated. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down | Move the current row. With Shift, select a range. With Ctrl, move without a change to the selection (`ExtendedSelection`). |
| Enter | Emits `activated()`. |

### Mouse

- A left click makes the row current, selects it and emits `clicked()` and `activated()`.
- Ctrl+click and Shift+click change the selection, as the selection mode tells.
- A left drag selects the range from the pressed row to the row under the mouse.
- A double click emits `doubleClicked()` and opens the editor (`DoubleClicked`).
- A right click makes the row current and asks for the context menu.

## QTuiTableView

`#include <QtTUI/QTuiTableView.h>` · Inherits: `QTuiAbstractItemView`

Shows a table model with a header row. A space separates the columns. The
view paints its own header; it does not use `QTuiHeaderView`, and it does
not show the vertical header.

The natural width of a column is the widest of the header and the visible
cells. If the view is too narrow, the columns shrink. `setColumnWidth()`
replaces the natural width.

Sorting works as in `QTableView`. The view calls
`QAbstractItemModel::sort()`, so the model (or a `QSortFilterProxyModel` in
front of it) must implement `sort()`. The header of the sort column shows
"▲" (ascending) or "▼" (descending).

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QAbstractTableModel>
#include <algorithm>

using namespace QtTUI;

// An editable table of names and ages, without Q_OBJECT
class PeopleModel : public QAbstractTableModel {
public:
    using QAbstractTableModel::QAbstractTableModel;

    int rowCount(const QModelIndex &parent = {}) const override
    { return parent.isValid() ? 0 : int(m_rows.size()); }
    int columnCount(const QModelIndex &parent = {}) const override
    { return parent.isValid() ? 0 : 2; }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role != Qt::DisplayRole && role != Qt::EditRole)
            return {};
        const QStringList &row = m_rows.at(index.row());
        return row.at(index.column());
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        return section == 0 ? QStringLiteral("Name") : QStringLiteral("Age");
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    { return QAbstractTableModel::flags(index) | Qt::ItemIsEditable; }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role != Qt::EditRole)
            return false;
        m_rows[index.row()][index.column()] = value.toString();
        emit dataChanged(index, index);
        return true;
    }

    void sort(int column, Qt::SortOrder order) override
    {
        emit layoutAboutToBeChanged();
        std::sort(m_rows.begin(), m_rows.end(),
                  [column, order](const QStringList &a, const QStringList &b) {
            return order == Qt::AscendingOrder ? a.at(column) < b.at(column)
                                               : b.at(column) < a.at(column);
        });
        emit layoutChanged();
    }

private:
    QList<QStringList> m_rows{{"Ada", "36"}, {"Linus", "54"}, {"Grace", "85"}};
};

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QTuiMainWindow window;
    app.setMainWindow(&window);

    PeopleModel model;
    auto *table = new QTuiTableView(&window);
    table->setModel(&model);
    table->setSortingEnabled(true);
    table->setColumnWidth(0, 12);
    table->setSelectionBehavior(QTuiAbstractItemView::SelectRows);

    auto *layout = createVBoxLayout(&window);
    layout->addWidget(table, 1);
    window.setLayout(layout);

    table->setFocus();
    window.show();
    app.triggerRender();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTableView(QTuiWidget *parent = nullptr)` | Makes a table view. It takes the focus with Tab and with a click. |
| `void setModel(QAbstractItemModel *model)` | Sets the model. The cell at row 0, column 0 becomes current. |
| `QModelIndex currentIndex() const` | Returns the current cell, or an invalid index if the model is empty. |
| `void setCurrentIndex(const QModelIndex &index)` | Makes the cell current. A row or column out of range goes to the nearest one. |
| `void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Scrolls to the row and to the column of the cell. |
| `QRect visualRect(const QModelIndex &index) const` | Returns the screen area of the cell. |
| `QRect hoverRect() const override` | Returns the row under the mouse, for the hover highlight. |
| `int horizontalScrollOffset() const` | Returns the first visible column. |
| `bool isSortingEnabled() const` | Returns `true` if a header click sorts. |
| `void setSortingEnabled(bool enable)` | Turns sorting with the header and with Alt+Up/Down on or off. The default is off. |
| `void sortByColumn(int column, Qt::SortOrder order)` | Calls `sort()` on the model and shows the sort indicator. |
| `int sortColumn() const` | Returns the sort column, or -1 if the view is not sorted. |
| `Qt::SortOrder sortOrder() const` | Returns the sort order. |
| `int columnWidth(int column) const` | Returns the width of the column as the view paints it. |
| `void setColumnWidth(int column, int width)` | Sets the width of the column. The minimum is 1. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the header and the visible rows. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down, Left, Right | Move the current cell. With Shift, select a range. With Ctrl, move without a change to the selection (`ExtendedSelection`). |
| Alt+Left, Alt+Right | Make the current column narrower or wider by 1. |
| Alt+Up, Alt+Down | Sort by the current column, ascending or descending. Only if sorting is enabled. |
| Enter | Emits `activated()`. |

### Mouse

- A left click on a cell makes it current, selects it and emits `clicked()`. A single click does not emit `activated()`.
- A left drag selects the range from the pressed cell to the cell under the mouse.
- A click on a header sorts by that column, if sorting is enabled. A second click reverses the order.
- A left drag that starts on the separator after a header sets the width of that column.
- A double click emits `doubleClicked()`. Then it opens the editor (`DoubleClicked`). If no editor opens, it emits `activated()`.

## QTuiTreeView

`#include <QtTUI/QTuiTreeView.h>` · Inherits: `QTuiAbstractItemView`

Shows a tree model. Each level has an indent of 2 cells. An item with
children has "▶" (collapsed) or "▼" (expanded) before its text. Different
from Qt: the view shows only column 0 and has no header. If you set an item
delegate, the view calls its `paint()` for the text of each item.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

// model is any tree model
void example(QTuiWidget *parent, QAbstractItemModel *model)
{
    auto *tree = new QTuiTreeView(parent);
    tree->setModel(model);
    tree->setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
    tree->expand(model->index(0, 0));

    QObject::connect(tree, &QTuiTreeView::expanded, [](const QModelIndex &index) {
        qDebug() << "expanded" << index.data().toString();
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTreeView(QTuiWidget *parent = nullptr)` | Makes a tree view. It takes the focus with Tab and with a click. |
| `void setModel(QAbstractItemModel *model)` | Sets the model. All items are collapsed. |
| `QModelIndex currentIndex() const` | Returns the current item. |
| `void setCurrentIndex(const QModelIndex &index)` | Makes the item current. If the item is in a collapsed parent, nothing changes. |
| `void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Expands the parents of the item and scrolls to it. |
| `QRect visualRect(const QModelIndex &index) const` | Returns the area of the item text, after the indent and the indicator. |
| `QRect hoverRect() const override` | Returns the row under the mouse, for the hover highlight. |
| `void expand(const QModelIndex &index)` | Shows the children of the item and emits `expanded()`. |
| `void collapse(const QModelIndex &index)` | Hides the children of the item and emits `collapsed()`. |
| `bool isExpanded(const QModelIndex &index) const` | Returns `true` if the item is expanded. |
| `void selectAll() override` | Selects all visible items in the three multiple modes. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the visible rows. |
| `QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const override` | Returns the visible rows between the two items, at all levels. |

### Signals

| Signal | Description |
| --- | --- |
| `expanded(const QModelIndex &index)` | The item is expanded. |
| `collapsed(const QModelIndex &index)` | The item is collapsed. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down | Move the current item. With Shift, select a range. |
| Right | Expands the item. If it is expanded, moves to its first child. |
| Left | Collapses the item. If it is collapsed or has no children, moves to its parent. |
| Enter | Expands or collapses an item with children, and emits `activated()`. |

### Mouse

- A left click makes the item current, selects it and emits `clicked()`.
- A left drag selects the range from the pressed item to the item under the mouse.
- A double click emits `doubleClicked()`. Then it opens the editor (`DoubleClicked`). If no editor opens, it expands or collapses the item and emits `activated()`.

## QTuiColumnView

`#include <QtTUI/QTuiColumnView.h>` · Inherits: `QTuiAbstractItemView`

Shows a tree model as columns, as in `QColumnView`. The first column shows
the top-level items. The next column shows the children of the current item
of the column before it. A "│" line separates the columns. An item with
children has " ▶" after its text. The view does not scroll vertically: it
shows only the rows that fit in its height.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent, QAbstractItemModel *model)
{
    auto *columns = new QTuiColumnView(parent);
    columns->setModel(model);
    columns->setDefaultColumnWidth(16);
    columns->setColumnWidths({24, 16});
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiColumnView(QTuiWidget *parent = nullptr)` | Makes a column view. It takes the focus with Tab and with a click. |
| `void setModel(QAbstractItemModel *model)` | Sets the model and shows the first column. |
| `QModelIndex currentIndex() const` | Returns the current item of the active column. |
| `void setCurrentIndex(const QModelIndex &index)` | Opens the columns on the path to the item and makes it current. |
| `void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Scrolls to the column of the item, if the view shows that column. The hint has no effect. |
| `QRect visualRect(const QModelIndex &index) const` | Returns the screen area of the item. |
| `int defaultColumnWidth() const` | Returns the width of a column that has no width in `columnWidths()`. The default is 20. |
| `void setDefaultColumnWidth(int width)` | Sets the default width. A value of 0 or less has no effect. |
| `QList<int> columnWidths() const` | Returns the widths of the first columns. |
| `void setColumnWidths(const QList<int> &widths)` | Sets the widths of the first columns, from the left. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the visible columns. |
| `QSize minimumSize() const override` | Returns the default column width and 3 lines. |
| `QRect hoverRect() const override` | Returns the item under the mouse, for the hover highlight. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down | Move the current item in the active column. The next column shows its children. |
| Right | Moves to the child column, if the item has children. |
| Left | Moves to the column before. |
| Enter | Emits `activated()`. |

### Mouse

- A click makes the item current and shows its children in the next column.
- Ctrl+click and Shift+click change the selection, as the selection mode tells. A drag does not select.
- A double click emits `doubleClicked()`. Then it opens the editor (`DoubleClicked`). If no editor opens, it emits `activated()`.

## QTuiHeaderView

`#include <QtTUI/QTuiHeaderView.h>` · Inherits: `QTuiWidget`

A row (horizontal) or a column (vertical) of header labels from a model.
Different from Qt: it is a separate widget that you put in a layout. The
views do not use it, and it does not take the mouse. Each section has a
size. The default size is 10 and the minimum is 2.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *model = new QStringListModel({"one", "two", "three"}, parent);
    auto *header = new QTuiHeaderView(Qt::Vertical, parent);
    header->setModel(model);
    header->setSectionHidden(1, true);

    QObject::connect(header, &QTuiHeaderView::sectionClicked, [](int section) {
        qDebug() << "section" << section;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiHeaderView(Qt::Orientation orientation, QTuiWidget *parent = nullptr)` | Makes a header. `Qt::Horizontal` shows the column headers, `Qt::Vertical` the row headers. |
| `void setModel(QAbstractItemModel *model)` | Sets the model. The header follows the header, row and column signals of the model. |
| `QAbstractItemModel *model() const` | Returns the model. |
| `Qt::Orientation orientation() const` | Returns the orientation (also a read-only property). |
| `int sectionSize(int logicalIndex) const` | Returns the size of the section, or 0 if it does not exist. |
| `void resizeSection(int logicalIndex, int size)` | Sets the size of the section and emits `sectionResized()`. |
| `bool isSectionHidden(int logicalIndex) const` | Returns `true` if the section is hidden. |
| `void setSectionHidden(int logicalIndex, bool hide)` | Hides or shows the section. |
| `int count() const` | Returns the number of sections: the columns or the rows of the model. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the labels. The focused section has the focus colors. |
| `QSize minimumSize() const override` | Horizontal: the sum of the visible section sizes. Vertical: the longest label and the number of visible sections. |

### Signals

| Signal | Description |
| --- | --- |
| `sectionClicked(int logicalIndex)` | Enter on the focused section. |
| `sectionResized(int logicalIndex, int oldSize, int newSize)` | The size of a section changed. |

### Keys

| Key | Action |
| --- | --- |
| Left, Right | Move to the previous or next visible section (horizontal). |
| Up, Down | Move to the previous or next visible section (vertical). |
| Enter | Emits `sectionClicked()` for the focused section. |
| +, = | Make the focused section larger by 1. |
| - | Make the focused section smaller by 1. |

## QTuiItemDelegate

`#include <QtTUI/QTuiItemDelegate.h>` · Inherits: `QObject`

The base class of the delegates. A delegate makes the editors for a view,
and `QTuiTreeView` also uses it to paint the item text. Different from Qt:
the list, table and column views always paint the text themselves, and no
view calls `sizeHint()`. The base class makes no editor, so a view with a
`QTuiItemDelegate` cannot edit.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QtTUI/RenderUtils.h>

using namespace QtTUI;

// Paints the item text in upper case, in yellow
class UpperDelegate : public QTuiStyledItemDelegate {
public:
    using QTuiStyledItemDelegate::QTuiStyledItemDelegate;

    void paint(ScreenBuffer &buffer, const QRect &rect,
               const QModelIndex &index) const override
    {
        const Cell style = cellStyle(Color(255, 220, 0), Color(0, 0, 0));
        fillRect(buffer, QRect(rect.x(), rect.y(), rect.width(), 1), style);
        renderText(buffer, rect.x(), rect.y(), index.data().toString().toUpper(),
                   rect.width(), style);
    }
};

void example(QTuiTreeView *tree)
{
    tree->setItemDelegate(new UpperDelegate(tree));
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiItemDelegate(QObject *parent = nullptr)` | Makes a delegate. |
| `virtual void paint(ScreenBuffer &buffer, const QRect &rect, const QModelIndex &index) const` | Writes the `Qt::DisplayRole` text in the rectangle. It changes only the characters, not the colors. |
| `virtual QTuiWidget *createEditor(QTuiWidget *parent, const QModelIndex &index) const` | Returns a new editor for the item. The default returns `nullptr`. |
| `virtual void setEditorData(QTuiWidget *editor, const QModelIndex &index) const` | Copies the value of the item to the editor. The default does nothing. |
| `virtual void setModelData(QTuiWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const` | Writes the value of the editor to the model. The default does nothing. |
| `virtual QSize sizeHint(const QModelIndex &index) const` | Returns the length of the text and a height of 1. |

### Signals

| Signal | Description |
| --- | --- |
| `commitData(QTuiWidget *editor)` | Emit it to make the view write the value of the editor to the model. The editor stays open. |
| `closeEditor(QTuiWidget *editor)` | Emit it to make the view close the editor without a change. |

## QTuiStyledItemDelegate

`#include <QtTUI/QTuiStyledItemDelegate.h>` · Inherits: `QTuiItemDelegate`

The default delegate of the views. It selects the editor from the data of
the item:

| Data of the item | Editor |
| --- | --- |
| A `QStringList` in `Qt::UserRole` | A [`QTuiComboBox`](text-input.md) with those items. It writes the current text. |
| An `int` or `qlonglong` in `Qt::EditRole` | A `QTuiSpinBox` with the range -999999 to 999999. It writes the value. |
| Other data | A [`QTuiLineEdit`](text-input.md). It writes the text. |

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

// A line edit that takes 8 characters at most
class ShortTextDelegate : public QTuiStyledItemDelegate {
public:
    using QTuiStyledItemDelegate::QTuiStyledItemDelegate;

    QTuiWidget *createEditor(QTuiWidget *parent, const QModelIndex &index) const override
    {
        QTuiWidget *editor = QTuiStyledItemDelegate::createEditor(parent, index);
        if (auto *lineEdit = qobject_cast<QTuiLineEdit *>(editor))
            lineEdit->setMaxLength(8);
        return editor;
    }
};

void example(QTuiTableView *table)
{
    table->setItemDelegate(new ShortTextDelegate(table));
    table->setEditTriggers(QTuiAbstractItemView::DoubleClicked
                           | QTuiAbstractItemView::EditKeyPressed);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiStyledItemDelegate(QObject *parent = nullptr)` | Makes a delegate. |
| `void paint(ScreenBuffer &buffer, const QRect &rect, const QModelIndex &index) const override` | Fills the first line of the rectangle with the colors of the `QTuiStyledItemDelegate` theme entry and writes the text. |
| `QTuiWidget *createEditor(QTuiWidget *parent, const QModelIndex &index) const override` | Makes a combo box, a spin box or a line edit, as the table above tells. |
| `void setEditorData(QTuiWidget *editor, const QModelIndex &index) const override` | Copies the `Qt::EditRole` value to the editor. A combo box selects the item with the same text. |
| `void setModelData(QTuiWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override` | Writes the value of the editor with `Qt::EditRole`. |

## QTuiDataWidgetMapper

`#include <QtTUI/QTuiDataWidgetMapper.h>` · Inherits: `QObject`

Shows one row of a model in a set of widgets, one widget for each column
(section). Different from Qt: the mapper selects the widget property from
the widget type, and it has no delegate. With `AutoSubmit`, the mapper
writes each change of a widget to the model at once, not when the focus
leaves the widget. It maps only top-level rows.

| Widget | Property |
| --- | --- |
| `QTuiSpinBox`, `QTuiDoubleSpinBox` | `value` |
| `QTuiComboBox` | `currentIndex` |
| `QTuiCheckBox` | `checkState` |
| Other widgets | `text` |

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *model = new QStringListModel({"first", "second", "third"}, parent);
    auto *list = new QTuiListView(parent);
    list->setModel(model);

    auto *edit = new QTuiLineEdit(parent);
    auto *mapper = new QTuiDataWidgetMapper(parent);
    mapper->setModel(model);
    mapper->addMapping(edit, 0);
    mapper->toFirst();

    // The line edit follows the current row of the list
    QObject::connect(list->selectionModel(), &QItemSelectionModel::currentChanged,
                     mapper, [mapper](const QModelIndex &current) {
        mapper->setCurrentIndex(current.row());
    });
}
```

### Enums

| Enum | Values |
| --- | --- |
| `SubmitPolicy` | `AutoSubmit` (the default): each change of a widget goes to the model. `ManualSubmit`: changes go to the model only when you call `submit()`. |

### Functions

| Function | Description |
| --- | --- |
| `QTuiDataWidgetMapper(QObject *parent = nullptr)` | Makes a mapper. |
| `void setModel(QAbstractItemModel *model)` | Sets the model. The current index becomes -1; call `toFirst()` to show a row. |
| `QAbstractItemModel *model() const` | Returns the model. |
| `void addMapping(QTuiWidget *widget, int section)` | Maps the widget to a column. A widget has one mapping only. If a row is current, the widget gets its value at once. |
| `void removeMapping(QTuiWidget *widget)` | Removes the mapping of the widget. |
| `void clearMapping()` | Removes all mappings. |
| `int currentIndex() const` | Returns the current row, or -1. Also a property. |
| `SubmitPolicy submitPolicy() const` | Returns the submit policy. |
| `void setSubmitPolicy(SubmitPolicy policy)` | Sets the submit policy. |

### Slots

| Slot | Description |
| --- | --- |
| `void toFirst()` | Makes row 0 current. |
| `void toLast()` | Makes the last row current. |
| `void toNext()` | Makes the next row current, if there is one. |
| `void toPrevious()` | Makes the previous row current, if there is one. |
| `void setCurrentIndex(int index)` | Makes the row current and fills the widgets. A row out of range has no effect. |
| `bool submit()` | Writes the values of all widgets to the model. Returns `false` if one `setData()` fails. |
| `void revert()` | Fills the widgets again from the model. |

### Signals

| Signal | Description |
| --- | --- |
| `currentIndexChanged(int index)` | The current row changed. |

When the model emits `dataChanged` for the current row, the mapper fills the
mapped widgets of the changed columns again.
