#include <QtTUI/QTuiTableWidget.h>
#include <QtTUI/TuiEvents.h>
#include <algorithm>
#include <numeric>

namespace QtTUI {

// ---- QTuiTableWidgetModel ----

QTuiTableWidgetModel::QTuiTableWidgetModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int QTuiTableWidgetModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows;
}

int QTuiTableWidgetModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_columns;
}

QVariant QTuiTableWidgetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_itemsRef)
        return {};
    int r = index.row();
    int c = index.column();
    if (r < 0 || r >= m_rows || c < 0 || c >= m_columns)
        return {};
    QTuiTableWidgetItem *item = (*m_itemsRef)[r][c];
    if (!item)
        return {};
    // As in Qt, the edit text is the display text
    return item->data(role == Qt::EditRole ? Qt::DisplayRole : role);
}

Qt::ItemFlags QTuiTableWidgetModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !m_itemsRef)
        return Qt::NoItemFlags;
    if (QTuiTableWidgetItem *item = (*m_itemsRef)[index.row()][index.column()])
        return item->flags();
    // An empty cell is editable, as in Qt
    return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool QTuiTableWidgetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !m_itemsRef)
        return false;
    if (role == Qt::EditRole)
        role = Qt::DisplayRole;
    QTuiTableWidgetItem *item = (*m_itemsRef)[index.row()][index.column()];
    if (!item) {
        // A value for an empty cell makes a new item, as in Qt
        auto *table = qobject_cast<QTuiTableWidget *>(QObject::parent());
        if (!table || !value.isValid())
            return false;
        item = new QTuiTableWidgetItem;
        table->setItem(index.row(), index.column(), item);
    }
    item->setData(role, value);
    return true;
}

QVariant QTuiTableWidgetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation == Qt::Horizontal)
        return m_horizontalHeaders.value(section);
    return m_verticalHeaders.value(section);
}

bool QTuiTableWidgetModel::setHeaderData(int section, Qt::Orientation orientation,
                                          const QVariant &value, int role)
{
    if (role != Qt::EditRole && role != Qt::DisplayRole)
        return false;
    if (orientation == Qt::Horizontal)
        m_horizontalHeaders[section] = value;
    else
        m_verticalHeaders[section] = value;
    emit headerDataChanged(orientation, section, section);
    return true;
}

void QTuiTableWidgetModel::setDimensions(int rows, int columns)
{
    beginResetModel();
    m_rows = rows;
    m_columns = columns;
    endResetModel();
}

void QTuiTableWidgetModel::sort(int column, Qt::SortOrder order)
{
    if (!m_itemsRef || column < 0 || column >= m_columns)
        return;
    auto &rows = *m_itemsRef;
    // Sort the row numbers by the text of the column. Empty cells go last.
    QList<int> from(m_rows);
    std::iota(from.begin(), from.end(), 0);
    auto text = [&](int row) {
        QTuiTableWidgetItem *item = rows[row][column];
        return item ? item->text() : QString();
    };
    std::stable_sort(from.begin(), from.end(), [&](int a, int b) {
        const QString ta = text(a), tb = text(b);
        if (ta.isEmpty() != tb.isEmpty())
            return tb.isEmpty();
        return order == Qt::AscendingOrder ? ta < tb : tb < ta;
    });

    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    QList<QList<QTuiTableWidgetItem *>> sorted;
    for (int row : from)
        sorted.append(rows[row]);
    rows = sorted;
    // The items and the persistent indexes (the selection) go to the new rows
    QList<int> to(m_rows);
    for (int row = 0; row < m_rows; ++row) {
        to[from[row]] = row;
        for (QTuiTableWidgetItem *item : rows[row])
            if (item)
                item->m_row = row;
    }
    const QModelIndexList old = persistentIndexList();
    QModelIndexList moved;
    for (const QModelIndex &index : old)
        moved.append(this->index(to[index.row()], index.column()));
    changePersistentIndexList(old, moved);
    emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);
}

void QTuiTableWidgetModel::updateCell(int row, int column)
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
        return;
    QModelIndex idx = index(row, column);
    emit dataChanged(idx, idx);
}

// ---- QTuiTableWidget ----

QTuiTableWidget::QTuiTableWidget(QTuiWidget *parent)
    : QTuiTableView(parent)
    , m_internalModel(new QTuiTableWidgetModel(this))
{
    m_internalModel->m_itemsRef = &m_items;
    setModel(m_internalModel);
    connect(this, &QTuiAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        emit cellClicked(index.row(), index.column());
    });
}

QTuiTableWidget::QTuiTableWidget(int rows, int columns, QTuiWidget *parent)
    : QTuiTableWidget(parent)
{
    resizeItemGrid(rows, columns);
    m_internalModel->setDimensions(rows, columns);
}

QTuiTableWidget::~QTuiTableWidget()
{
    for (auto &row : m_items) {
        for (auto *item : row)
            delete item;
    }
    m_items.clear();
}

void QTuiTableWidget::setRowCount(int rows)
{
    if (rows == m_internalModel->m_rows)
        return;
    int cols = m_internalModel->m_columns;
    // The model contract requires beginResetModel() BEFORE any mutation the
    // model can observe (the item grid is read through m_itemsRef).
    m_internalModel->beginResetModel();
    resizeItemGrid(rows, cols);
    m_internalModel->m_rows = rows;
    m_internalModel->m_columns = cols;
    m_internalModel->endResetModel();
}

int QTuiTableWidget::rowCount() const
{
    return m_internalModel->m_rows;
}

void QTuiTableWidget::setColumnCount(int columns)
{
    if (columns == m_internalModel->m_columns)
        return;
    int rows = m_internalModel->m_rows;
    // The model contract requires beginResetModel() BEFORE any mutation the
    // model can observe (the item grid is read through m_itemsRef).
    m_internalModel->beginResetModel();
    resizeItemGrid(rows, columns);
    m_internalModel->m_rows = rows;
    m_internalModel->m_columns = columns;
    m_internalModel->endResetModel();
}

int QTuiTableWidget::columnCount() const
{
    return m_internalModel->m_columns;
}

void QTuiTableWidget::setItem(int row, int column, QTuiTableWidgetItem *item)
{
    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
        return;

    QTuiTableWidgetItem *old = m_items[row][column];
    if (old) {
        old->m_tableWidget = nullptr;
        old->m_row = -1;
        old->m_column = -1;
        delete old;
    }

    m_items[row][column] = item;
    if (item) {
        item->m_tableWidget = this;
        item->m_row = row;
        item->m_column = column;
    }
    m_internalModel->updateCell(row, column);
}

QTuiTableWidgetItem *QTuiTableWidget::item(int row, int column) const
{
    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
        return nullptr;
    return m_items[row][column];
}

QTuiTableWidgetItem *QTuiTableWidget::takeItem(int row, int column)
{
    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
        return nullptr;

    QTuiTableWidgetItem *taken = m_items[row][column];
    if (taken) {
        taken->m_tableWidget = nullptr;
        taken->m_row = -1;
        taken->m_column = -1;
        m_items[row][column] = nullptr;
        m_internalModel->updateCell(row, column);
    }
    return taken;
}

void QTuiTableWidget::setHorizontalHeaderLabels(const QStringList &labels)
{
    for (int i = 0; i < labels.size() && i < columnCount(); ++i)
        m_internalModel->setHeaderData(i, Qt::Horizontal, labels.at(i));
}

void QTuiTableWidget::setVerticalHeaderLabels(const QStringList &labels)
{
    for (int i = 0; i < labels.size() && i < rowCount(); ++i)
        m_internalModel->setHeaderData(i, Qt::Vertical, labels.at(i));
}

QList<QTuiTableWidgetSelectionRange> QTuiTableWidget::selectedRanges() const
{
    QList<QTuiTableWidgetSelectionRange> ranges;
    if (!selectionModel())
        return ranges;
    for (const QItemSelectionRange &r : selectionModel()->selection()) {
        QTuiTableWidgetSelectionRange range;
        range.m_topRow = r.top();
        range.m_bottomRow = r.bottom();
        range.m_leftColumn = r.left();
        range.m_rightColumn = r.right();
        ranges.append(range);
    }
    return ranges;
}

QList<QTuiTableWidgetItem *> QTuiTableWidget::selectedItems() const
{
    QList<QTuiTableWidgetItem *> items;
    if (!selectionModel())
        return items;
    for (const QModelIndex &index : selectionModel()->selectedIndexes()) {
        if (QTuiTableWidgetItem *it = item(index.row(), index.column()))
            items.append(it);
    }
    return items;
}

void QTuiTableWidget::keyPressEvent(TuiKeyEvent *event)
{
    int prevRow = currentIndex().isValid() ? currentIndex().row() : -1;
    int prevCol = currentIndex().isValid() ? currentIndex().column() : -1;

    QTuiTableView::keyPressEvent(event);

    int newRow = currentIndex().isValid() ? currentIndex().row() : -1;
    int newCol = currentIndex().isValid() ? currentIndex().column() : -1;

    if (newRow != prevRow || newCol != prevCol)
        emit currentCellChanged(newRow, newCol, prevRow, prevCol);

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (newRow >= 0 && newCol >= 0)
            emit cellClicked(newRow, newCol);
    }
}

void QTuiTableWidget::mousePressEvent(TuiMouseEvent *event)
{
    const QModelIndex prev = currentIndex();
    QTuiTableView::mousePressEvent(event);
    const QModelIndex current = currentIndex();
    if (current != prev)
        emit currentCellChanged(current.row(), current.column(), prev.row(), prev.column());
}

void QTuiTableWidget::syncItemToModel(int row, int column)
{
    m_internalModel->updateCell(row, column);
}

void QTuiTableWidget::resizeItemGrid(int rows, int columns)
{
    int oldRows = m_items.size();
    int oldCols = oldRows > 0 ? m_items[0].size() : 0;

    // Delete items from rows being removed
    for (int r = rows; r < oldRows; ++r) {
        for (auto *item : m_items[r]) {
            if (item) {
                item->m_tableWidget = nullptr;
                item->m_row = -1;
                item->m_column = -1;
                delete item;
            }
        }
    }

    // Delete items from columns being removed in remaining rows
    for (int r = 0; r < qMin(oldRows, rows); ++r) {
        for (int c = columns; c < oldCols; ++c) {
            if (m_items[r][c]) {
                m_items[r][c]->m_tableWidget = nullptr;
                m_items[r][c]->m_row = -1;
                m_items[r][c]->m_column = -1;
                delete m_items[r][c];
            }
        }
    }

    m_items.resize(rows);
    for (int r = 0; r < rows; ++r)
        m_items[r].resize(columns, nullptr);
}

} // namespace QtTUI
