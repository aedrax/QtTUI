#include <QtTUI/QTuiTableView.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QDebug>

namespace QtTUI {

QTuiTableView::QTuiTableView(QTuiWidget *parent)
    : QTuiAbstractItemView(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp scrolling whenever our geometry
    // changes so the current cell stays visible after a shrink or grow.
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        if (!m_model)
            return;
        const int rows = m_model->rowCount();
        const int visibleDataRows = geometry().height() - m_headerHeight;
        if (visibleDataRows > 0)
            m_scrollOffset = qBound(0, m_scrollOffset, qMax(0, rows - visibleDataRows));
        clampCurrentIndex();
        ensureCurrentVisible();
        ensureCurrentColumnVisible();
        markDirty();
    });
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
    invalidateColumnWidths();

    if (m_model)
        connectModel();
    resetSelectionModel();
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
    int row = qBound(0, index.row(), qMax(0, m_model->rowCount() - 1));
    int col = qBound(0, index.column(), qMax(0, m_model->columnCount() - 1));
    if (row != m_currentRow || col != m_currentCol) {
        m_currentRow = row;
        m_currentCol = col;
        ensureCurrentVisible();
        markDirty();
    }
    selectCurrent(Qt::NoModifier, false);
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

    // Final (post-scaling) column widths, shared with mouse hit testing and
    // horizontal scrolling.
    const QVector<int> colWidths = effectiveColumnWidths();

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
            QString header = headerText(c);
            renderTextTruncated(buffer, xOff, geo.y(), header, availWidth, headerStyle);
            // Fill remaining space in column with background
            int textLen = qMin(header.length(), availWidth);
            for (int i = textLen; i < availWidth; ++i) {
                Cell &cell = buffer.backCell(xOff + i, geo.y());
                cell.setChar(U' ');
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.bold = true;
                cell.inverse = false;
            }
            xOff += availWidth;
            // Render separator space between columns
            if (c < cols - 1 && xOff < geo.x() + viewWidth) {
                Cell &sep = buffer.backCell(xOff, geo.y());
                sep.setChar(U' ');
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
            cell.setChar(U' ');
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
                cell.setChar(U' ');
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
            QModelIndex idx;
            if (modelRow < rows) {
                idx = m_model->index(modelRow, c);
                cellText = m_model->data(idx, Qt::DisplayRole).toString();
            }

            bool isCurrent = (modelRow == m_currentRow && c == m_currentCol && modelRow < rows);
            const Cell cellStyle = itemStyle(idx, isCurrent, theme);

            // Render cell text using renderTextTruncated
            renderTextTruncated(buffer, xOff, screenY, cellText, availWidth, cellStyle);

            // Fill remaining space with background (clear old content)
            int textLen = qMin(cellText.length(), availWidth);
            for (int i = textLen; i < availWidth; ++i) {
                Cell &cell = buffer.backCell(xOff + i, screenY);
                cell.setChar(U' ');
                cell.foreground = cellStyle.foreground;
                cell.background = cellStyle.background;
                cell.bold = false;
                cell.inverse = false;
                cell.underline = cellStyle.underline;
            }
            xOff += availWidth;

            // Render separator space between columns. Between two selected
            // cells it has the selection color, so a selected row is one bar.
            if (c < cols - 1 && xOff < geo.x() + viewWidth) {
                const bool joined = isSelected(idx)
                    && isSelected(m_model->index(modelRow, c + 1));
                Cell &sep = buffer.backCell(xOff, screenY);
                sep.setChar(U' ');
                sep.foreground = theme.foreground;
                sep.background = joined ? cellStyle.background : theme.background;
                sep.underline = false;
                sep.bold = false;
                sep.inverse = false;
                xOff++;
            }
        }
        // Fill remaining row with background
        for (int x = xOff; x < geo.x() + viewWidth; ++x) {
            Cell &cell = buffer.backCell(x, screenY);
            cell.setChar(U' ');
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

    if (selectionKeyPressEvent(event))
        return;

    const int rowCount = m_model->rowCount();
    const int colCount = m_model->columnCount();
    const QModelIndex previous = currentIndex();

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

    if (currentIndex() != previous)
        selectCurrent(event->modifiers(), false);
    markDirty();
}

bool QTuiTableView::isSortingEnabled() const
{
    return m_sortingEnabled;
}

void QTuiTableView::setSortingEnabled(bool enable)
{
    if (m_sortingEnabled == enable)
        return;
    m_sortingEnabled = enable;
    invalidateColumnWidths();
    markDirty();
}

void QTuiTableView::sortByColumn(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;
    if (m_model && column >= 0)
        m_model->sort(column, order); // the model emits layoutChanged()
    invalidateColumnWidths();
    markDirty();
}

int QTuiTableView::sortColumn() const
{
    return m_sortColumn;
}

Qt::SortOrder QTuiTableView::sortOrder() const
{
    return m_sortOrder;
}

int QTuiTableView::columnWidth(int column) const
{
    return effectiveColumnWidths().value(column);
}

void QTuiTableView::setColumnWidth(int column, int width)
{
    if (column < 0)
        return;
    m_userColumnWidths.insert(column, qMax(1, width));
    invalidateColumnWidths();
    markDirty();
}

QString QTuiTableView::headerText(int column) const
{
    QString text = m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();
    if (m_sortingEnabled && column == m_sortColumn)
        text += m_sortOrder == Qt::AscendingOrder ? QStringLiteral(" \u25B2") : QStringLiteral(" \u25BC");
    return text;
}

void QTuiTableView::mousePressEvent(TuiMouseEvent *event)
{
    // The Press before the DoubleClick already made the item current
    if (event->mouseType() == TuiMouseEvent::DoubleClick) {
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return;
        emit doubleClicked(index);
        emit activated(index);
        return;
    }
    // A header drag resizes the column that it started on
    if (m_resizingColumn >= 0) {
        if (event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton) {
            setColumnWidth(m_resizingColumn, event->x() - geometry().x() - m_resizeColumnX);
            return;
        }
        if (event->mouseType() == TuiMouseEvent::Release) {
            m_resizingColumn = -1;
            return;
        }
    }
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    if (!m_model || m_model->rowCount() == 0 || m_model->columnCount() == 0)
        return;

    const QRect geo = geometry();
    const int cols = m_model->columnCount();
    const int rows = m_model->rowCount();

    // Calculate clicked row (subtract 1 for header)
    const bool onHeader = event->y() - geo.y() < m_headerHeight;
    int clickedRow = (event->y() - geo.y() - m_headerHeight) + m_scrollOffset;
    if (!onHeader && (clickedRow < 0 || clickedRow >= rows))
        return;

    // Same final column widths as paintEvent
    const QVector<int> colWidths = effectiveColumnWidths();

    // Determine which column was clicked
    int clickX = event->x() - geo.x();
    int xOff = 0;
    int clickedCol = -1;
    int firstVisibleCol = m_horizontalScrollOffset;
    if (firstVisibleCol >= cols)
        firstVisibleCol = 0;

    for (int c = firstVisibleCol; c < cols; ++c) {
        int colEnd = xOff + colWidths[c];
        if (clickX >= xOff && clickX < colEnd) {
            clickedCol = c;
            break;
        }
        // A left press on the separator after a header starts a resize
        if (onHeader && clickX == colEnd && c < cols - 1
            && event->button() == Qt::LeftButton) {
            m_resizingColumn = c;
            m_resizeColumnX = xOff;
            return;
        }
        xOff = colEnd + 1; // +1 for separator
    }

    if (clickedCol < 0)
        clickedCol = cols - 1;

    // A header click sorts: a new column ascending, the same column reversed
    if (onHeader) {
        if (m_sortingEnabled) {
            const Qt::SortOrder order = (clickedCol == m_sortColumn && m_sortOrder == Qt::AscendingOrder)
                ? Qt::DescendingOrder : Qt::AscendingOrder;
            sortByColumn(clickedCol, order);
        }
        return;
    }

    // Another button keeps a selection that contains the cell
    m_currentRow = clickedRow;
    m_currentCol = clickedCol;
    markDirty();
    if (event->button() == Qt::LeftButton)
        selectCurrent(event->modifiers(), true);
    else if (!isSelected(currentIndex()))
        selectCurrent(Qt::NoModifier, true);
}

void QTuiTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    invalidateColumnWidths();

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

    invalidateColumnWidths();

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

    invalidateColumnWidths();

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

void QTuiTableView::onColumnsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if (!m_model)
        return;

    invalidateColumnWidths();
    clampCurrentIndex();
    m_horizontalScrollOffset = qBound(0, m_horizontalScrollOffset,
                                      qMax(0, m_model->columnCount() - 1));
    ensureCurrentColumnVisible();
    markDirty();
}

void QTuiTableView::onColumnsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if (!m_model)
        return;

    invalidateColumnWidths();
    clampCurrentIndex();
    m_horizontalScrollOffset = qBound(0, m_horizontalScrollOffset,
                                      qMax(0, m_model->columnCount() - 1));
    ensureCurrentColumnVisible();
    markDirty();
}

void QTuiTableView::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_UNUSED(first);
    Q_UNUSED(last);

    if (orientation != Qt::Horizontal)
        return;

    // Header text feeds the column width calculation
    invalidateColumnWidths();

    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;
    markDirty(QRect(geo.x(), geo.y(), geo.width(), m_headerHeight));
}

void QTuiTableView::onLayoutChanged()
{
    // Rows/columns may have been reordered wholesale (e.g. proxy sort).
    // Treat cached row/column-derived state as stale.
    invalidateColumnWidths();
    clampCurrentIndex();
    ensureCurrentVisible();
    ensureCurrentColumnVisible();
    markDirty();
}

void QTuiTableView::onModelReset()
{
    m_currentRow = 0;
    m_currentCol = 0;
    m_scrollOffset = 0;
    m_horizontalScrollOffset = 0;
    invalidateColumnWidths();
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
    disconnect(m_model, &QAbstractItemModel::columnsInserted,
               this, &QTuiTableView::onColumnsInserted);
    disconnect(m_model, &QAbstractItemModel::columnsRemoved,
               this, &QTuiTableView::onColumnsRemoved);
    disconnect(m_model, &QAbstractItemModel::headerDataChanged,
               this, &QTuiTableView::onHeaderDataChanged);
    disconnect(m_model, &QAbstractItemModel::layoutChanged,
               this, &QTuiTableView::onLayoutChanged);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiTableView::onModelReset);
    disconnect(m_model, &QObject::destroyed, this, nullptr);
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
    connect(m_model, &QAbstractItemModel::columnsInserted,
            this, &QTuiTableView::onColumnsInserted);
    connect(m_model, &QAbstractItemModel::columnsRemoved,
            this, &QTuiTableView::onColumnsRemoved);
    connect(m_model, &QAbstractItemModel::headerDataChanged,
            this, &QTuiTableView::onHeaderDataChanged);
    connect(m_model, &QAbstractItemModel::layoutChanged,
            this, &QTuiTableView::onLayoutChanged);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiTableView::onModelReset);
    // Guard against the model being deleted while still set on the view
    connect(m_model, &QObject::destroyed, this, [this]() {
        m_model = nullptr;
        onModelReset();
    });
}

void QTuiTableView::ensureCurrentVisible()
{
    if (!m_model)
        return;

    scrollRowIntoView(m_currentRow, geometry().height() - m_headerHeight);
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

    const int viewWidth = geometry().width();
    if (viewWidth <= 0)
        return;

    // Use the same final (post-scaling) widths that paintEvent renders with,
    // so scrolling and painting always agree on which columns are visible.
    const QVector<int> colWidths = effectiveColumnWidths();
    if (colWidths.size() != cols)
        return;

    // Last fully visible column when rendering starts at firstCol
    const auto lastVisibleFrom = [&](int firstCol) {
        int usedWidth = 0;
        int lastVisibleCol = firstCol;
        for (int c = firstCol; c < cols; ++c) {
            int needed = colWidths.at(c) + (c > firstCol ? 1 : 0); // +1 separator
            if (usedWidth + needed > viewWidth)
                break;
            usedWidth += needed;
            lastVisibleCol = c;
        }
        return lastVisibleCol;
    };

    // Scroll right until the current column is visible
    while (m_horizontalScrollOffset < m_currentCol
           && m_currentCol > lastVisibleFrom(m_horizontalScrollOffset)) {
        m_horizontalScrollOffset++;
    }
}

void QTuiTableView::invalidateColumnWidths()
{
    m_columnWidthsStale = true;
}

const QVector<int> &QTuiTableView::effectiveColumnWidths() const
{
    const QRect geo = geometry();

    // The cache is keyed on the viewport: sampling depends on the vertical
    // scroll offset and the geometry, scaling depends on the view width.
    if (!m_columnWidthsStale
        && m_columnWidthsScrollOffset == m_scrollOffset
        && m_columnWidthsGeometry == geo) {
        return m_cachedColumnWidths;
    }

    m_columnWidthsStale = false;
    m_columnWidthsScrollOffset = m_scrollOffset;
    m_columnWidthsGeometry = geo;
    m_cachedColumnWidths.clear();

    if (!m_model)
        return m_cachedColumnWidths;

    const int cols = m_model->columnCount();
    const int rows = m_model->rowCount();
    if (cols == 0)
        return m_cachedColumnWidths;

    // Sample only the header plus the rows currently in the viewport instead
    // of scanning the entire model on every paint (O(visible) not O(rows)).
    // If the geometry has no data rows yet, fall back to a bounded sample
    // from the top of the model.
    static constexpr int FallbackSampleRows = 100;
    const int visibleDataRows = geo.height() - m_headerHeight;
    const int sampleFirst = qBound(0, m_scrollOffset, qMax(0, rows - 1));
    const int sampleCount = visibleDataRows > 0 ? visibleDataRows : FallbackSampleRows;
    const int sampleEnd = qMin(rows, sampleFirst + sampleCount);

    // Step 1: Natural width per column = max(header width, widest sampled cell)
    QVector<int> colWidths(cols, 0);
    for (int c = 0; c < cols; ++c) {
        QString header = headerText(c);
        int maxW = header.length();
        for (int r = sampleFirst; r < sampleEnd; ++r) {
            QModelIndex idx = m_model->index(r, c);
            const int len = m_model->data(idx, Qt::DisplayRole).toString().length();
            if (len > maxW)
                maxW = len;
        }
        colWidths[c] = qMax(1, maxW); // at least 1 char per column
        // A width that the user set replaces the natural width
        if (m_userColumnWidths.contains(c))
            colWidths[c] = m_userColumnWidths.value(c);
    }

    // Step 2: Constrain total column widths to view width
    // Account for separators: 1 space between each pair of columns
    const int viewWidth = geo.width();
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
            // Add extra chars to leading columns first
            for (int c = 0; c < cols && diff > 0; ++c) {
                scaled[c]++;
                diff--;
            }
        } else if (diff < 0) {
            // Remove chars from trailing columns (but keep minimum 1)
            for (int c = cols - 1; c >= 0 && diff < 0; --c) {
                if (scaled[c] > 1) {
                    scaled[c]--;
                    diff++;
                }
            }
        }
        colWidths = scaled;
    }

    m_cachedColumnWidths = colWidths;
    return m_cachedColumnWidths;
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
