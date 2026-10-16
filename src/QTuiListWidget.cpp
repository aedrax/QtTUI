#include <QtTUI/QTuiListWidget.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

// ---- QTuiListWidgetModel ----

QTuiListWidgetModel::QTuiListWidgetModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int QTuiListWidgetModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_itemsRef)
        return 0;
    return m_itemsRef->size();
}

QVariant QTuiListWidgetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_itemsRef)
        return {};
    int row = index.row();
    if (row < 0 || row >= m_itemsRef->size())
        return {};
    // As in Qt, the edit text is the display text
    return m_itemsRef->at(row)->data(role == Qt::EditRole ? Qt::DisplayRole : role);
}

Qt::ItemFlags QTuiListWidgetModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !m_itemsRef || index.row() >= m_itemsRef->size())
        return Qt::NoItemFlags;
    return m_itemsRef->at(index.row())->flags();
}

bool QTuiListWidgetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !m_itemsRef || index.row() >= m_itemsRef->size())
        return false;
    m_itemsRef->at(index.row())->setData(role == Qt::EditRole ? Qt::DisplayRole : role, value);
    return true;
}

void QTuiListWidgetModel::appendItem(QTuiListWidgetItem *item)
{
    if (!m_itemsRef)
        return;
    int row = m_itemsRef->size();
    beginInsertRows(QModelIndex(), row, row);
    m_itemsRef->append(item);
    endInsertRows();
}

void QTuiListWidgetModel::insertItemAt(int row, QTuiListWidgetItem *item)
{
    if (!m_itemsRef)
        return;
    row = qBound(0, row, m_itemsRef->size());
    beginInsertRows(QModelIndex(), row, row);
    m_itemsRef->insert(row, item);
    endInsertRows();
}

void QTuiListWidgetModel::removeItemAt(int row)
{
    if (!m_itemsRef || row < 0 || row >= m_itemsRef->size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_itemsRef->removeAt(row);
    endRemoveRows();
}

void QTuiListWidgetModel::updateItem(int row)
{
    if (!m_itemsRef || row < 0 || row >= m_itemsRef->size())
        return;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx);
}

void QTuiListWidgetModel::resetItems()
{
    beginResetModel();
    endResetModel();
}

// ---- QTuiListWidget ----

QTuiListWidget::QTuiListWidget(QTuiWidget *parent)
    : QTuiListView(parent)
    , m_internalModel(new QTuiListWidgetModel(this))
{
    m_internalModel->m_itemsRef = &m_items;
    setModel(m_internalModel);
    connect(this, &QTuiAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        emit itemClicked(item(index.row()));
    });
}

QTuiListWidget::~QTuiListWidget()
{
    qDeleteAll(m_items);
    m_items.clear();
}

void QTuiListWidget::addItem(const QString &label)
{
    auto *lwItem = new QTuiListWidgetItem();
    lwItem->m_data[Qt::DisplayRole] = label;
    addItem(lwItem);
}

void QTuiListWidget::addItem(QTuiListWidgetItem *item)
{
    if (!item || m_items.contains(item))
        return;

    item->m_listWidget = this;
    m_internalModel->appendItem(item);
}

QTuiListWidgetItem *QTuiListWidget::takeItem(int row)
{
    if (row < 0 || row >= m_items.size())
        return nullptr;

    QTuiListWidgetItem *lwItem = m_items.at(row);
    lwItem->m_listWidget = nullptr;
    m_internalModel->removeItemAt(row);
    return lwItem;
}

void QTuiListWidget::insertItem(int row, QTuiListWidgetItem *item)
{
    if (!item || m_items.contains(item))
        return;

    item->m_listWidget = this;
    m_internalModel->insertItemAt(row, item);
}

int QTuiListWidget::count() const
{
    return m_items.size();
}

QTuiListWidgetItem *QTuiListWidget::item(int row) const
{
    if (row < 0 || row >= m_items.size())
        return nullptr;
    return m_items.at(row);
}

QTuiListWidgetItem *QTuiListWidget::currentItem() const
{
    int row = currentRow();
    if (row < 0 || row >= m_items.size())
        return nullptr;
    return m_items.at(row);
}

QList<QTuiListWidgetItem *> QTuiListWidget::selectedItems() const
{
    QList<QTuiListWidgetItem *> items;
    if (!selectionModel())
        return items;
    for (const QModelIndex &index : selectionModel()->selectedIndexes()) {
        if (index.column() == 0 && index.row() < m_items.size())
            items.append(m_items.at(index.row()));
    }
    return items;
}

void QTuiListWidget::setCurrentItem(QTuiListWidgetItem *item)
{
    int row = rowForItem(item);
    if (row >= 0)
        setCurrentRow(row);
}

int QTuiListWidget::currentRow() const
{
    QModelIndex idx = currentIndex();
    return idx.isValid() ? idx.row() : -1;
}

void QTuiListWidget::setCurrentRow(int row)
{
    if (row < 0 || row >= m_items.size())
        return;

    int prevRow = currentRow();
    setCurrentIndex(m_internalModel->index(row, 0));

    if (prevRow != row) {
        QTuiListWidgetItem *prev = (prevRow >= 0 && prevRow < m_items.size()) ? m_items.at(prevRow) : nullptr;
        QTuiListWidgetItem *curr = m_items.at(row);
        emit currentItemChanged(curr, prev);
    }
}

void QTuiListWidget::keyPressEvent(TuiKeyEvent *event)
{
    int prevRow = currentRow();

    // Let base class handle navigation
    QTuiListView::keyPressEvent(event);

    int newRow = currentRow();

    // Emit currentItemChanged if row changed
    if (newRow != prevRow && newRow >= 0 && newRow < m_items.size()) {
        QTuiListWidgetItem *prev = (prevRow >= 0 && prevRow < m_items.size()) ? m_items.at(prevRow) : nullptr;
        QTuiListWidgetItem *curr = m_items.at(newRow);
        emit currentItemChanged(curr, prev);
    }

    // Emit itemClicked on Enter/Return
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (newRow >= 0 && newRow < m_items.size())
            emit itemClicked(m_items.at(newRow));
    }
}

void QTuiListWidget::mousePressEvent(TuiMouseEvent *event)
{
    const int prevRow = currentRow();
    QTuiListView::mousePressEvent(event);
    const int newRow = currentRow();
    if (newRow != prevRow && newRow >= 0 && newRow < m_items.size())
        emit currentItemChanged(m_items.at(newRow), item(prevRow));
}

void QTuiListWidget::syncItemToModel(QTuiListWidgetItem *item)
{
    int row = rowForItem(item);
    if (row < 0)
        return;
    m_internalModel->updateItem(row);
}

int QTuiListWidget::rowForItem(QTuiListWidgetItem *item) const
{
    if (!item)
        return -1;
    return m_items.indexOf(item);
}

} // namespace QtTUI
