#include <QtTUI/QTuiTableView.h>
#include <climits>
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
    m_header = new QTuiHeaderView(Qt::Horizontal, this);
    m_header->setFocusPolicy(Qt::NoFocus);
    m_header->setVisible(false);
    const auto headerChanged = [this]() {
        invalidateColumnWidths();
        markDirty();
    };
    connect(m_header, &QTuiHeaderView::sectionResized, this, headerChanged);
    connect(m_header, &QTuiHeaderView::geometriesChanged, this, headerChanged);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp scrolling whenever our geometry
    // changes so the current cell stays visible after a shrink or grow.
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        if (!m_model)
            return;
        clampScrollOffset(geometry().height() - m_headerHeight);
        clampCurrentIndex();
        ensureCurrentVisible();
        ensureColumnVisible(m_currentCol);
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
    clearHiddenRows();
    remeasureColumnWidths();

    if (m_model)
        connectModel();
    m_header->setModel(m_model);
    resetSelectionModel();
}

QModelIndex QTuiTableView::currentIndex() const
{
    if (shownRows() == 0 || m_model->columnCount() == 0)
        return QModelIndex();
    return m_model->index(m_currentRow, m_currentCol);
}

void QTuiTableView::setCurrentIndex(const QModelIndex &index)
{
    if (shownRows() == 0)
        return;
    // A hidden row cannot be current: the next row that shows is
    const int row = shownRowNear(index.row());
    int col = qBound(0, index.column(), qMax(0, m_model->columnCount() - 1));
    if (row != m_currentRow || col != m_currentCol) {
        m_currentRow = row;
        m_currentCol = col;
        ensureCurrentVisible();
        ensureColumnVisible(m_currentCol);
        markDirty();
    }
    selectCurrent(Qt::NoModifier, false);
}

void QTuiTableView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (!m_model || index.model() != m_model || index.parent().isValid())
        return;
    ensureColumnVisible(index.column());
    if (!isHiddenRow(index.row()))
        scrollToRow(visualRow(index.row()), geometry().height() - m_headerHeight, shownRows(), hint);
}

QRect QTuiTableView::visualRect(const QModelIndex &index) const
{
    if (!m_model || index.model() != m_model || index.parent().isValid())
        return QRect();
    const QRect geo = geometry();
    const int line = visualRow(index.row()) - m_scrollOffset;
    if (line < 0 || line >= geo.height() - m_headerHeight || isHiddenRow(index.row()))
        return QRect();
    // The same column positions as paintEvent: a separator after each column
    const QVector<int> colWidths = effectiveColumnWidths();
    int x = geo.x();
    for (int c = m_horizontalScrollOffset; c < colWidths.size() && x < geo.right() + 1; ++c) {
        if (isColumnHidden(c))
            continue;
        const int width = qMin(colWidths.at(c), geo.right() + 1 - x);
        if (c == index.column())
            return QRect(x, geo.y() + m_headerHeight + line, width, 1);
        x += width + 1;
    }
    return QRect();
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
    const int rows = shownRows();
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
    // The separators between the columns and the rest of each row use the
    // plain theme colors.
    const Cell plain = cellStyle(theme.foreground, theme.background);
    {
        Cell headerStyle = plain;
        headerStyle.bold = true;
        fillRect(buffer, QRect(geo.x(), geo.y(), viewWidth, 1), plain);

        int xOff = geo.x();
        for (int c = firstVisibleCol; c < cols; ++c) {
            if (isColumnHidden(c))
                continue;
            if (xOff >= geo.x() + viewWidth)
                break;
            int availWidth = qMin(colWidths[c], geo.x() + viewWidth - xOff);
            fillRect(buffer, QRect(xOff, geo.y(), availWidth, 1), headerStyle);
            renderTextTruncated(buffer, xOff, geo.y(), headerText(c), availWidth, headerStyle);
            xOff += availWidth + 1;
        }
    }

    // --- Handle empty model (0 rows): fill remaining with theme background ---
    int visibleDataRows = viewHeight - m_headerHeight;
    if (rows == 0) {
        fillRect(buffer, QRect(geo.x(), geo.y() + m_headerHeight, viewWidth, visibleDataRows), plain);
        return;
    }

    // --- Render visible data rows ---
    for (int vr = 0; vr < visibleDataRows; ++vr) {
        const bool hasRow = m_scrollOffset + vr < rows;
        const int modelRow = hasRow ? logicalRow(m_scrollOffset + vr) : -1;
        int screenY = geo.y() + m_headerHeight + vr;
        fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1), plain);

        int xOff = geo.x();
        for (int c = firstVisibleCol; c < cols; ++c) {
            if (isColumnHidden(c))
                continue;
            if (xOff >= geo.x() + viewWidth)
                break;
            int availWidth = qMin(colWidths[c], geo.x() + viewWidth - xOff);

            QString cellText;
            QModelIndex idx;
            if (hasRow) {
                idx = m_model->index(modelRow, c);
                cellText = m_model->data(idx, Qt::DisplayRole).toString();
            }

            bool isCurrent = (modelRow == m_currentRow && c == m_currentCol);
            const Cell style = itemStyle(idx, isCurrent, theme);

            // The space after the text has the item colors, without bold or inverse
            Cell padStyle = style;
            padStyle.bold = false;
            padStyle.inverse = false;
            fillRect(buffer, QRect(xOff, screenY, availWidth, 1), padStyle);
            renderTextTruncated(buffer, xOff, screenY, cellText, availWidth, style);
            xOff += availWidth;

            // Render separator space between columns. Between two selected
            // cells it has the selection color, so a selected row is one bar.
            const int next = movedColumn(c, 1);
            if (next != c && xOff < geo.x() + viewWidth) {
                if (isSelected(idx) && isSelected(m_model->index(modelRow, next)))
                    fillRect(buffer, QRect(xOff, screenY, 1, 1), cellStyle(theme.foreground, style.background));
                xOff++;
            }
        }
    }
}

void QTuiTableView::keyPressEvent(TuiKeyEvent *event)
{
    if (shownRows() == 0 || m_model->columnCount() == 0)
        return;

    if (editKeyPressEvent(event) || selectionKeyPressEvent(event))
        return;

    // Alt+Left and Alt+Right make the current column narrower or wider,
    // as a drag on the header does. Alt+Up and Alt+Down sort by it, as a
    // header click does.
    if (event->modifiers() == Qt::AltModifier) {
        const bool resizable = m_header->sectionResizeMode(m_currentCol) == QTuiHeaderView::Interactive;
        switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
            if (resizable)
                setColumnWidth(m_currentCol, columnWidth(m_currentCol)
                                                 + (event->key() == Qt::Key_Left ? -1 : 1));
            return;
        case Qt::Key_Up:
        case Qt::Key_Down:
            if (m_sortingEnabled)
                sortByColumn(m_currentCol, event->key() == Qt::Key_Up ? Qt::AscendingOrder
                                                                      : Qt::DescendingOrder);
            return;
        default:
            break;
        }
    }

    const QModelIndex previous = currentIndex();
    CursorAction action;
    Qt::KeyboardModifiers modifiers;
    // Home and End (also with Ctrl) go to the first and the last row and
    // keep the column: a cell has no text cursor to move
    if (cursorAction(event, &action, &modifiers)) {
        if (action == MoveLeft || action == MoveRight) {
            m_currentCol = movedColumn(m_currentCol, action == MoveLeft ? -1 : 1);
            ensureColumnVisible(m_currentCol);
        } else {
            m_currentRow = logicalRow(movedRow(action, visualRow(m_currentRow), shownRows(),
                                               geometry().height() - m_headerHeight));
            ensureCurrentVisible();
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit activated(m_model->index(m_currentRow, m_currentCol));
    }

    if (currentIndex() != previous)
        selectCurrent(modifiers, false);
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
    // The natural widths keep space for the sort indicator
    remeasureColumnWidths();
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
    m_header->resizeSection(column, width);
}

bool QTuiTableView::isColumnHidden(int column) const
{
    return m_header->isSectionHidden(column);
}

void QTuiTableView::setColumnHidden(int column, bool hide)
{
    if (!m_model || hide == isColumnHidden(column))
        return;
    m_header->setSectionHidden(column, hide);
    // The current cell must be in a column that shows
    if (isColumnHidden(m_currentCol)) {
        int col = movedColumn(m_currentCol, 1);
        if (col == m_currentCol)
            col = movedColumn(m_currentCol, -1);
        setCurrentIndex(m_model->index(m_currentRow, col));
    }
    invalidateColumnWidths();
    ensureColumnVisible(m_currentCol);
    updateEditorGeometry();
    markDirty();
}

bool QTuiTableView::isRowHidden(int row) const
{
    return isHiddenRow(row);
}

void QTuiTableView::setRowHidden(int row, bool hide)
{
    if (!m_model || row < 0 || row >= m_model->rowCount() || !setHiddenRow(row, hide))
        return;
    // The current cell must be in a row that shows
    if (hide && row == m_currentRow && shownRows() > 0)
        setCurrentIndex(m_model->index(row, m_currentCol));
    clampScrollOffset(geometry().height() - m_headerHeight);
    ensureCurrentVisible();
    invalidateColumnWidths();
    updateEditorGeometry();
    markRowsDirty(visualRow(row), INT_MAX, m_headerHeight);
}

int QTuiTableView::movedColumn(int column, int step) const
{
    const int cols = m_model ? m_model->columnCount() : 0;
    for (int c = column + step; c >= 0 && c < cols; c += step) {
        if (!isColumnHidden(c))
            return c;
    }
    return column;
}

QTuiHeaderView *QTuiTableView::horizontalHeader() const
{
    return m_header;
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
    // Shift+wheel scrolls one column per notch. Some terminals keep
    // Shift+wheel for their own scrollback.
    if (m_model && event->mouseType() == TuiMouseEvent::Wheel
        && (event->modifiers() & Qt::ShiftModifier)) {
        const int column = m_horizontalScrollOffset - event->wheelDelta() / 120;
        m_horizontalScrollOffset = qBound(0, column, qMax(0, m_model->columnCount() - 1));
        updateEditorGeometry();
        markDirty();
        return;
    }
    if (m_model && wheelScroll(event, &m_scrollOffset, geometry().height() - m_headerHeight,
                               shownRows()))
        return;
    // The Press before the DoubleClick already made the item current
    if (event->mouseType() == TuiMouseEvent::DoubleClick) {
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return;
        emit doubleClicked(index);
        // No editor for a double click on the header
        const bool onHeader = event->y() - geometry().y() < m_headerHeight;
        if (!onHeader && (editTriggers() & DoubleClicked) && edit(index))
            return;
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
    // A left drag selects a range, as Shift+click does (see QTuiListView)
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
        return;
    if (shownRows() == 0 || m_model->columnCount() == 0)
        return;

    const QRect geo = geometry();
    const int cols = m_model->columnCount();

    // Calculate clicked row (subtract 1 for header)
    const bool onHeader = event->y() - geo.y() < m_headerHeight;
    const int clickedLine = (event->y() - geo.y() - m_headerHeight) + m_scrollOffset;
    if (drag && onHeader)
        return;
    if (!onHeader && (clickedLine < 0 || clickedLine >= shownRows()))
        return;
    const int clickedRow = logicalRow(clickedLine);

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
        if (isColumnHidden(c))
            continue;
        int colEnd = xOff + colWidths[c];
        if (clickX >= xOff && clickX < colEnd) {
            clickedCol = c;
            break;
        }
        // A left press on the separator after a header starts a resize
        if (onHeader && clickX == colEnd && movedColumn(c, 1) != c && event->button() == Qt::LeftButton
            && m_header->sectionResizeMode(c) == QTuiHeaderView::Interactive) {
            m_resizingColumn = c;
            m_resizeColumnX = xOff;
            return;
        }
        xOff = colEnd + 1; // +1 for separator
    }

    // A click after the last column goes to the last column that shows
    if (clickedCol < 0)
        clickedCol = movedColumn(cols, -1);
    if (clickedCol >= cols)
        return;

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
        selectCurrent(drag ? Qt::ShiftModifier : event->modifiers(), !drag);
    else if (!isSelected(currentIndex()))
        selectCurrent(Qt::NoModifier, true);
    if (event->button() == Qt::LeftButton && !drag)
        emit clicked(m_model->index(clickedRow, clickedCol));
}

void QTuiTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    invalidateColumnWidths();
    const int lastRow = m_model->rowCount() - 1;
    markRowsDirty(visualRow(qBound(0, topLeft.row(), lastRow)),
                  visualRow(qBound(0, bottomRight.row(), lastRow)), m_headerHeight);
}

void QTuiTableView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    if (!m_model || parent.isValid())
        return;

    shiftHiddenRows(first, last - first + 1);
    invalidateColumnWidths();
    // Mark from the insertion point to the bottom of the view
    markRowsDirty(visualRow(first), INT_MAX, m_headerHeight);
    clampCurrentIndex();
}

void QTuiTableView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    if (!m_model || parent.isValid())
        return;

    shiftHiddenRows(first, first - last - 1);
    invalidateColumnWidths();
    // Mark from the removal point to the bottom of the view
    markRowsDirty(visualRow(first), INT_MAX, m_headerHeight);
    clampCurrentIndex();
    ensureCurrentVisible();
}

void QTuiTableView::onColumnsChanged()
{
    if (!m_model)
        return;

    remeasureColumnWidths();
    clampCurrentIndex();
    m_horizontalScrollOffset = qBound(0, m_horizontalScrollOffset,
                                      qMax(0, m_model->columnCount() - 1));
    ensureColumnVisible(m_currentCol);
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
    ensureColumnVisible(m_currentCol);
    markDirty();
}

void QTuiTableView::onModelReset()
{
    m_currentRow = 0;
    m_currentCol = 0;
    m_scrollOffset = 0;
    m_horizontalScrollOffset = 0;
    clearHiddenRows();
    remeasureColumnWidths();
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
               this, &QTuiTableView::onColumnsChanged);
    disconnect(m_model, &QAbstractItemModel::columnsRemoved,
               this, &QTuiTableView::onColumnsChanged);
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
            this, &QTuiTableView::onColumnsChanged);
    connect(m_model, &QAbstractItemModel::columnsRemoved,
            this, &QTuiTableView::onColumnsChanged);
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

    scrollRowIntoView(visualRow(m_currentRow), geometry().height() - m_headerHeight);
}

void QTuiTableView::ensureColumnVisible(int column)
{
    if (!m_model)
        return;

    const int cols = m_model->columnCount();
    if (cols == 0)
        return;

    // If the column is before the horizontal scroll offset, scroll left
    if (column < m_horizontalScrollOffset)
        m_horizontalScrollOffset = column;

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
            if (isColumnHidden(c))
                continue;
            int needed = colWidths.at(c) + (usedWidth > 0 ? 1 : 0); // +1 separator
            if (usedWidth + needed > viewWidth)
                break;
            usedWidth += needed;
            lastVisibleCol = c;
        }
        return lastVisibleCol;
    };

    // Scroll right until the column is visible
    while (m_horizontalScrollOffset < column
           && column > lastVisibleFrom(m_horizontalScrollOffset)) {
        m_horizontalScrollOffset++;
    }
}

// Makes the next paint calculate the widths again. This does not discard
// the stored natural widths.
void QTuiTableView::invalidateColumnWidths()
{
    m_columnWidthsStale = true;
}

void QTuiTableView::remeasureColumnWidths()
{
    m_naturalWidths.clear();
    invalidateColumnWidths();
}

int QTuiTableView::measureColumn(int column) const
{
    // Sample only the rows in the viewport, not the full model. If the
    // geometry has no data rows yet, sample a bounded number of rows from
    // the top of the model.
    static constexpr int FallbackSampleRows = 100;
    const int rows = shownRows();
    const int visibleDataRows = geometry().height() - m_headerHeight;
    const int first = qBound(0, m_scrollOffset, qMax(0, rows - 1));
    const int end = qMin(rows, first + (visibleDataRows > 0 ? visibleDataRows : FallbackSampleRows));

    // Keep space for the sort indicator, so a sort does not cut the header
    int width = m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString().length()
                + (m_sortingEnabled ? 2 : 0);
    for (int r = first; r < end; ++r)
        width = qMax(width, int(m_model->data(m_model->index(logicalRow(r), column), Qt::DisplayRole).toString().length()));
    return qMax(1, width);
}

void QTuiTableView::resizeColumnToContents(int column)
{
    if (m_model && column >= 0 && column < m_model->columnCount())
        m_header->resizeSection(column, measureColumn(column));
}

void QTuiTableView::resizeColumnsToContents()
{
    if (!m_model)
        return;
    for (int c = 0; c < m_model->columnCount(); ++c)
        resizeColumnToContents(c);
}

const QVector<int> &QTuiTableView::effectiveColumnWidths() const
{
    const QRect geo = geometry();

    // The cache is keyed on the viewport: a ResizeToContents column depends
    // on the vertical scroll offset, a Stretch column depends on the view
    // width.
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
    if (cols == 0)
        return m_cachedColumnWidths;

    // The view measures the natural widths one time, when the model has its
    // first rows. After that, a scroll or a change of the data does not
    // change them. Before the first rows, each paint measures the headers.
    const bool measureNow = m_naturalWidths.size() != cols && m_model->rowCount() > 0;
    if (measureNow)
        m_naturalWidths.resize(cols);

    // Step 1: The width per column from its resize mode. A Fixed column and
    // a resized Interactive column have the size of the header section. A
    // ResizeToContents column follows the visible cells. The other columns
    // have the stored natural width.
    QVector<int> colWidths(cols, 0);
    for (int c = 0; c < cols; ++c) {
        const QTuiHeaderView::ResizeMode mode = m_header->sectionResizeMode(c);
        const bool measured = m_naturalWidths.size() == cols && !measureNow;
        if (mode == QTuiHeaderView::ResizeToContents || !measured) {
            colWidths[c] = measureColumn(c);
            if (measureNow)
                m_naturalWidths[c] = colWidths[c];
        } else {
            colWidths[c] = m_naturalWidths.at(c);
        }
        if (mode == QTuiHeaderView::Fixed
            || (mode == QTuiHeaderView::Interactive && m_header->isSectionResized(c)))
            colWidths[c] = qMax(1, m_header->sectionSize(c));
    }

    // Step 2: The Stretch columns (and the last column with
    // stretchLastSection) share the free width. Columns that do not fit
    // keep their widths: the view scrolls by column.
    int shown = 0;
    int totalNatural = 0;
    for (int c = 0; c < cols; ++c) {
        if (isColumnHidden(c))
            continue;
        ++shown;
        totalNatural += colWidths[c];
    }
    const int availableForCols = geo.width() - qMax(0, shown - 1); // a separator between columns

    if (totalNatural < availableForCols) {
        QVector<int> stretch;
        for (int c = 0; c < cols; ++c) {
            if (isColumnHidden(c))
                continue;
            if (m_header->sectionResizeMode(c) == QTuiHeaderView::Stretch
                || (c == movedColumn(cols, -1) && m_header->stretchLastSection()))
                stretch.append(c);
        }
        const int free = availableForCols - totalNatural;
        for (int i = 0; i < stretch.size(); ++i)
            colWidths[stretch.at(i)] += free / stretch.size() + (i < free % stretch.size() ? 1 : 0);
    }

    m_cachedColumnWidths = colWidths;
    return m_cachedColumnWidths;
}

void QTuiTableView::clampCurrentIndex()
{
    if (shownRows() == 0) {
        m_currentRow = 0;
        m_currentCol = 0;
        return;
    }
    m_currentRow = shownRowNear(m_currentRow);
    m_currentCol = qBound(0, m_currentCol, qMax(0, m_model->columnCount() - 1));
}

QRect QTuiTableView::hoverRect() const
{
    // The header, or only the row under the mouse if it has an item
    const QRect geo = geometry();
    const int line = hoverPos().y() - geo.y();
    if (line >= 0 && line < m_headerHeight)
        return QRect(geo.x(), geo.y(), geo.width(), m_headerHeight);
    const int row = line - m_headerHeight + m_scrollOffset;
    if (row < 0 || row >= shownRows())
        return QRect();
    return QRect(geo.x(), hoverPos().y(), geo.width(), 1);
}

} // namespace QtTUI
