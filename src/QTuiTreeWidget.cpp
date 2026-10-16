#include <QtTUI/QTuiTreeWidget.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

// ---- QTuiTreeWidgetModel ----

QTuiTreeWidgetModel::QTuiTreeWidgetModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

static QTuiTreeWidgetItem *itemForIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;
    return static_cast<QTuiTreeWidgetItem *>(index.internalPointer());
}

QModelIndex QTuiTreeWidgetModel::index(int row, int column,
                                        const QModelIndex &parent) const
{
    if (!m_topLevelItemsRef || column < 0 || column >= m_columnCount || row < 0)
        return {};

    if (!parent.isValid()) {
        // Top-level
        if (row >= m_topLevelItemsRef->size())
            return {};
        return createIndex(row, column, m_topLevelItemsRef->at(row));
    }

    QTuiTreeWidgetItem *parentItem = itemForIndex(parent);
    if (!parentItem || row >= parentItem->childCount())
        return {};
    return createIndex(row, column, parentItem->child(row));
}

QModelIndex QTuiTreeWidgetModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || !m_topLevelItemsRef)
        return {};

    QTuiTreeWidgetItem *item = itemForIndex(child);
    if (!item)
        return {};

    QTuiTreeWidgetItem *parentItem = item->parent();
    if (!parentItem)
        return {}; // top-level item, no parent

    // Find the row of parentItem in its own parent
    QTuiTreeWidgetItem *grandparent = parentItem->parent();
    if (!grandparent) {
        // parentItem is a top-level item
        int row = m_topLevelItemsRef->indexOf(parentItem);
        if (row < 0)
            return {};
        return createIndex(row, 0, parentItem);
    }

    // parentItem is a child of grandparent
    for (int i = 0; i < grandparent->childCount(); ++i) {
        if (grandparent->child(i) == parentItem)
            return createIndex(i, 0, parentItem);
    }
    return {};
}

int QTuiTreeWidgetModel::rowCount(const QModelIndex &parent) const
{
    if (!m_topLevelItemsRef)
        return 0;

    if (!parent.isValid())
        return m_topLevelItemsRef->size();

    QTuiTreeWidgetItem *item = itemForIndex(parent);
    if (!item || parent.column() != 0)
        return 0;
    return item->childCount();
}

int QTuiTreeWidgetModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_columnCount;
}

QVariant QTuiTreeWidgetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    QTuiTreeWidgetItem *item = itemForIndex(index);
    if (!item)
        return {};

    // As in Qt, the edit text is the display text
    return item->data(index.column(), role == Qt::EditRole ? Qt::DisplayRole : role);
}

Qt::ItemFlags QTuiTreeWidgetModel::flags(const QModelIndex &index) const
{
    QTuiTreeWidgetItem *item = index.isValid() ? itemForIndex(index) : nullptr;
    return item ? item->flags() : Qt::NoItemFlags;
}

bool QTuiTreeWidgetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QTuiTreeWidgetItem *item = index.isValid() ? itemForIndex(index) : nullptr;
    if (!item)
        return false;
    item->setData(index.column(), role == Qt::EditRole ? Qt::DisplayRole : role, value);
    return true;
}

bool QTuiTreeWidgetModel::hasChildren(const QModelIndex &parent) const
{
    if (!m_topLevelItemsRef)
        return false;

    if (!parent.isValid())
        return !m_topLevelItemsRef->isEmpty();

    QTuiTreeWidgetItem *item = itemForIndex(parent);
    if (!item || parent.column() != 0)
        return false;
    return item->childCount() > 0;
}

QVariant QTuiTreeWidgetModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    return m_headerData.value(section);
}

void QTuiTreeWidgetModel::rebuild()
{
    beginResetModel();
    endResetModel();
}

QModelIndex QTuiTreeWidgetModel::indexForItem(QTuiTreeWidgetItem *item) const
{
    if (!item || !m_topLevelItemsRef)
        return {};

    QTuiTreeWidgetItem *parentItem = item->parent();
    int row = -1;
    if (!parentItem) {
        row = m_topLevelItemsRef->indexOf(item);
    } else {
        for (int i = 0; i < parentItem->childCount(); ++i) {
            if (parentItem->child(i) == item) {
                row = i;
                break;
            }
        }
    }
    if (row < 0)
        return {};
    return createIndex(row, 0, item);
}

void QTuiTreeWidgetModel::itemDataChanged(QTuiTreeWidgetItem *item, int column)
{
    QModelIndex idx = indexForItem(item);
    if (!idx.isValid())
        return;
    const int col = qBound(0, column, qMax(0, m_columnCount - 1));
    QModelIndex cell = (col == 0) ? idx : createIndex(idx.row(), col, item);
    emit dataChanged(cell, cell);
}

void QTuiTreeWidgetModel::beginInsertItem(QTuiTreeWidgetItem *parent, int row)
{
    beginInsertRows(parent ? indexForItem(parent) : QModelIndex(), row, row);
}

void QTuiTreeWidgetModel::endInsertItem()
{
    endInsertRows();
}

void QTuiTreeWidgetModel::beginRemoveItem(QTuiTreeWidgetItem *parent, int row)
{
    beginRemoveRows(parent ? indexForItem(parent) : QModelIndex(), row, row);
}

void QTuiTreeWidgetModel::endRemoveItem()
{
    endRemoveRows();
}

// ---- QTuiTreeWidget ----

QTuiTreeWidget::QTuiTreeWidget(QTuiWidget *parent)
    : QTuiTreeView(parent)
    , m_internalModel(new QTuiTreeWidgetModel(this))
{
    m_internalModel->m_topLevelItemsRef = &m_topLevelItems;
    m_internalModel->m_columnCount = m_columnCount;
    setModel(m_internalModel);
    connect(this, &QTuiAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        emit itemClicked(itemFromIndex(index), index.column());
    });
}

QTuiTreeWidget::~QTuiTreeWidget()
{
    qDeleteAll(m_topLevelItems);
    m_topLevelItems.clear();
}

void QTuiTreeWidget::addTopLevelItem(QTuiTreeWidgetItem *item)
{
    if (!item || m_topLevelItems.contains(item))
        return;

    if (item->m_parent) {
        // Detach from the old parent with proper removal signals when the
        // old parent belongs to a tree widget.
        QTuiTreeWidget *oldTree = item->m_treeWidget;
        int oldRow = item->m_parent->m_children.indexOf(item);
        if (oldTree && oldRow >= 0) {
            oldTree->beginRemoveItem(item->m_parent, oldRow);
            item->m_parent->m_children.removeAt(oldRow);
            oldTree->endRemoveItem();
        } else {
            item->m_parent->m_children.removeOne(item);
        }
        item->m_parent = nullptr;
    }

    item->setTreeWidgetRecursive(this);
    // The model contract requires begin* BEFORE the container mutation
    beginInsertItem(nullptr, m_topLevelItems.size());
    m_topLevelItems.append(item);
    endInsertItem();
}

QTuiTreeWidgetItem *QTuiTreeWidget::topLevelItem(int index) const
{
    if (index < 0 || index >= m_topLevelItems.size())
        return nullptr;
    return m_topLevelItems.at(index);
}

int QTuiTreeWidget::topLevelItemCount() const
{
    return m_topLevelItems.size();
}

QTuiTreeWidgetItem *QTuiTreeWidget::takeTopLevelItem(int index)
{
    if (index < 0 || index >= m_topLevelItems.size())
        return nullptr;

    beginRemoveItem(nullptr, index);
    QTuiTreeWidgetItem *taken = m_topLevelItems.takeAt(index);
    endRemoveItem();
    taken->m_parent = nullptr;
    taken->setTreeWidgetRecursive(nullptr);
    return taken;
}

void QTuiTreeWidget::setHeaderLabels(const QStringList &labels)
{
    // Changing the column count restructures every index: full reset, with
    // begin* called before any mutation as the model contract requires.
    m_internalModel->beginResetModel();
    m_columnCount = qMax(m_columnCount, labels.size());
    m_internalModel->m_columnCount = m_columnCount;
    for (int i = 0; i < labels.size(); ++i)
        m_internalModel->m_headerData[i] = labels.at(i);
    m_internalModel->endResetModel();
}

int QTuiTreeWidget::columnCount() const
{
    return m_columnCount;
}

void QTuiTreeWidget::setColumnCount(int columns)
{
    if (columns < 1)
        columns = 1;
    if (columns == m_columnCount)
        return;
    // Changing the column count restructures every index: full reset, with
    // begin* called before any mutation as the model contract requires.
    m_internalModel->beginResetModel();
    m_columnCount = columns;
    m_internalModel->m_columnCount = columns;
    m_internalModel->endResetModel();
}

QTuiTreeWidgetItem *QTuiTreeWidget::currentItem() const
{
    QModelIndex idx = currentIndex();
    if (!idx.isValid())
        return nullptr;
    return itemFromIndex(idx);
}

QList<QTuiTreeWidgetItem *> QTuiTreeWidget::selectedItems() const
{
    QList<QTuiTreeWidgetItem *> items;
    if (!selectionModel())
        return items;
    for (const QModelIndex &index : selectionModel()->selectedIndexes()) {
        if (index.column() == 0)
            items.append(itemFromIndex(index));
    }
    return items;
}

void QTuiTreeWidget::setCurrentItem(QTuiTreeWidgetItem *item)
{
    if (!item)
        return;

    // Find the model index for this item by walking the hierarchy
    auto findIndex = [this](QTuiTreeWidgetItem *target,
                            const QList<QTuiTreeWidgetItem *> &siblings,
                            const QModelIndex &parentIdx,
                            auto &self) -> QModelIndex {
        for (int i = 0; i < siblings.size(); ++i) {
            QModelIndex idx = m_internalModel->index(i, 0, parentIdx);
            if (siblings.at(i) == target)
                return idx;
            QModelIndex found = self(target, siblings.at(i)->m_children, idx, self);
            if (found.isValid())
                return found;
        }
        return QModelIndex();
    };

    QModelIndex idx = findIndex(item, m_topLevelItems, QModelIndex(), findIndex);
    if (idx.isValid())
        setCurrentIndex(idx);
}

void QTuiTreeWidget::mousePressEvent(TuiMouseEvent *event)
{
    QTuiTreeWidgetItem *prevItem = currentItem();
    QTuiTreeView::mousePressEvent(event);
    if (currentItem() != prevItem)
        emit currentItemChanged(currentItem(), prevItem);
}

void QTuiTreeWidget::keyPressEvent(TuiKeyEvent *event)
{
    QTuiTreeWidgetItem *prevItem = currentItem();

    QTuiTreeView::keyPressEvent(event);

    QTuiTreeWidgetItem *newItem = currentItem();

    if (newItem != prevItem)
        emit currentItemChanged(newItem, prevItem);

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (newItem)
            emit itemClicked(newItem, 0);
    }
}

void QTuiTreeWidget::notifyItemDataChanged(QTuiTreeWidgetItem *item, int column)
{
    m_internalModel->itemDataChanged(item, column);
}

void QTuiTreeWidget::beginInsertItem(QTuiTreeWidgetItem *parent, int row)
{
    m_internalModel->beginInsertItem(parent, row);
}

void QTuiTreeWidget::endInsertItem()
{
    m_internalModel->endInsertItem();
}

void QTuiTreeWidget::beginRemoveItem(QTuiTreeWidgetItem *parent, int row)
{
    m_internalModel->beginRemoveItem(parent, row);
}

void QTuiTreeWidget::endRemoveItem()
{
    m_internalModel->endRemoveItem();
}

QTuiTreeWidgetItem *QTuiTreeWidget::itemFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    return static_cast<QTuiTreeWidgetItem *>(index.internalPointer());
}

} // namespace QtTUI
