# Item widgets

This page documents the list, table and tree widgets and their item classes.
An item widget is a view with its own model: you add items, and the widget
owns them.

Each item widget inherits a view from [model/view](model-view.md), so the
selection, editing, keys and mouse of that view also work here. The
internal model classes (`QTuiListWidgetModel`, `QTuiTableWidgetModel`,
`QTuiTreeWidgetModel`) are in the public headers, but they are not part of
the API. Do not use them.

The widgets show the `Qt::DisplayRole` text of each item. They do not show
icons or check states.

## QTuiListWidget

`#include <QtTUI/QTuiListWidget.h>` · Inherits: `QTuiListView`

A list of `QTuiListWidgetItem` objects. The widget owns its items and
deletes them when it is deleted. Different from Qt: `itemClicked()` comes
from Enter on the current item, not from a mouse click.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *list = new QTuiListWidget(&window);
    list->addItem(QStringLiteral("Read mail"));
    list->addItem(QStringLiteral("Write report"));
    auto *item = new QTuiListWidgetItem(QStringLiteral("Call Bob"));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    list->insertItem(0, item);
    list->setCurrentRow(0);

    QObject::connect(list, &QTuiListWidget::itemClicked,
                     [](QTuiListWidgetItem *clicked) {
        qDebug() << "done:" << clicked->text();
    });

    auto *layout = createVBoxLayout(&window);
    layout->addWidget(list, 1);
    window.setLayout(layout);

    list->setFocus();
    window.show();
    app.triggerRender();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiListWidget(QTuiWidget *parent = nullptr)` | Makes an empty list widget. |
| `~QTuiListWidget()` | Deletes all items. |
| `void addItem(const QString &label)` | Adds a new item with the text at the end. |
| `void addItem(QTuiListWidgetItem *item)` | Adds the item at the end. The widget takes ownership. An item that is already in the list has no effect. |
| `void insertItem(int row, QTuiListWidgetItem *item)` | Inserts the item at the row. A row out of range goes to the nearest end. The widget takes ownership. |
| `QTuiListWidgetItem *takeItem(int row)` | Removes the item from the list and returns it. You own it. Returns `nullptr` if the row is out of range. |
| `int count() const` | Returns the number of items. |
| `QTuiListWidgetItem *item(int row) const` | Returns the item at the row, or `nullptr`. |
| `QTuiListWidgetItem *currentItem() const` | Returns the current item, or `nullptr` if the list is empty. |
| `void setCurrentItem(QTuiListWidgetItem *item)` | Makes the item current. |
| `int currentRow() const` | Returns the current row, or -1 if the list is empty. |
| `void setCurrentRow(int row)` | Makes the row current and emits `currentItemChanged()` if the row changes. A row out of range has no effect. |
| `QList<QTuiListWidgetItem *> selectedItems() const` | Returns the selected items. |
| `void keyPressEvent(TuiKeyEvent *event) override` | Handles the keys of `QTuiListView` and emits the signals below. |

### Signals

| Signal | Description |
| --- | --- |
| `currentItemChanged(QTuiListWidgetItem *current, QTuiListWidgetItem *previous)` | A key or `setCurrentRow()` made a different item current. A mouse click does not emit it. |
| `itemClicked(QTuiListWidgetItem *item)` | Enter on the current item. |

The signals of `QTuiAbstractItemView` (`activated()`, `doubleClicked()`) and
of the selection model also work. Use them for the mouse.

### Keys

| Key | Action |
| --- | --- |
| Up, Down | Move the current item and emit `currentItemChanged()`. |
| Enter | Emits `activated()` and `itemClicked()`. |

The selection and editing keys of [`QTuiAbstractItemView`](model-view.md#keys)
also work.

## QTuiListWidgetItem

`#include <QtTUI/QTuiListWidgetItem.h>` · Inherits: none

One item of a `QTuiListWidget`. The default flags are
`Qt::ItemIsSelectable | Qt::ItemIsEnabled`, so an item is not editable. Add
`Qt::ItemIsEditable` to edit it in place. Different from Qt: the icon is a
string, and the check state is an `int`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiListWidget *list)
{
    // The constructor adds the item to the list
    auto *item = new QTuiListWidgetItem(QStringLiteral("Draft"), list);
    item->setData(Qt::UserRole, 42);
    item->setText(QStringLiteral("Final"));
    qDebug() << item->data(Qt::UserRole).toInt() << item->listWidget()->count();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiListWidgetItem(const QString &text = QString(), QTuiListWidget *parent = nullptr)` | Makes an item. With a parent, it adds the item to the end of that list. |
| `QString text() const` | Returns the `Qt::DisplayRole` text. |
| `void setText(const QString &text)` | Sets the text and repaints the item. |
| `QString icon() const` | Returns the `Qt::DecorationRole` string. |
| `void setIcon(const QString &icon)` | Sets the `Qt::DecorationRole` string. The view does not show it. |
| `Qt::ItemFlags flags() const` | Returns the flags. |
| `void setFlags(Qt::ItemFlags flags)` | Sets the flags. |
| `int checkState() const` | Returns the check state (0, 1 or 2). |
| `void setCheckState(int state)` | Sets the check state. It is not the `Qt::CheckStateRole` data, and the view does not show it. |
| `QVariant data(int role) const` | Returns the data for the role. |
| `void setData(int role, const QVariant &value)` | Sets the data for the role and repaints the item. |
| `QTuiListWidget *listWidget() const` | Returns the list that has the item, or `nullptr`. |

## QTuiTableWidget

`#include <QtTUI/QTuiTableWidget.h>` · Inherits: `QTuiTableView`

A table of `QTuiTableWidgetItem` objects. A cell can be empty. The widget
owns its items and deletes them when it is deleted. Different from Qt:
`cellClicked()` comes from Enter on the current cell, not from a mouse
click. The vertical header labels are stored, but the table does not show
them. The internal model does not sort, so `setSortingEnabled()` shows the
indicator but does not change the order.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiWidget *parent)
{
    auto *table = new QTuiTableWidget(3, 2, parent);
    table->setHorizontalHeaderLabels({QStringLiteral("City"), QStringLiteral("Country")});
    table->setItem(0, 0, new QTuiTableWidgetItem(QStringLiteral("Oslo")));
    table->setItem(0, 1, new QTuiTableWidgetItem(QStringLiteral("Norway")));
    table->setSelectionMode(QTuiAbstractItemView::ExtendedSelection);

    QObject::connect(table, &QTuiTableWidget::currentCellChanged,
                     [](int row, int column, int, int) {
        qDebug() << "current" << row << column;
    });
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTableWidget(QTuiWidget *parent = nullptr)` | Makes a table with 0 rows and 0 columns. |
| `QTuiTableWidget(int rows, int columns, QTuiWidget *parent = nullptr)` | Makes a table with empty cells. |
| `~QTuiTableWidget()` | Deletes all items. |
| `void setRowCount(int rows)` | Sets the number of rows. It deletes the items of removed rows and resets the model, so the current cell goes to row 0, column 0. |
| `int rowCount() const` | Returns the number of rows. |
| `void setColumnCount(int columns)` | Sets the number of columns, as `setRowCount()` does for rows. |
| `int columnCount() const` | Returns the number of columns. |
| `void setItem(int row, int column, QTuiTableWidgetItem *item)` | Puts the item in the cell. The widget takes ownership and deletes the old item. A cell out of range has no effect. |
| `QTuiTableWidgetItem *item(int row, int column) const` | Returns the item in the cell, or `nullptr`. |
| `QTuiTableWidgetItem *takeItem(int row, int column)` | Removes the item from the cell and returns it. You own it. |
| `void setHorizontalHeaderLabels(const QStringList &labels)` | Sets the column headers. Labels after the last column have no effect. |
| `void setVerticalHeaderLabels(const QStringList &labels)` | Sets the row headers. The table does not show them. |
| `QList<QTuiTableWidgetSelectionRange> selectedRanges() const` | Returns the selected ranges. |
| `QList<QTuiTableWidgetItem *> selectedItems() const` | Returns the selected cells that have an item. |
| `void keyPressEvent(TuiKeyEvent *event) override` | Handles the keys of `QTuiTableView` and emits the signals below. |

Items are editable by default. An empty cell is also editable: when you
type a value, the table makes a new item for it.

### Signals

| Signal | Description |
| --- | --- |
| `currentCellChanged(int row, int column, int previousRow, int previousColumn)` | A key made a different cell current. A mouse click does not emit it. |
| `cellClicked(int row, int column)` | Enter on the current cell. |

### Keys

| Key | Action |
| --- | --- |
| Arrow keys | Move the current cell and emit `currentCellChanged()`. |
| Enter | Emits `activated()` and `cellClicked()`. |

The keys of [`QTuiTableView`](model-view.md#qtuitableview) and of
`QTuiAbstractItemView` also work.

## QTuiTableWidgetItem

`#include <QtTUI/QTuiTableWidgetItem.h>` · Inherits: none

One cell of a `QTuiTableWidget`. The default flags are
`Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled`, as in Qt.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiTableWidget *table)
{
    auto *item = new QTuiTableWidgetItem(QStringLiteral("fixed"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); // not editable
    table->setItem(1, 1, item);
    qDebug() << item->row() << item->column() << (item->tableWidget() == table);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTableWidgetItem(const QString &text = QString())` | Makes an item with the text. |
| `QString text() const` | Returns the `Qt::DisplayRole` text. |
| `void setText(const QString &text)` | Sets the text and repaints the cell. |
| `Qt::ItemFlags flags() const` | Returns the flags. |
| `void setFlags(Qt::ItemFlags flags)` | Sets the flags. |
| `QVariant data(int role) const` | Returns the data for the role. |
| `void setData(int role, const QVariant &value)` | Sets the data for the role and repaints the cell. |
| `QTuiTableWidget *tableWidget() const` | Returns the table that has the item, or `nullptr`. |
| `int row() const` | Returns the row of the item, or -1 if it is not in a table. |
| `int column() const` | Returns the column of the item, or -1 if it is not in a table. |

## QTuiTableWidgetSelectionRange

`#include <QtTUI/QTuiTableWidgetItem.h>` · Inherits: none

One rectangle of selected cells, from `QTuiTableWidget::selectedRanges()`.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int selectedCellCount(const QTuiTableWidget *table)
{
    int cells = 0;
    for (const QTuiTableWidgetSelectionRange &range : table->selectedRanges())
        cells += range.rowCount() * range.columnCount();
    return cells;
}
```

### Functions

| Function | Description |
| --- | --- |
| `int topRow() const` | Returns the first row. |
| `int leftColumn() const` | Returns the first column. |
| `int bottomRow() const` | Returns the last row. |
| `int rightColumn() const` | Returns the last column. |
| `int rowCount() const` | Returns the number of rows. |
| `int columnCount() const` | Returns the number of columns. |

## QTuiTreeWidget

`#include <QtTUI/QTuiTreeWidget.h>` · Inherits: `QTuiTreeView`

A tree of `QTuiTreeWidgetItem` objects. The widget owns its top-level items
and deletes them when it is deleted. Different from Qt: the tree shows only
column 0 and has no header, so the header labels and the other columns are
stored but not shown. `itemClicked()` comes from Enter on the current item,
not from a mouse click.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *tree = new QTuiTreeWidget(&window);
    auto *fruit = new QTuiTreeWidgetItem({QStringLiteral("Fruit")});
    fruit->addChild(new QTuiTreeWidgetItem({QStringLiteral("Apple")}));
    fruit->addChild(new QTuiTreeWidgetItem({QStringLiteral("Pear")}));
    tree->addTopLevelItem(fruit);
    tree->addTopLevelItem(new QTuiTreeWidgetItem({QStringLiteral("Bread")}));
    tree->expand(tree->model()->index(0, 0));

    QObject::connect(tree, &QTuiTreeWidget::itemClicked,
                     [](QTuiTreeWidgetItem *item, int column) {
        qDebug() << item->text(column);
    });

    auto *layout = createVBoxLayout(&window);
    layout->addWidget(tree, 1);
    window.setLayout(layout);

    tree->setFocus();
    window.show();
    app.triggerRender();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTreeWidget(QTuiWidget *parent = nullptr)` | Makes an empty tree with 1 column. |
| `~QTuiTreeWidget()` | Deletes all items. |
| `void addTopLevelItem(QTuiTreeWidgetItem *item)` | Adds the item and its children at the end of the top level. The widget takes ownership. If the item has a parent, the item moves from that parent. |
| `QTuiTreeWidgetItem *topLevelItem(int index) const` | Returns the top-level item, or `nullptr`. |
| `int topLevelItemCount() const` | Returns the number of top-level items. |
| `QTuiTreeWidgetItem *takeTopLevelItem(int index)` | Removes the top-level item and returns it. You own it. Returns `nullptr` if the index is out of range. |
| `void setHeaderLabels(const QStringList &labels)` | Sets the header labels. The column count grows to the number of labels. It resets the model, so all items collapse. |
| `int columnCount() const` | Returns the number of columns. |
| `void setColumnCount(int columns)` | Sets the number of columns. The minimum is 1. It resets the model, so all items collapse. |
| `QTuiTreeWidgetItem *currentItem() const` | Returns the current item, or `nullptr`. |
| `void setCurrentItem(QTuiTreeWidgetItem *item)` | Makes the item current. The parents of the item must be expanded. It does not emit `currentItemChanged()`. |
| `QList<QTuiTreeWidgetItem *> selectedItems() const` | Returns the selected items. |
| `void keyPressEvent(TuiKeyEvent *event) override` | Handles the keys of `QTuiTreeView` and emits the signals below. |

### Signals

| Signal | Description |
| --- | --- |
| `currentItemChanged(QTuiTreeWidgetItem *current, QTuiTreeWidgetItem *previous)` | A key made a different item current. A mouse click does not emit it. |
| `itemClicked(QTuiTreeWidgetItem *item, int column)` | Enter on the current item. The column is always 0. |

### Keys

| Key | Action |
| --- | --- |
| Up, Down, Left, Right | Move the current item, expand or collapse, as in `QTuiTreeView`. They emit `currentItemChanged()` if the current item changes. |
| Enter | Expands or collapses the item, and emits `activated()` and `itemClicked()`. |

The keys of [`QTuiTreeView`](model-view.md#qtuitreeview) and of
`QTuiAbstractItemView` also work.

## QTuiTreeWidgetItem

`#include <QtTUI/QTuiTreeWidgetItem.h>` · Inherits: none

One item of a `QTuiTreeWidget`, with a text for each column and child
items. The item owns its children. The default flags are
`Qt::ItemIsSelectable | Qt::ItemIsEnabled`, so an item is not editable.
Different from Qt: the check state is an `int`, and the tree does not show
it.

The destructor deletes the children, but it does not remove the item from
its parent or its tree. Call `takeChild()` or
`QTuiTreeWidget::takeTopLevelItem()` before you delete an item.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void example(QTuiTreeWidget *tree)
{
    auto *folder = new QTuiTreeWidgetItem({QStringLiteral("src"), QStringLiteral("dir")});
    auto *file = new QTuiTreeWidgetItem({QStringLiteral("main.cpp")});
    file->setData(0, Qt::UserRole, QStringLiteral("/src/main.cpp"));
    file->setCheckState(0, Qt::Checked);
    folder->addChild(file);
    tree->addTopLevelItem(folder);

    // Later: remove and delete the child
    delete folder->takeChild(0);
}
```

### Functions

| Function | Description |
| --- | --- |
| `QTuiTreeWidgetItem(const QStringList &texts = QStringList())` | Makes an item with one text for each column. |
| `~QTuiTreeWidgetItem()` | Deletes the children. |
| `void addChild(QTuiTreeWidgetItem *child)` | Adds the child at the end. The item takes ownership. If the child has a parent, the child moves from that parent. |
| `QTuiTreeWidgetItem *child(int index) const` | Returns the child, or `nullptr`. |
| `int childCount() const` | Returns the number of children. |
| `QTuiTreeWidgetItem *takeChild(int index)` | Removes the child and returns it. You own it. Returns `nullptr` if the index is out of range. |
| `QTuiTreeWidgetItem *parent() const` | Returns the parent item, or `nullptr` for a top-level item. |
| `QTuiTreeWidget *treeWidget() const` | Returns the tree that has the item, or `nullptr`. |
| `QString text(int column) const` | Returns the `Qt::DisplayRole` text of the column. |
| `void setText(int column, const QString &text)` | Sets the text of the column and repaints the item. |
| `int checkState(int column) const` | Returns the `Qt::CheckStateRole` data of the column as an `int`. |
| `void setCheckState(int column, int state)` | Sets the `Qt::CheckStateRole` data of the column. |
| `QVariant data(int column, int role) const` | Returns the data of the column for the role. |
| `void setData(int column, int role, const QVariant &value)` | Sets the data of the column for the role and repaints the item. |
| `int columnCount() const` | Returns the number of columns that have data. |
| `Qt::ItemFlags flags() const` | Returns the flags. |
| `void setFlags(Qt::ItemFlags flags)` | Sets the flags. |

## QTuiTreeWidgetItemIterator

`#include <QtTUI/QTuiTreeWidgetItemIterator.h>` · Inherits: none

Goes through the items of a tree, depth first: an item, then its children,
then the next item. Different from Qt: an iterator that starts at an item
goes through that item and its descendants only, not the items after it.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

QStringList checkedNames(QTuiTreeWidget *tree)
{
    QStringList names;
    for (QTuiTreeWidgetItemIterator it(tree, QTuiTreeWidgetItemIterator::Checked);
         !it.atEnd(); ++it)
        names.append((*it)->text(0));
    return names;
}
```

### Enums

| Enum | Values |
| --- | --- |
| `IteratorFlag` (flags `IteratorFlags`) | `All`: all items (the default). `Checked`: the check state of column 0 is 2. `Unchecked`: the check state of column 0 is 0. `Selected`: the item has `Qt::ItemIsSelectable` (not the selection). `Hidden`: matches no item. An item that matches one of the flags is included. |

### Functions

| Function | Description |
| --- | --- |
| `QTuiTreeWidgetItemIterator(QTuiTreeWidget *widget, IteratorFlags flags = All)` | Starts at the first top-level item that matches the flags. |
| `QTuiTreeWidgetItemIterator(QTuiTreeWidgetItem *item, IteratorFlags flags = All)` | Starts at the item, or at its first descendant that matches the flags. |
| `QTuiTreeWidgetItemIterator &operator++()` | Moves to the next item that matches. |
| `QTuiTreeWidgetItemIterator operator++(int)` | Moves to the next item that matches and returns the old position. |
| `QTuiTreeWidgetItem *operator*() const` | Returns the current item, or `nullptr` at the end. |
| `bool atEnd() const` | Returns `true` if there are no more items. |
