#include <QtTUI/QTuiTableView.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
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
    m_horizontalScrollOffset = 0;

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

int QTuiTableView::horizontalScrollOffset() const
{
    return m_horizontalScrollOffset;
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

    if (cols == 0)
        return;

    // --- Content-based column width calculation ---
    // Step 1: Compute natural width for each column as max(header width, max data cell width)
    QVector<int> colWidths(cols, 0);
    for (int c = 0; c < cols; ++c) {
        QString header = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        int maxW = header.length();
        for (int r = 0; r < rows; ++r) {
            QModelIndex idx = m_model->index(r, c);
            QString cellText = m_model->data(idx, Qt::DisplayRole).toString();
            if (cellText.length() > maxW)
                maxW = cellText.length();
        }
        colWidths[c] = qMax(1, maxW); // at least 1 char per column
    }

    // Step 2: Constrain total column widths to view width
    // Account for separators: 1 space between each pair of columns
    int separatorTotal = (cols > 1) ? (cols - 1) : 0;
    int availableForCols = viewWidth - separatorTotal;
    if (availableForCols < cols)
        availableForCols = cols; // ensure at least 1 char per column

    int totalNatural = 0;
    for (int c = 0; c < cols; ++c)
        totalNatural += colWidths[c];

    if (totalNatural > availableForCols) {
        // Proportionally scale down, ensuring each column gets at least 1 char
        QVector<int> scaled(cols, 0);
        for (int c = 0; c < cols; ++c) {
            int proportional = static_cast<int>(
                static_cast<double>(colWidths[c]) / totalNatural * availableForCols);
            if (proportional < 1)
                proportional = 1;
            scaled[c] = proportional;
        }
        // Adjust to fit exactly in availableForCols
        int scaledTotal = 0;
        for (int c = 0; c < cols; ++c)
            scaledTotal += scaled[c];

        // Distribute remainder or trim excess
        int diff = availableForCols - scaledTotal;
        if (diff > 0) {
            // Add extra chars to widest columns first
            for (int c = 0; c < cols && diff > 0; ++c) {
                scaled[c]++;
                diff--;
            }
        } else if (diff < 0) {
            // Remove chars from widest columns (but keep minimum 1)
            for (int c = cols - 1; c >= 0 && diff < 0; --c) {
                if (scaled[c] > 1) {
                    scaled[c]--;
                    diff++;
                }
            }
        }
        colWidths = scaled;
    }

    // --- Determine visible columns based on horizontal scroll offset ---
    int firstVisibleCol = m_horizontalScrollOffset;
    if (firstVisibleCol >= cols)
        firstVisibleCol = 0;

    // --- Render column headers on first row with bold text ---
    {
        Cell headerStyle;
        headerStyle.foreground = theme.foreground;
        headerStyle.background = theme.background;
        headerStyle.bold = true;
        headerStyle.inverse = false;

        int xOff = geo.x();
        for (int c = firstVisibleCol; c < cols; ++c) {
            if (xOff >= geo.x() + viewWidth)
                break;
            int availWidth = qMin(colWidths[c], geo.x() + viewWidth - xOff);
            QString header = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
            renderTextTruncated(buffer, xOff, geo.y(), header, availWidth, headerStyle);
            // Fill remaining space in column with background
            int textLen = qMin(header.length(), availWidth);
            for (int i = textLen; i < availWidth; ++i) {
                Cell &cell = buffer.backCell(xOff + i, geo.y());
                cell.character = QLatin1Char(' ');
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.bold = true;
                cell.inverse = false;
            }
            xOff += availWidth;
            // Render separator space between columns
            if (c < cols - 1 && xOff < geo.x() + viewWidth) {
                Cell &sep = buffer.backCell(xOff, geo.y());
                sep.character = QLatin1Char(' ');
                sep.foreground = theme.foreground;
                sep.background = theme.background;
                sep.bold = false;
                sep.inverse = false;
                xOff++;
            }
        }
        // Fill remaining header row with background
        for (int x = xOff; x < geo.x() + viewWidth; ++x) {
            Cell &cell = buffer.backCell(x, geo.y());
            cell.character = QLatin1Char(' ');
            cell.foreground = theme.foreground;
            cell.background = theme.background;
            cell.bold = false;
            cell.inverse = false;
        }
    }

    // --- Handle empty model (0 rows): fill remaining with theme background ---
    if (rows == 0) {
        int visibleDataRows = viewHeight - m_headerHeight;
        for (int vr = 0; vr < visibleDataRows; ++vr) {
            int screenY = geo.y() + m_headerHeight + vr;
            for (int x = geo.x(); x < geo.x() + viewWidth; ++x) {
                Cell &cell = buffer.backCell(x, screenY);
                cell.character = QLatin1Char(' ');
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.bold = false;
                cell.inverse = false;
            }
        }
        return;
    }

    // --- Render visible data rows ---
    int visibleDataRows = viewHeight - m_headerHeight;
    for (int vr = 0; vr < visibleDataRows; ++vr) {
        int modelRow = m_scrollOffset + vr;
        int screenY = geo.y() + m_headerHeight + vr;

        int xOff = geo.x();
        for (int c = firstVisibleCol; c < cols; ++c) {
            if (xOff >= geo.x() + viewWidth)
                break;
            int availWidth = qMin(colWidths[c], geo.x() + viewWidth - xOff);

            QString cellText;
            if (modelRow < rows) {
                QModelIndex idx = m_model->index(modelRow, c);
                cellText = m_model->data(idx, Qt::DisplayRole).toString();
            }

            // Determine style based on focus highlighting
            // Use focusForeground/focusBackground only when widget has focus
            bool isCurrent = (modelRow == m_currentRow && c == m_currentCol && modelRow < rows);
            Cell cellStyle;
            if (isCurrent && hasFocus()) {
                cellStyle.foreground = theme.focusForeground;
                cellStyle.background = theme.focusBackground;
            } else {
                cellStyle.foreground = theme.foreground;
                cellStyle.background = theme.background;
            }
            cellStyle.bold = false;
            cellStyle.inverse = false;

            // Render cell text using renderTextTruncated
            renderTextTruncated(buffer, xOff, screenY, cellText, availWidth, cellStyle);

            // Fill remaining space with background (clear old content)
            int textLen = qMin(cellText.length(), availWidth);
            for (int i = textLen; i < availWidth; ++i) {
                Cell &cell = buffer.backCell(xOff + i, screenY);
                cell.character = QLatin1Char(' ');
                cell.foreground = cellStyle.foreground;
                cell.background = cellStyle.background;
                cell.bold = false;
                cell.inverse = false;
            }
            xOff += availWidth;

            // Render separator space between columns
            if (c < cols - 1 && xOff < geo.x() + viewWidth) {
                Cell &sep = buffer.backCell(xOff, screenY);
                sep.character = QLatin1Char(' ');
                sep.foreground = theme.foreground;
                sep.background = theme.background;
                sep.bold = false;
                sep.inverse = false;
                xOff++;
            }
        }
        // Fill remaining row with background
        for (int x = xOff; x < geo.x() + viewWidth; ++x) {
            Cell &cell = buffer.backCell(x, screenY);
            cell.character = QLatin1Char(' ');
            cell.foreground = theme.foreground;
            cell.background = theme.background;
            cell.bold = false;
            cell.inverse = false;
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
        ensureCurrentColumnVisible();
        break;
    case Qt::Key_Right:
        m_currentCol = qMin(colCount - 1, m_currentCol + 1);
        ensureCurrentColumnVisible();
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
    m_horizontalScrollOffset = 0;
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

void QTuiTableView::ensureCurrentColumnVisible()
{
    if (!m_model)
        return;

    const int cols = m_model->columnCount();
    if (cols == 0)
        return;

    // If current column is before the horizontal scroll offset, scroll left
    if (m_currentCol < m_horizontalScrollOffset)
        m_horizontalScrollOffset = m_currentCol;

    // If current column is beyond the visible range, scroll right
    // Compute how many columns fit starting from m_horizontalScrollOffset
    const int viewWidth = geometry().width();
    int usedWidth = 0;
    int lastVisibleCol = m_horizontalScrollOffset;
    for (int c = m_horizontalScrollOffset; c < cols; ++c) {
        // Compute column width (same logic as paintEvent)
        QString header = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        int colW = header.length();
        int rows = m_model->rowCount();
        for (int r = 0; r < rows; ++r) {
            QModelIndex idx = m_model->index(r, c);
            QString cellText = m_model->data(idx, Qt::DisplayRole).toString();
            if (cellText.length() > colW)
                colW = cellText.length();
        }
        colW = qMax(1, colW);

        int needed = colW + (c > m_horizontalScrollOffset ? 1 : 0); // +1 for separator
        if (usedWidth + needed > viewWidth)
            break;
        usedWidth += needed;
        lastVisibleCol = c;
    }

    if (m_currentCol > lastVisibleCol) {
        // Scroll right until current column is visible
        while (m_horizontalScrollOffset < m_currentCol) {
            m_horizontalScrollOffset++;
            // Recompute last visible col
            usedWidth = 0;
            lastVisibleCol = m_horizontalScrollOffset;
            for (int c = m_horizontalScrollOffset; c < cols; ++c) {
                QString header = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
                int colW = header.length();
                int rows = m_model->rowCount();
                for (int r = 0; r < rows; ++r) {
                    QModelIndex idx = m_model->index(r, c);
                    QString cellText = m_model->data(idx, Qt::DisplayRole).toString();
                    if (cellText.length() > colW)
                        colW = cellText.length();
                }
                colW = qMax(1, colW);
                int needed = colW + (c > m_horizontalScrollOffset ? 1 : 0);
                if (usedWidth + needed > viewWidth)
                    break;
                usedWidth += needed;
                lastVisibleCol = c;
            }
            if (m_currentCol <= lastVisibleCol)
                break;
        }
    }
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
