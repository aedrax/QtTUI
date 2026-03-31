#include <QtTUI/QTuiTableWidget.h>
#include <QtTUI/TuiEvents.h>

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
    return item->data(role);
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
}

QTuiTableWidget::QTuiTableWidget(int rows, int columns, QTuiWidget *parent)
    : QTuiTableView(parent)
    , m_internalModel(new QTuiTableWidgetModel(this))
{
    m_internalModel->m_itemsRef = &m_items;
    resizeItemGrid(rows, columns);
    m_internalModel->setDimensions(rows, columns);
    setModel(m_internalModel);
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
    resizeItemGrid(rows, cols);
    m_internalModel->setDimensions(rows, cols);
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
    resizeItemGrid(rows, columns);
    m_internalModel->setDimensions(rows, columns);
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
    QModelIndex idx = currentIndex();
    if (!idx.isValid())
        return {};

    QTuiTableWidgetSelectionRange range;
    range.m_topRow = idx.row();
    range.m_bottomRow = idx.row();
    range.m_leftColumn = idx.column();
    range.m_rightColumn = idx.column();
    return { range };
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
