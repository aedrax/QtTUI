#include <QtTUI/QTuiTableView.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QDebug>

namespace QtTUI {

QTuiTableView::QTuiTableView(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

void QTuiTableView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_currentRow = 0;
    m_currentCol = 0;
    m_scrollOffset = 0;

    if (m_model)
        connectModel();
}

QAbstractItemModel *QTuiTableView::model() const
{
    return m_model;
}

QModelIndex QTuiTableView::currentIndex() const
{
    if (!m_model || m_model->rowCount() == 0 || m_model->columnCount() == 0)
        return QModelIndex();
    return m_model->index(m_currentRow, m_currentCol);
}

void QTuiTableView::setCurrentIndex(const QModelIndex &index)
{
    if (!m_model)
        return;
    m_currentRow = qBound(0, index.row(), qMax(0, m_model->rowCount() - 1));
    m_currentCol = qBound(0, index.column(), qMax(0, m_model->columnCount() - 1));
    ensureCurrentVisible();
}

int QTuiTableView::scrollOffset() const
{
    return m_scrollOffset;
}

void QTuiTableView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0 || !m_model)
        return;

    const auto theme = themeProperties();
    const int cols = m_model->columnCount();
    const int rows = m_model->rowCount();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    // Calculate column widths: distribute evenly
    QVector<int> colWidths(cols, 0);
    if (cols > 0) {
        int baseWidth = viewWidth / cols;
        int remainder = viewWidth % cols;
        for (int c = 0; c < cols; ++c)
            colWidths[c] = baseWidth + (c < remainder ? 1 : 0);
    }

    // Render column headers (row 0 of the view)
    {
        int xOff = geo.x();
        for (int c = 0; c < cols; ++c) {
            QString header = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
            for (int i = 0; i < colWidths[c] && xOff + i < geo.x() + viewWidth; ++i) {
                Cell &cell = buffer.backCell(xOff + i, geo.y());
                cell.character = (i < header.length()) ? header.at(i) : QLatin1Char(' ');
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.bold = true;
                cell.inverse = false;
            }
            xOff += colWidths[c];
        }
    }

    // Render visible data rows
    int visibleDataRows = viewHeight - m_headerHeight;
    for (int vr = 0; vr < visibleDataRows; ++vr) {
        int modelRow = m_scrollOffset + vr;
        int screenY = geo.y() + m_headerHeight + vr;

        int xOff = geo.x();
        for (int c = 0; c < cols; ++c) {
            QString cellText;
            if (modelRow < rows) {
                QModelIndex idx = m_model->index(modelRow, c);
                cellText = m_model->data(idx, Qt::DisplayRole).toString();
            }

            for (int i = 0; i < colWidths[c] && xOff + i < geo.x() + viewWidth; ++i) {
                Cell &cell = buffer.backCell(xOff + i, screenY);
                cell.character = (i < cellText.length()) ? cellText.at(i) : QLatin1Char(' ');
                cell.bold = false;
                // Highlight current cell
                bool isCurrent = (modelRow == m_currentRow && c == m_currentCol && modelRow < rows);
                if (isCurrent) {
                    cell.foreground = theme.focusForeground;
                    cell.background = theme.focusBackground;
                } else {
                    cell.foreground = theme.foreground;
                    cell.background = theme.background;
                }
                cell.inverse = isCurrent;
            }
            xOff += colWidths[c];
        }
    }
}

void QTuiTableView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_model || m_model->rowCount() == 0 || m_model->columnCount() == 0)
        return;

    const int rowCount = m_model->rowCount();
    const int colCount = m_model->columnCount();

    switch (event->key()) {
    case Qt::Key_Up:
        m_currentRow = qMax(0, m_currentRow - 1);
        ensureCurrentVisible();
        break;
    case Qt::Key_Down:
        m_currentRow = qMin(rowCount - 1, m_currentRow + 1);
        ensureCurrentVisible();
        break;
    case Qt::Key_Left:
        m_currentCol = qMax(0, m_currentCol - 1);
        break;
    case Qt::Key_Right:
        m_currentCol = qMin(colCount - 1, m_currentCol + 1);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit activated(m_model->index(m_currentRow, m_currentCol));
        break;
    default:
        break;
    }
}

void QTuiTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    const QRect geo = geometry();
    const int visibleDataRows = geo.height() - m_headerHeight;
    if (visibleDataRows <= 0)
        return;

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + visibleDataRows - 1;

    // Clamp to valid model range
    int top = qBound(0, topLeft.row(), m_model->rowCount() - 1);
    int bottom = qBound(0, bottomRight.row(), m_model->rowCount() - 1);

    // Filter out signals entirely outside the visible viewport
    if (top > lastVisibleRow || bottom < firstVisibleRow)
        return;

    // Compute the visible dirty rows relative to the widget geometry
    int dirtyTop = qMax(top, firstVisibleRow) - m_scrollOffset + m_headerHeight;
    int dirtyBottom = qMin(bottom, lastVisibleRow) - m_scrollOffset + m_headerHeight;

    QRect dirtyRect(geo.x(), geo.y() + dirtyTop,
                    geo.width(), dirtyBottom - dirtyTop + 1);
    markDirty(dirtyRect);
}

void QTuiTableView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    if (!m_model)
        return;

    const QRect geo = geometry();
    const int visibleDataRows = geo.height() - m_headerHeight;
    if (visibleDataRows <= 0) {
        clampCurrentIndex();
        return;
    }

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + visibleDataRows - 1;

    // If insertion is entirely below the visible viewport, no dirty marking needed
    if (first > lastVisibleRow) {
        clampCurrentIndex();
        return;
    }

    // Mark from insertion point (or top of viewport) to bottom of viewport
    int dirtyTop = qMax(first, firstVisibleRow) - m_scrollOffset + m_headerHeight;
    QRect dirtyRect(geo.x(), geo.y() + dirtyTop,
                    geo.width(), visibleDataRows - dirtyTop + m_headerHeight);
    markDirty(dirtyRect);

    clampCurrentIndex();
}

void QTuiTableView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    if (!m_model)
        return;

    const QRect geo = geometry();
    const int visibleDataRows = geo.height() - m_headerHeight;
    if (visibleDataRows <= 0) {
        clampCurrentIndex();
        ensureCurrentVisible();
        return;
    }

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + visibleDataRows - 1;

    // If removal is entirely below the visible viewport, no dirty marking needed
    if (first > lastVisibleRow) {
        clampCurrentIndex();
        ensureCurrentVisible();
        return;
    }

    // Mark from removal point (or top of viewport) to bottom of viewport
    int dirtyTop = qMax(first, firstVisibleRow) - m_scrollOffset + m_headerHeight;
    QRect dirtyRect(geo.x(), geo.y() + dirtyTop,
                    geo.width(), visibleDataRows - dirtyTop + m_headerHeight);
    markDirty(dirtyRect);

    clampCurrentIndex();
    ensureCurrentVisible();
}

void QTuiTableView::onModelReset()
{
    m_currentRow = 0;
    m_currentCol = 0;
    m_scrollOffset = 0;
    markDirty();
}

void QTuiTableView::disconnectModel()
{
    if (!m_model)
        return;

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &QTuiTableView::onDataChanged);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &QTuiTableView::onRowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &QTuiTableView::onRowsRemoved);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiTableView::onModelReset);
}

void QTuiTableView::connectModel()
{
    if (!m_model)
        return;

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &QTuiTableView::onDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QTuiTableView::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QTuiTableView::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiTableView::onModelReset);
}

void QTuiTableView::ensureCurrentVisible()
{
    if (!m_model)
        return;

    int visibleDataRows = geometry().height() - m_headerHeight;
    if (visibleDataRows <= 0)
        return;

    if (m_currentRow < m_scrollOffset)
        m_scrollOffset = m_currentRow;
    else if (m_currentRow >= m_scrollOffset + visibleDataRows)
        m_scrollOffset = m_currentRow - visibleDataRows + 1;
}

void QTuiTableView::clampCurrentIndex()
{
    if (!m_model || m_model->rowCount() == 0) {
        m_currentRow = 0;
        m_currentCol = 0;
        return;
    }
    m_currentRow = qBound(0, m_currentRow, m_model->rowCount() - 1);
    m_currentCol = qBound(0, m_currentCol, qMax(0, m_model->columnCount() - 1));
}

} // namespace QtTUI
