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

    return item->data(index.column(), role);
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

// ---- QTuiTreeWidget ----

QTuiTreeWidget::QTuiTreeWidget(QTuiWidget *parent)
    : QTuiTreeView(parent)
    , m_internalModel(new QTuiTreeWidgetModel(this))
{
    m_internalModel->m_topLevelItemsRef = &m_topLevelItems;
    m_internalModel->m_columnCount = m_columnCount;
    setModel(m_internalModel);
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
        item->m_parent->m_children.removeOne(item);
        item->m_parent = nullptr;
    }

    item->setTreeWidgetRecursive(this);
    m_topLevelItems.append(item);
    syncFromItems();
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

    QTuiTreeWidgetItem *taken = m_topLevelItems.takeAt(index);
    taken->m_parent = nullptr;
    taken->setTreeWidgetRecursive(nullptr);
    syncFromItems();
    return taken;
}

void QTuiTreeWidget::setHeaderLabels(const QStringList &labels)
{
    m_columnCount = qMax(m_columnCount, labels.size());
    m_internalModel->m_columnCount = m_columnCount;
    for (int i = 0; i < labels.size(); ++i)
        m_internalModel->m_headerData[i] = labels.at(i);
    syncFromItems();
}

int QTuiTreeWidget::columnCount() const
{
    return m_columnCount;
}

void QTuiTreeWidget::setColumnCount(int columns)
{
    if (columns < 1)
        columns = 1;
    m_columnCount = columns;
    m_internalModel->m_columnCount = columns;
    syncFromItems();
}

QTuiTreeWidgetItem *QTuiTreeWidget::currentItem() const
{
    QModelIndex idx = currentIndex();
    if (!idx.isValid())
        return nullptr;
    return itemFromIndex(idx);
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

void QTuiTreeWidget::syncFromItems()
{
    m_internalModel->rebuild();
}

QTuiTreeWidgetItem *QTuiTreeWidget::itemFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    return static_cast<QTuiTreeWidgetItem *>(index.internalPointer());
}

} // namespace QtTUI
