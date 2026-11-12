# Model/view

This page tells how the views show a Qt model, how selection, editing and
context menus work, and documents the view, header, delegate and mapper
classes. For views that keep their own items, see [item widgets](item-widgets.md).

## How the views use a model

A view does not copy the data of the model. Each paint reads the model again
with `QAbstractItemModel::data()`. A view shows the `Qt::DisplayRole` text
of each item, in the colors of `Qt::ForegroundRole` and `Qt::BackgroundRole`
(see [item colors](#item-colors)). Before the text, the list, table and
tree views show the check box of an item that has a `Qt::CheckStateRole`
value, and the `Qt::DecorationRole` value if it is a string (see
[check boxes and glyphs](#check-boxes-and-glyphs)). The item delegate writes
each item in these views (see [QTuiItemDelegate](#qtuiitemdelegate)).

`setModel()` connects the view to the model signals in the table below. When
a signal comes, the view marks only the screen rows that changed as dirty,
and the next render pass paints them. See
[events and rendering](events-and-rendering.md) for the render pass.

| Model signal | What the view repaints |
| --- | --- |
| `dataChanged` | The changed rows. `QTuiColumnView` repaints all. |
| `rowsInserted`, `rowsRemoved` | The rows from the first changed row to the bottom of the view. |
| `columnsInserted`, `columnsRemoved` | All (`QTuiTableView` and `QTuiTreeView`). |
| `headerDataChanged` | The header row (`QTuiTableView` and `QTuiTreeView`), or all for the vertical header of a table if it shows. |
| `layoutChanged`, `modelReset` | All. |

The current item follows its item through inserts, removes, moves and
sorts. A model reset makes the first item current and closes the editor
without a change. The view does not take ownership of the model. If the
model is deleted, the view no longer uses it.

Use any `QAbstractItemModel`. QtTUI uses Qt Core only, so
`QStandardItemModel` is not available. Use `QStringListModel`, a
`QSortFilterProxyModel`, or your own model.

## Root index

`setRootIndex()` sets the parent of the rows that a view shows, as in Qt.
The default is the invalid index: the view shows the top-level rows. With a
root, `QTuiListView` and `QTuiTableView` show the children of the root,
`QTuiTreeView` shows the tree below the root, and `QTuiColumnView` shows the
children of the root in its first column. `QTuiTableView` also counts the
columns below the root.

A new root makes its first row current and shows all [hidden rows](#hidden-rows)
again. The view ignores the model signals for rows of a different parent.
If the model removes the root or resets, the root is again the invalid
index. `setModel()` also resets the root. An index of a different model has
no effect.

```cpp
// Show the files of one directory of a tree model in a list
list->setModel(treeModel);
list->setRootIndex(treeModel->index(2, 0));
```

## Item colors

`itemStyle()` reads `Qt::ForegroundRole` (the text color) and
`Qt::BackgroundRole` of each item. `QColor` is in Qt GUI, so the view
accepts these values:

| Value | Example |
| --- | --- |
| A `QtTUI::Color` | `QVariant::fromValue(Color(255, 0, 0))` |
| An integer `0xRRGGBB` (`int`, `uint`, `qlonglong` or `qulonglong`) | `0xff0000` |
| A string `"#RRGGBB"` | `QStringLiteral("#ff0000")` |
| A `QColor` | Only if the application links Qt GUI, which converts a `QColor` to `"#RRGGBB"`. |

The view ignores other values, for example a `QBrush`. It also ignores
`Qt::FontRole`, because `QFont` is in Qt GUI.

The selected items and the current item keep the focus colors of the
theme, as in Qt, where the selection replaces the item colors. In the
`NoColor` mode (`NO_COLOR`, `QTTUI_COLOR=none`), the view ignores the role
colors.

With `setAlternatingRowColors(true)`, each second row that shows has a
different shade of the background, as in Qt. The shade is the theme
background with one part in eight of the theme foreground. A
`Qt::BackgroundRole` color, the selection and the current item come before
the shade. A hidden row does not count. In the tree, the lines alternate.
The `NoColor` mode shows no shade, so the rows look the same as without
the property.

`itemStyle()` is virtual. Override it in a subclass of a view to use a
different rule, for example a color for each log level:

```cpp
class LogView : public QtTUI::QTuiTableView {
protected:
    QtTUI::Cell itemStyle(const QModelIndex &index, bool isCurrent,
                          const QtTUI::ThemeProperties &theme) const override
    {
        QtTUI::Cell style = QTuiTableView::itemStyle(index, isCurrent, theme);
        if (!isSelected(index) && index.siblingAtColumn(1).data().toString() == QLatin1String("ERROR"))
            style.foreground = QtTUI::Color(255, 80, 80);
        return style;
    }
};
```

## Check boxes and glyphs

An item that has a `Qt::CheckStateRole` value shows a check box before its
text: `[ ]` (`Qt::Unchecked`), `[x]` (`Qt::Checked`) or `[-]`
(`Qt::PartiallyChecked`). An item without a value shows no check box.

Space toggles the check box of the current item, and a left click on the
check box toggles that item. The view calls `setData()` of the model with
`Qt::CheckStateRole`, as Qt does. The item must have the flags
`Qt::ItemIsUserCheckable` and `Qt::ItemIsEnabled`. With
`Qt::ItemIsUserTristate`, the item goes through the three states.

As in Qt, Space on a current item with a check box does not change the
selection. Ctrl+Space toggles the selection of such an item in
`MultiSelection` and `ExtendedSelection`. Different from Qt: a click on the
check box also makes the item current and selects it.

`QIcon` is in Qt GUI, so a view shows a `Qt::DecorationRole` value only if
it is a string: a glyph, for example an emoji or a Nerd Font symbol. The
glyph shows after the check box and before the text, with a space after it.

```cpp
QVariant data(const QModelIndex &index, int role) const override
{
    if (role == Qt::CheckStateRole)
        return m_done.at(index.row()) ? Qt::Checked : Qt::Unchecked;
    if (role == Qt::DecorationRole)
        return QStringLiteral("★");
    // ...
}
```

`QTuiColumnView` does not show check boxes or glyphs.

## Long text

A text that is too long for its item ends in "…". `setTextElideMode()` sets
where the "…" goes, as in Qt:

| Mode | "a long line of text" in 8 columns |
| --- | --- |
| `Qt::ElideRight` (the default) | `a long …` |
| `Qt::ElideLeft` | `…of text` |
| `Qt::ElideMiddle` | `a lo…ext` |
| `Qt::ElideNone` | `a long l` |

`Qt::ElideLeft` is useful for paths. The mode applies to the items of the
list, table and tree views. The headers always use `Qt::ElideRight`, and
`QTuiColumnView` always cuts the text.

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

## Hidden rows

`QTuiListView` and `QTuiTableView` have `setRowHidden(row, hide)` and
`isRowHidden(row)`, and the table also has `hideRow()` and `showRow()`.
`QTuiTreeView` has `setRowHidden(row, parent, hide)` and
`isRowHidden(row, parent)`; a hidden item also hides its children.

A hidden row takes no line. The keys, the pages, the wheel, a click,
`scrollTo()` and `visualRect()` all go past it. A hidden row cannot be
current: if the current row becomes hidden, the next row that shows becomes
current. An editor on a row that becomes hidden writes its value and closes.

A range that Shift or a drag selects does not contain the hidden rows, and
`selectAll()` does not select them. Different from Qt: `QTableView` selects
the hidden rows in a range and removes them only in `selectedIndexes()`.

The list and the table keep the hidden rows as sorted row numbers. Each
paint and each key costs a search in the hidden rows, not a loop on the rows
of the model. When the model inserts or removes rows, the hidden rows move
with their items. A model reset and `setModel()` show all rows again.
Different from Qt: a sort (`layoutChanged`) keeps the hidden row numbers,
not the items. In the tree, the hidden items follow a sort.

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
not set a delegate, the view uses a `QTuiStyledItemDelegate`. A delegate
for one row (`setItemDelegateForRow()`) comes before a delegate for one
column (`setItemDelegateForColumn()`), and that comes before the delegate
of the view, as in Qt. The editor
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
| `CursorAction` | `MoveUp`, `MoveDown`, `MoveLeft`, `MoveRight`, `MoveHome`, `MoveEnd`, `MovePageUp`, `MovePageDown` (as in Qt), `MoveHalfPageUp`, `MoveHalfPageDown` |

### Properties

`selectionMode`, `selectionBehavior`, `editTriggers`, `wheelScrollLines`,
`alternatingRowColors`, `textElideMode`.

### Functions

| Function | Description |
| --- | --- |
| `QTuiAbstractItemView(QTuiWidget *parent = nullptr)` | Makes the base of a view. Call it from the constructor of your view. |
| `virtual void setModel(QAbstractItemModel *model)` | Sets the model and makes a new selection model. Each view implements it. |
| `QAbstractItemModel *model() const` | Returns the model, or `nullptr`. |
| `virtual QModelIndex currentIndex() const` | Returns the current item. |
| `virtual void setCurrentIndex(const QModelIndex &index)` | Makes the item current and scrolls to it. It also selects the item, but not in `NoSelection` or `MultiSelection`. |
| `virtual void setRootIndex(const QModelIndex &index)` | Sets the parent of the rows that the view shows. See [root index](#root-index). |
| `QModelIndex rootIndex() const` | Returns the root index. The default is the invalid index. |
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
| `void setItemDelegate(QTuiItemDelegate *delegate)` | Sets the delegate that paints the items and makes the editors. The view does not take ownership. An open editor closes without a change. |
| `QTuiItemDelegate *itemDelegate() const` | Returns the delegate that you set, or `nullptr`. |
| `void setItemDelegateForRow(int row, QTuiItemDelegate *delegate)` | Sets a delegate for the items of one model row. `nullptr` removes it. The view does not take ownership. |
| `QTuiItemDelegate *itemDelegateForRow(int row) const` | Returns the delegate of the row, or `nullptr`. |
| `void setItemDelegateForColumn(int column, QTuiItemDelegate *delegate)` | Sets a delegate for the items of one model column. `nullptr` removes it. The view does not take ownership. |
| `QTuiItemDelegate *itemDelegateForColumn(int column) const` | Returns the delegate of the column, or `nullptr`. |
| `virtual QTuiItemDelegate *itemDelegateForIndex(const QModelIndex &index) const` | Returns the delegate that the view uses for the item: of its row, then of its column, then of the view, then the default `QTuiStyledItemDelegate`. It does not return `nullptr`. |
| `bool edit(const QModelIndex &index)` | Opens an editor on the item. Returns `false` if the item is not editable, not visible, or the delegate makes no editor. |
| `bool isEditing() const` | Returns `true` if an editor is open. |
| `QTuiWidget *editor() const` | Returns the open editor, or `nullptr`. |
| `void closeEditor(bool commit = true)` | Closes the editor. With `commit`, the editor writes its value to the model first. |
| `virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Scrolls the view so that the item is visible. |
| `virtual QRect visualRect(const QModelIndex &index) const` | Returns the screen area of the item, or an empty rectangle if the item is not visible. |
| `bool alternatingRowColors() const` | Returns `true` if each second row has a different shade. The default is `false`. |
| `void setAlternatingRowColors(bool enable)` | Turns the shade of each second row on or off. See [item colors](#item-colors). |
| `Qt::TextElideMode textElideMode() const` | Returns where the "…" goes in a long text. The default is `Qt::ElideRight`. |
| `void setTextElideMode(Qt::TextElideMode mode)` | Sets where the "…" goes. See [long text](#long-text). |
| `int wheelScrollLines() const` | Returns the rows that one notch of the mouse wheel scrolls. The default is 3. |
| `void setWheelScrollLines(int lines)` | Sets the rows for one notch. The minimum is 1. |

### Protected functions

Use these functions when you write a view.

| Function | Description |
| --- | --- |
| `void resetSelectionModel()` | Makes a new selection model. Call it at the end of `setModel()`, after you connect to the model signals. |
| `void resetRootIndex()` | Sets the root to the invalid index and shows all rows. Call it in `setModel()`, after you set `m_model` and before you read the new model. |
| `void selectCurrent(Qt::KeyboardModifiers modifiers, bool click)` | Updates the selection for the mode and the modifiers. Call it after a key or a click makes a different item current. |
| `bool selectionKeyPressEvent(TuiKeyEvent *event)` | Handles Space, Ctrl+Space and Ctrl+A. Returns `true` if the key changed the selection or the check state. |
| `bool toggleCheckState(const QModelIndex &index)` | Toggles the check state of the item with `setData()`. Returns `false` if the item is not user-checkable or has no check state. |
| `bool checkBoxPressEvent(const TuiMouseEvent *event, const QModelIndex &index)` | Toggles the check state if the event is a left press on the check box of the item (the first 3 cells of `visualRect()`). |
| `void clickCurrent(const TuiMouseEvent *event, bool drag)` | A view calls it after a press or a drag made an item current. It marks the view dirty and updates the selection. A left press also calls `checkBoxPressEvent()` and emits `clicked()`. |
| `bool editKeyPressEvent(TuiKeyEvent *event)` | Handles F2 and typed characters. Returns `true` if the key opened an editor. |
| `bool cursorAction(const TuiKeyEvent *event, CursorAction *action, Qt::KeyboardModifiers *modifiers) const` | Reads the move of a navigation key (see [keys](#keys)). Returns `false` for other keys. `*modifiers` gets the modifiers to give to `selectCurrent()`. |
| `static int movedRow(CursorAction action, int row, int rowCount, int visibleRows)` | Returns the view row after a vertical move. A page is `visibleRows - 1` rows. |
| `bool wheelScroll(const TuiMouseEvent *event, int *offset, int visibleRows, int rowCount)` | Scrolls `*offset` for a wheel event, with the limits of `scrollToRow()`. Returns `false` if the event is not a wheel event. |
| `void scrollToRow(int row, int visibleRows, int rowCount, ScrollHint hint)` | Moves the scroll offset to show the row, as the hint tells. |
| `void markRowsDirty(int first, int last, int top = 0)` | Marks the visible rows from `first` to `last` as dirty. `top` is the number of lines above the first row (for example a header). |
| `bool isHiddenRow(int row) const` | Returns `true` if the row is hidden. |
| `bool setHiddenRow(int row, bool hide)` | Hides or shows a row. Returns `false` if the row already has that state. The view must then repaint and move its current row. |
| `void clearHiddenRows()` | Shows all rows. |
| `int shownRows() const` | Returns the number of rows that show. |
| `int visualRow(int row) const` | Returns the view row of a model row: its position in the rows that show. `movedRow()`, `wheelScroll()`, `scrollToRow()` and `markRowsDirty()` use view rows. |
| `int logicalRow(int visualRow) const` | Returns the model row of a view row. |
| `int shownRowNear(int row) const` | Returns the row that shows at `row` or after it, or the last row that shows, or -1. |
| `void clampScrollOffset(int visibleRows)` | Keeps the scroll offset in the rows that show. |
| `void shiftHiddenRows(int first, int count)` | Moves the hidden rows after the model inserts (`count` > 0) or removes (`count` < 0) rows at `first`. Call it first in the `rowsInserted` and `rowsRemoved` slots. |
| `QTuiHeaderView *createColumnHeader()` | Makes the header of the columns. Call it in the constructor of a view that has columns. The functions below need it. `QTuiTableView` and `QTuiTreeView` share them. |
| `int headerHeight() const` | Returns 1 if the header row shows, or 0. |
| `virtual int columnsLeft() const` | Returns the cells at the left of the first column. The default is 0; the table returns the width of its vertical header. |
| `int horizontalScrollOffset() const`, `int firstVisibleColumn() const` | Return the first column that shows after the horizontal scroll. |
| `int columnWidth(int column) const`, `void setColumnWidth(int column, int width)`, `void resizeColumnToContents(int column)`, `void resizeColumnsToContents()`, `bool isColumnHidden(int column) const` | The column functions that the table and the tree make public. |
| `int movedColumn(int column, int step) const` | Returns the next column that shows in the direction of `step` (1 or -1), or `column`. |
| `void ensureColumnVisible(int column)` | Scrolls by column until the column shows. |
| `void invalidateColumnWidths()` | Makes the next paint calculate the widths again. |
| `virtual void remeasureColumnWidths()` | Discards the stored natural widths. |
| `void growColumnWidth(int column, int width)` | Makes the stored natural width of the column wider. It does not make it narrower. |
| `virtual int measureColumn(int column) const` | Returns the natural width of a column. The default measures the header; a view adds the cells that show. |
| `const QVector<int> &effectiveColumnWidths() const` | Returns the width of each column, from the resize modes and the natural widths. |
| `QList<ColumnSpan> visibleColumns() const` | Returns the columns that show, with the screen x and the width of each. Painting, hit tests and `visualRect()` use it. |
| `int columnAt(int x, bool *separator = nullptr, int *columnX = nullptr) const` | Returns the column at a screen x, or -1 at the left of the first column. |
| `void paintHeaderRow(ScreenBuffer &buffer, const ThemeProperties &theme) const` | Paints the header row with the header view, if it shows. |
| `bool resizeColumnKeyEvent(const TuiKeyEvent *event, int column)` | Handles Alt+Left and Alt+Right for the column. |
| `bool headerResizeMouseEvent(const TuiMouseEvent *event)` | Handles a drag on the separator after a header. |
| `bool horizontalWheelEvent(const TuiMouseEvent *event)` | Handles Shift+wheel. |
| `void updateEditorGeometry()` | Moves the editor to its item. If the item is not visible, it closes the editor and writes the value. |
| `bool eventFilter(QObject *watched, QEvent *event) override` | Gives Enter and Escape in the editor to the view. |
| `bool isSelected(const QModelIndex &index) const` | Returns `true` if the item is selected. |
| `virtual Cell itemStyle(const QModelIndex &index, bool isCurrent, const ThemeProperties &theme) const` | Returns the colors and attributes of an item, as the selection and [item colors](#item-colors) sections tell. |
| `void paintItem(ScreenBuffer &buffer, const QRect &rect, const QModelIndex &index, const Cell &style) const` | Calls `paint()` of the delegate of the item, with the style and the elide mode of the view. Fill the rectangle first. |
| `virtual bool isAlternateRow(const QModelIndex &index) const` | Returns `true` if `alternatingRowColors` is on and the item is on a second row. `itemStyle()` calls it. |
| `static Color alternateBackground(const ThemeProperties &theme)` | Returns the background of the alternate rows. |
| `virtual QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const` | Returns the items from `from` to `to`. The default is the rectangle between the two items if they have the same parent. |

### Signals

| Signal | Description |
| --- | --- |
| `clicked(const QModelIndex &index)` | A left click on an item. A drag does not emit it. |
| `activated(const QModelIndex &index)` | Enter on an item, or a click or double click. Each view section tells which. |
| `doubleClicked(const QModelIndex &index)` | A double click on an item. |

### Keys

These keys work in all views. Shift with a move key selects a range, as
Shift+Up and Shift+Down do. Ctrl with a move key (not the vim Ctrl keys)
moves without a change to the selection (`ExtendedSelection`), as in Qt.

| Key | Action |
| --- | --- |
| Page Up, Page Down | Move the current item by one page: the visible rows minus one. The item stays visible. |
| Home, End (also with Ctrl) | Go to the first or the last row. |
| j, k | Move down or up (as Down and Up). J and K also select a range. |
| h, l | As Left and Right. |
| g (or gg), G | Go to the first or the last row. |
| Ctrl+F, Ctrl+B | Move one page down or up. |
| Ctrl+D, Ctrl+U | Move half a page down or up. |
| Space | Toggles the check box of the current item, if it has one. If not, selects the current item; in `MultiSelection`, toggles it. |
| Ctrl+Space | Toggles the selection of the current item in `MultiSelection` and `ExtendedSelection`. |
| Ctrl+A | Selects all items in the three multiple modes. |
| F2 | Opens the editor (`EditKeyPressed`). |
| A printable character | Opens the editor and types the character (`AnyKeyPressed`). |
| Enter (in the editor) | Writes the value and closes the editor. |
| Escape (in the editor) | Closes the editor without a change. |
| Shift+F10, Menu | Asks for the context menu. |

The vim letters move only if they do not open an editor: with the
`AnyKeyPressed` edit trigger, a letter opens the editor. A shortcut of an
action also comes before the view.

### Mouse wheel

The wheel scrolls the view by `wheelScrollLines` rows (3) for each notch
(`wheelDelta() / 120`). It does not change the current item, as in Qt. The
limits are the same as for `scrollTo()`. In `QTuiTableView`, Shift+wheel
scrolls by one column. Some terminals keep Shift+wheel for their own
scrollback. `QTuiTreeView` does the same. In `QTuiColumnView`, the wheel scrolls the column under the
mouse. The next key that moves the current item scrolls back to it.

## QTuiListView

`#include <QtTUI/QTuiListView.h>` · Inherits: `QTuiAbstractItemView`

Shows column 0 of the rows below the [root index](#root-index) of a model
(by default the top-level rows), one row for each line.
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
| `void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible)` | Scrolls to a row of the root. |
| `QRect visualRect(const QModelIndex &index) const` | Returns the line of the row. |
| `QRect hoverRect() const override` | Returns the row under the mouse, for the hover highlight. |
| `bool isRowHidden(int row) const` | Returns `true` if the row is hidden. |
| `void setRowHidden(int row, bool hide)` | Hides or shows the row. See [hidden rows](#hidden-rows). |
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

Shows a table model with a header row. A space separates the columns.

The view has two `QTuiHeaderView` objects, as `QTableView` has:
`horizontalHeader()` for the columns and `verticalHeader()` for the rows.
The table gives each header its place and paints it with the paint
functions of the header, so the headers are not focus stops and do not take
the mouse. The keys and the mouse of the table do the header actions (see
below).

- `horizontalHeader()->setVisible(false)` hides the header row.
- `verticalHeader()->setVisible(true)` shows a column at the left with the
  vertical header data of the model. The default data of a model is the row
  number. Different from Qt: the vertical header is hidden by default.

The width of the vertical header does not change when the view scrolls. It
has space for the labels of the rows that show at the first paint, and for
a row number of 7 digits. Thus it also does not change while a model grows,
up to 9,999,999 rows. The view measures the labels again when the model emits
`headerDataChanged` for the rows. A click on the vertical header does
nothing. `verticalHeader()->setSectionHidden()` has no effect on the table:
use `setRowHidden()`.

```cpp
table->verticalHeader()->setVisible(true);   // row numbers
table->horizontalHeader()->setVisible(false); // no header row
```

The resize modes of the horizontal header set the column widths:

| Mode | Width of the column |
| --- | --- |
| `Interactive` (the default) | The natural width, until `setColumnWidth()`, `resizeColumnToContents()`, a drag on the header, Alt+Left or Alt+Right sets a width. |
| `Fixed` | `sectionSize()` of the header. The user cannot change it. |
| `Stretch` | The natural width and a part of the free width. The user cannot change it. |
| `ResizeToContents` | The widest of the header and the visible cells. It changes when the view scrolls or the data changes. The user cannot change it. |

The natural width is the widest of the header and the cells that show when
the model has its first rows. The view measures it one time. After that, a
scroll or a change of the data does not change the width. A model reset, a
change of the column count and `setSortingEnabled()` make the view measure
again. Before the model has rows, the width is the width of the header text.
The view reads only the visible rows, so a model with millions of rows stays
fast. With `setStretchLastSection(true)`, the last column also gets the free
width. If the columns do not fit in the view, they keep their widths and the
view scrolls by column: the view shows the current column.

```cpp
QTuiHeaderView *header = table->horizontalHeader();
header->setSectionResizeMode(0, QTuiHeaderView::Fixed);
header->resizeSection(0, 8);
header->setSectionResizeMode(1, QTuiHeaderView::ResizeToContents);
header->setStretchLastSection(true);
```

`setSpan(row, column, rowSpan, columnSpan)` makes one cell cover more rows
and columns, as in `QTableView`. The cell has the width of its columns and
their separators. Its text shows on the first row of the span; the other
rows of the span are empty, in the colors of the cell. The keys and a click
in a span go to its first cell: Down and Right go to the cell after the
span. `setSpan(row, column, 1, 1)` removes a span.

```cpp
table->setSpan(0, 0, 1, 3); // a title over three columns
table->setSpan(1, 0, 4, 1); // one label for four rows
```

The view keeps the spans as row and column numbers. Rows that the model
inserts or removes before a span move it, and removed rows in a span remove
it. A model reset, `setModel()`, `setRootIndex()` and a change of the column
count remove all spans. Do not make spans that overlap, and do not hide the
first row of a span: its text is on that row. Each cell of a paint searches
the list of spans, so use tens of spans, not thousands.

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
| `void setColumnWidth(int column, int width)` | Sets the size of the header section (the minimum is 2). An `Interactive` or a `Fixed` column uses it. |
| `bool isColumnHidden(int column) const` | Returns true if the column is hidden. |
| `void setColumnHidden(int column, bool hide)` | Hides or shows the column. A hidden column takes no space, and the Left and Right keys go past it. If the current column becomes hidden, the next column that shows becomes current. `horizontalHeader()->setSectionHidden()` does the same. |
| `void hideColumn(int column)` | Calls `setColumnHidden(column, true)`. |
| `void showColumn(int column)` | Calls `setColumnHidden(column, false)`. |
| `bool isRowHidden(int row) const` | Returns `true` if the row is hidden. |
| `void setRowHidden(int row, bool hide)` | Hides or shows the row. See [hidden rows](#hidden-rows). |
| `void hideRow(int row)` | Calls `setRowHidden(row, true)`. |
| `void showRow(int row)` | Calls `setRowHidden(row, false)`. |
| `void setSpan(int row, int column, int rowSpan, int columnSpan)` | Makes the cell cover `rowSpan` rows and `columnSpan` columns. A span of 1, 1 removes the span of the cell. |
| `int rowSpan(int row, int column) const` | Returns the row count of the span that has the cell, or 1. |
| `int columnSpan(int row, int column) const` | Returns the column count of the span that has the cell, or 1. |
| `void clearSpans()` | Removes all spans. |
| `void resizeColumnToContents(int column)` | Sets the width of the column to the widest of the header and the visible cells, as `setColumnWidth()` does. A cell in a span does not count. |
| `void resizeColumnsToContents()` | Calls `resizeColumnToContents()` for each column. |
| `QTuiHeaderView *horizontalHeader() const` | Returns the header of the columns. It keeps the resize modes, the sizes and the sort indicator. Hide it to hide the header row. |
| `QTuiHeaderView *verticalHeader() const` | Returns the header of the rows. It is hidden by default; show it to see the row numbers or the row labels of the model. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the header and the visible rows. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down, Left, Right | Move the current cell. A move into a span goes to its first cell, and a move from a span starts at its edge. With Shift, select a range. With Ctrl, move without a change to the selection (`ExtendedSelection`). |
| Home, End, Ctrl+Home, Ctrl+End | Go to the first or the last row. The column does not change, because a cell has no text cursor. |
| Alt+Left, Alt+Right | Make the current column narrower or wider by 1. Only an `Interactive` column. |
| Alt+Up, Alt+Down | Sort by the current column, ascending or descending. Only if sorting is enabled. |
| Enter | Emits `activated()`. |

### Mouse

- A left click on a cell makes it current, selects it and emits `clicked()`. A click in a span does this for the first cell of the span. A click on the separator after a column goes to that column. A single click does not emit `activated()`.
- A left drag selects the range from the pressed cell to the cell under the mouse.
- A click on a header sorts by that column, if sorting is enabled. A second click reverses the order.
- A left drag that starts on the separator after a header sets the width of that column. Only an `Interactive` column.
- The wheel scrolls the rows. Shift+wheel scrolls the columns.
- A double click emits `doubleClicked()`. Then it opens the editor (`DoubleClicked`). If no editor opens, it emits `activated()`.

## QTuiTreeView

`#include <QtTUI/QTuiTreeView.h>` · Inherits: `QTuiAbstractItemView`

Shows a tree model with all its columns. In column 0, each level has an
indent of 2 cells, and an item with children has "▶" (collapsed) or "▼"
(expanded) before its text. A space separates the columns. The item
delegate writes each item; in column 0, after the indent and the indicator.

The view has a header row, which `header()` returns as a `QTuiHeaderView`.
Different from Qt: the header row is hidden by default, so a tree with one
column looks like a list with branches. `setHeaderHidden(false)` shows it.

The columns follow the same rules as in [QTuiTableView](#qtuitableview):
the resize modes of the header, the natural width that the view measures
one time, Alt+Left and Alt+Right, a drag on the header, hidden columns and
the scroll by column. Two things are different:

- The last column gets the free width (`stretchLastSection` is `true`), as
  in Qt. Thus a tree with one column uses the full width.
- When an item expands, the columns get wider if one page of its children
  needs more space (the children have an indent). The columns do not get
  narrower, and a scroll does not change them. A width that the user or
  `setColumnWidth()` set stays.

The tree has a current column. With the default `SelectItems`, only the
current cell has the selection colors. Use
`setSelectionBehavior(QTuiAbstractItemView::SelectRows)` to select the
full row, which is the default look of `QTreeView`.

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
| `QTuiHeaderView *header() const` | Returns the header of the columns. It keeps the resize modes, the sizes and the hidden state. |
| `bool isHeaderHidden() const` | Returns `true` if the header row does not show. The default is `true`. |
| `void setHeaderHidden(bool hide)` | Hides or shows the header row. |
| `int columnWidth(int column) const` | Returns the width of the column as the view paints it. |
| `void setColumnWidth(int column, int width)` | Sets the size of the header section (the minimum is 2). An `Interactive` or a `Fixed` column uses it. |
| `void resizeColumnToContents(int column)` | Sets the width of the column to the widest of the header and the visible items. |
| `bool isColumnHidden(int column) const` | Returns `true` if the column is hidden. |
| `void setColumnHidden(int column, bool hide)` | Hides or shows the column. If the current column becomes hidden, the next column that shows becomes current. Column 0 has the branches: if you hide it, the tree shows no indent. |
| `void hideColumn(int column)` | Calls `setColumnHidden(column, true)`. |
| `void showColumn(int column)` | Calls `setColumnHidden(column, false)`. |
| `int horizontalScrollOffset() const` | Returns the first visible column. |
| `void expand(const QModelIndex &index)` | Shows the children of the item and emits `expanded()`. The current item stays current. |
| `void collapse(const QModelIndex &index)` | Hides the children of the item and emits `collapsed()`. If the current item is below the item, the item becomes current, as in Qt. |
| `bool isExpanded(const QModelIndex &index) const` | Returns `true` if the item is expanded. |
| `bool isRowHidden(int row, const QModelIndex &parent) const` | Returns `true` if the item is hidden. |
| `void setRowHidden(int row, const QModelIndex &parent, bool hide)` | Hides or shows the item and its children. See [hidden rows](#hidden-rows). |
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
| Up, Down | Move the current item. The column stays. With Shift, select a range. |
| Right | Expands the item. If it is expanded, moves to its first child. On an item without children, moves to the next column, as in Qt. |
| Left | Moves to the column before. In the first column: collapses the item, or moves to its parent if it is collapsed or has no children. |
| Alt+Left, Alt+Right | Make the current column narrower or wider by 1. Only an `Interactive` column. |
| Enter | Expands or collapses an item with children, and emits `activated()`. |

### Mouse

- A left click makes the item and its column current, selects it and emits `clicked()`.
- A left drag that starts on the separator after a header sets the width of that column. Only an `Interactive` column. A click on a header does nothing else.
- The wheel scrolls the rows. Shift+wheel scrolls the columns.
- A left drag selects the range from the pressed item to the item under the mouse.
- A double click emits `doubleClicked()`. Then it opens the editor (`DoubleClicked`). If no editor opens, it expands or collapses the item and emits `activated()`.

## QTuiColumnView

`#include <QtTUI/QTuiColumnView.h>` · Inherits: `QTuiAbstractItemView`

Shows a tree model as columns, as in `QColumnView`. The first column shows
the top-level items. The next column shows the children of the current item
of the column before it. A "│" line separates the columns. An item with
children has " ▶" after its text. Each column scrolls vertically to show its
current item. The wheel scrolls the column under the mouse.

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
| Up, Down, Page Up, Page Down, Home, End | Move the current item in the active column. The next column shows its children. |
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
You can put a header in a layout as a widget of its own; it then takes the
focus and the keys below. Different from Qt: it does not take the mouse.

`QTuiTableView` has a header for its columns (`horizontalHeader()`) and one
for its rows (`verticalHeader()`). The table uses the resize modes, the
sizes, the hidden state and the sort indicator of the horizontal header,
and paints both headers with `paintSections()` and `paintSection()`. A
header of a table is not a focus stop.

Each section has a size. The default size is 10 and the minimum is 2. The
header keeps a size, a mode and a hidden state only for the sections that
have one, so a vertical header of a model with millions of rows is cheap.

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
| `int count() const` | Returns the number of sections: the columns or the rows of the model, below the root index. |
| `void setRootIndex(const QModelIndex &index)` | Sets the parent of the columns or rows that the header counts. `QTuiTableView::setRootIndex()` calls it. |
| `QModelIndex rootIndex() const` | Returns the root index. |
| `void setSectionResizeMode(ResizeMode mode)` | Sets the resize mode of all sections: `Interactive` (the default), `Stretch`, `Fixed` or `ResizeToContents`. The values are the same as in Qt. See [QTuiTableView](#qtuitableview). |
| `void setSectionResizeMode(int logicalIndex, ResizeMode mode)` | Sets the resize mode of one section. |
| `ResizeMode sectionResizeMode(int logicalIndex) const` | Returns the resize mode of the section. |
| `bool isSectionResized(int logicalIndex) const` | Returns `true` after `resizeSection()` set the size of the section. |
| `bool stretchLastSection() const` | Returns `true` if the last section gets the free width. |
| `void setStretchLastSection(bool stretch)` | Gives the free width also to the last section (also a property). The default is `false`. |
| `void setSortIndicator(int logicalIndex, Qt::SortOrder order)` | Sets the section and the order of the sort indicator: "▲" (ascending) or "▼" (descending) after the text of the section. It does not sort; `QTuiTableView::sortByColumn()` calls it. |
| `int sortIndicatorSection() const` | Returns the section of the sort indicator, or -1. |
| `Qt::SortOrder sortIndicatorOrder() const` | Returns the order of the sort indicator. |
| `bool isSortIndicatorShown() const` | Returns `true` if the sort indicator shows. The default is `false`. |
| `void setSortIndicatorShown(bool show)` | Shows or hides the sort indicator. `QTuiTableView::setSortingEnabled()` calls it. |
| `void paintSection(ScreenBuffer &buffer, const QRect &rect, int logicalIndex, const Cell &style) const` | Fills `rect` with `style` and writes the text of the section and its sort indicator. A text that is too long ends in "…". |
| `void paintSections(ScreenBuffer &buffer, const QRect &rect, int first, const QList<int> &sizes, int spacing, const Cell &style) const` | Paints the sections from `first` in one row. `sizes` has the width of each section, and `spacing` cells separate two sections. A hidden section takes no space. A view calls it to paint its header row. |
| `void paintEvent(ScreenBuffer &buffer) override` | Paints the labels. The focused section has the focus colors. |
| `QSize minimumSize() const override` | Horizontal: the sum of the visible section sizes. Vertical: the longest label and the number of visible sections. |

### Signals

| Signal | Description |
| --- | --- |
| `sectionClicked(int logicalIndex)` | Enter on the focused section. |
| `sectionResized(int logicalIndex, int oldSize, int newSize)` | The size of a section changed. |
| `geometriesChanged()` | A resize mode or `stretchLastSection` changed, or `resizeSection()` set a section for the first time. |

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

The base class of the delegates. A delegate writes the text of the items of
a view and makes the editors. `QTuiListView`, `QTuiTableView` and
`QTuiTreeView` call `paint()` for each item that shows. The view first fills
the area of the item with the colors of the item, and then the delegate
writes the text. Different from Qt: `QTuiColumnView` always paints the text
itself, and only the table view calls `sizeHint()`, to measure a column. The base class makes no editor, so a
view with a `QTuiItemDelegate` cannot edit.

`paint()` gets a `QTuiStyleOptionViewItem`, like `QStyleOptionViewItem`:

| Member | Description |
| --- | --- |
| `QRect rect` | The area of the item text, one line high. In a tree, it starts after the indent and the indicator. |
| `Cell style` | The colors and attributes of the item, from `itemStyle()` of the view: the selection, the current item, the role colors and the alternate shade. |
| `Qt::TextElideMode textElideMode` | The elide mode of the view. |

Up to version 0.2.1, `paint()` had a `const QRect &rect` argument and no
option, and only the tree view called it. A delegate that overrides the old
function does not compile with `override`. Change the argument to
`const QTuiStyleOptionViewItem &option` and use `option.rect`.

### Example

```cpp
#include <QtTUI/QtTUI>
#include <QtTUI/RenderUtils.h>

using namespace QtTUI;

// Paints the item text in upper case. An item that is not selected is yellow.
class UpperDelegate : public QTuiStyledItemDelegate {
public:
    using QTuiStyledItemDelegate::QTuiStyledItemDelegate;

    void paint(ScreenBuffer &buffer, const QTuiStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        Cell style = option.style;
        if (style.background == Theme_Engine::instance()->properties("QTuiTableView").background)
            style.foreground = Color(255, 220, 0);
        renderTextTruncated(buffer, option.rect.x(), option.rect.y(),
                            index.data().toString().toUpper(), option.rect.width(),
                            style, option.textElideMode);
    }
};

void example(QTuiTableView *table)
{
    // Only the items of column 1 use the delegate
    table->setItemDelegateForColumn(1, new UpperDelegate(table));
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiItemDelegate(QObject *parent = nullptr)` | Makes a delegate. |
| `virtual void paint(ScreenBuffer &buffer, const QTuiStyleOptionViewItem &option, const QModelIndex &index) const` | Writes the check box, the glyph and the `Qt::DisplayRole` text in `option.rect` with `option.style`. A text that is too long ends in "…", as `option.textElideMode` tells. It does not fill the area: the view does that first. |
| `virtual QTuiWidget *createEditor(QTuiWidget *parent, const QModelIndex &index) const` | Returns a new editor for the item. The default returns `nullptr`. |
| `virtual void setEditorData(QTuiWidget *editor, const QModelIndex &index) const` | Copies the value of the item to the editor. The default does nothing. |
| `virtual void setModelData(QTuiWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const` | Writes the value of the editor to the model. The default does nothing. |
| `virtual QSize sizeHint(const QModelIndex &index) const` | Returns the width of the check box, the glyph and the text, and a height of 1. |
| `static QString decorationText(const QModelIndex &index)` | Returns the text that `paint()` writes before the item text: the check box and the glyph, each with a space after it. |

### Signals

| Signal | Description |
| --- | --- |
| `commitData(QTuiWidget *editor)` | Emit it to make the view write the value of the editor to the model. The editor stays open. |
| `closeEditor(QTuiWidget *editor)` | Emit it to make the view close the editor without a change. |

## QTuiStyledItemDelegate

`#include <QtTUI/QTuiStyledItemDelegate.h>` · Inherits: `QTuiItemDelegate`

The default delegate of the views. It paints as `QTuiItemDelegate` does, so
a view with a `QTuiStyledItemDelegate` that you set looks the same as a view
without a delegate. It selects the editor from the data of the item:

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
