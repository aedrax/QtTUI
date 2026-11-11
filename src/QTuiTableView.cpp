#include <QtTUI/QTuiTableView.h>
#include <climits>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QDebug>

namespace QtTUI {

QTuiTableView::QTuiTableView(QTuiWidget *parent)
    : QTuiAbstractItemView(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // The table gives the headers their place and paints them, so they are
    // not focus stops and do not take the mouse. The vertical header is
    // hidden by default.
    createColumnHeader();
    m_verticalHeader = new QTuiHeaderView(Qt::Vertical, this);
    m_verticalHeader->setFocusPolicy(Qt::NoFocus);
    m_verticalHeader->setPaintedByParent(true);
    m_verticalHeader->setVisible(false);
    const auto headerChanged = [this]() {
        invalidateColumnWidths();
        clampScrollOffset(geometry().height() - headerHeight());
        ensureCurrentVisible();
        ensureColumnVisible(m_currentCol);
        updateEditorGeometry();
        markDirty();
    };
    connect(m_header, &QTuiWidget::visibilityChanged, this, headerChanged);
    connect(m_verticalHeader, &QTuiWidget::visibilityChanged, this, headerChanged);
    connect(m_header, &QTuiHeaderView::sectionResized, this, headerChanged);
    connect(m_header, &QTuiHeaderView::geometriesChanged, this, headerChanged);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp scrolling whenever our geometry
    // changes so the current cell stays visible after a shrink or grow.
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        if (!m_model)
            return;
        clampScrollOffset(geometry().height() - headerHeight());
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
    resetRootIndex();
    remeasureColumnWidths();

    if (m_model)
        connectModel();
    m_header->setModel(m_model);
    m_verticalHeader->setModel(m_model);
    resetSelectionModel();
}

QModelIndex QTuiTableView::currentIndex() const
{
    if (shownRows() == 0 || m_model->columnCount(rootIndex()) == 0)
        return QModelIndex();
    return m_model->index(m_currentRow, m_currentCol, rootIndex());
}

void QTuiTableView::setCurrentIndex(const QModelIndex &index)
{
    if (shownRows() == 0 || (index.isValid() && index.parent() != rootIndex()))
        return;
    // A hidden row cannot be current: the next row that shows is
    const int row = shownRowNear(index.row());
    int col = qBound(0, index.column(), qMax(0, m_model->columnCount(rootIndex()) - 1));
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
    if (!m_model || index.model() != m_model || index.parent() != rootIndex())
        return;
    ensureColumnVisible(index.column());
    if (!isHiddenRow(index.row()))
        scrollToRow(visualRow(index.row()), geometry().height() - headerHeight(), shownRows(), hint);
}

QRect QTuiTableView::visualRect(const QModelIndex &index) const
{
    if (!m_model || index.model() != m_model || index.parent() != rootIndex())
        return QRect();
    const QRect geo = geometry();
    const int line = visualRow(index.row()) - m_scrollOffset;
    if (line < 0 || line >= geo.height() - headerHeight() || isHiddenRow(index.row()))
        return QRect();
    // The same column positions as paintEvent
    for (const ColumnSpan &span : visibleColumns()) {
        if (span.column == index.column())
            return QRect(span.x, geo.y() + headerHeight() + line, span.width, 1);
    }
    return QRect();
}

void QTuiTableView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0 || !m_model)
        return;

    const auto theme = themeProperties();
    const int cols = m_model->columnCount(rootIndex());
    const int rows = shownRows();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();
    // The columns start after the vertical header and its separator
    const int left = columnsLeft();
    m_paintedColumnsLeft = left;

    if (cols == 0)
        return;

    // The column positions, shared with mouse hit testing and
    // horizontal scrolling
    const QList<ColumnSpan> spans = visibleColumns();

    // The separators between the columns and the rest of each row use the
    // plain theme colors. The vertical header is bold, as the header row.
    const Cell plain = cellStyle(theme.foreground, theme.background);
    Cell headerStyle = plain;
    headerStyle.bold = true;
    paintHeaderRow(buffer, theme);

    // --- Handle empty model (0 rows): fill remaining with theme background ---
    int visibleDataRows = viewHeight - headerHeight();
    if (rows == 0) {
        fillRect(buffer, QRect(geo.x(), geo.y() + headerHeight(), viewWidth, visibleDataRows), plain);
        return;
    }

    // --- Render visible data rows ---
    for (int vr = 0; vr < visibleDataRows; ++vr) {
        const bool hasRow = m_scrollOffset + vr < rows;
        const int modelRow = hasRow ? logicalRow(m_scrollOffset + vr) : -1;
        int screenY = geo.y() + headerHeight() + vr;
        // The separators and the rest of an alternate row have its shade
        Cell rowStyle = plain;
        if (hasRow && AnsiRenderer::colorMode() != AnsiRenderer::NoColor
            && isAlternateRow(m_model->index(modelRow, 0, rootIndex())))
            rowStyle.background = alternateBackground(theme);
        fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1), rowStyle);
        if (left > 0 && hasRow)
            m_verticalHeader->paintSection(buffer, QRect(geo.x(), screenY, left - 1, 1), modelRow,
                                           headerStyle);

        for (const ColumnSpan &span : spans) {
            const int c = span.column;
            QModelIndex idx;
            if (hasRow)
                idx = m_model->index(modelRow, c, rootIndex());

            bool isCurrent = (modelRow == m_currentRow && c == m_currentCol);
            const Cell style = itemStyle(idx, isCurrent, theme);

            // The space after the text has the item colors, without bold or inverse
            Cell padStyle = style;
            padStyle.bold = false;
            padStyle.inverse = false;
            const QRect cell(span.x, screenY, span.width, 1);
            fillRect(buffer, cell, padStyle);
            paintItem(buffer, cell, idx, style);

            // The separator between two selected cells has the selection
            // color, so a selected row is one bar
            const int next = movedColumn(c, 1);
            const int separatorX = span.x + span.width;
            if (next != c && separatorX < geo.x() + viewWidth && isSelected(idx)
                && isSelected(m_model->index(modelRow, next, rootIndex())))
                fillRect(buffer, QRect(separatorX, screenY, 1, 1), cellStyle(theme.foreground, style.background));
        }
    }
}

void QTuiTableView::keyPressEvent(TuiKeyEvent *event)
{
    if (shownRows() == 0 || m_model->columnCount(rootIndex()) == 0)
        return;

    if (editKeyPressEvent(event) || selectionKeyPressEvent(event))
        return;

    // Alt+Left and Alt+Right make the current column narrower or wider,
    // as a drag on the header does. Alt+Up and Alt+Down sort by it, as a
    // header click does.
    if (resizeColumnKeyEvent(event, m_currentCol))
        return;
    if (event->modifiers() == Qt::AltModifier
        && (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
        if (m_sortingEnabled)
            sortByColumn(m_currentCol, event->key() == Qt::Key_Up ? Qt::AscendingOrder
                                                                  : Qt::DescendingOrder);
        return;
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
                                               geometry().height() - headerHeight()));
            ensureCurrentVisible();
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit activated(m_model->index(m_currentRow, m_currentCol, rootIndex()));
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
    m_header->setSortIndicatorShown(enable);
    // The natural widths keep space for the sort indicator
    remeasureColumnWidths();
    markDirty();
}

void QTuiTableView::sortByColumn(int column, Qt::SortOrder order)
{
    m_header->setSortIndicator(column, order);
    if (m_model && column >= 0)
        m_model->sort(column, order); // the model emits layoutChanged()
    invalidateColumnWidths();
    markDirty();
}

int QTuiTableView::sortColumn() const
{
    return m_header->sortIndicatorSection();
}

Qt::SortOrder QTuiTableView::sortOrder() const
{
    return m_header->sortIndicatorOrder();
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
        setCurrentIndex(m_model->index(m_currentRow, col, rootIndex()));
    }
    invalidateColumnWidths();
    ensureColumnVisible(m_currentCol);
    updateEditorGeometry();
    markDirty();
}

void QTuiTableView::setRootIndex(const QModelIndex &index)
{
    QTuiAbstractItemView::setRootIndex(index);
    m_header->setRootIndex(rootIndex());
    m_verticalHeader->setRootIndex(rootIndex());
    m_currentRow = 0;
    m_currentCol = qMax(0, movedColumn(-1, 1));
    m_horizontalScrollOffset = 0;
    remeasureColumnWidths();
    selectCurrent(Qt::NoModifier, false);
}

bool QTuiTableView::isRowHidden(int row) const
{
    return isHiddenRow(row);
}

void QTuiTableView::setRowHidden(int row, bool hide)
{
    if (!m_model || row < 0 || row >= m_model->rowCount(rootIndex()) || !setHiddenRow(row, hide))
        return;
    // The current cell must be in a row that shows
    if (hide && row == m_currentRow && shownRows() > 0)
        setCurrentIndex(m_model->index(row, m_currentCol, rootIndex()));
    clampScrollOffset(geometry().height() - headerHeight());
    ensureCurrentVisible();
    invalidateColumnWidths();
    updateEditorGeometry();
    markRowsDirty(visualRow(row), INT_MAX, headerHeight());
}

QTuiHeaderView *QTuiTableView::horizontalHeader() const
{
    return m_header;
}

QTuiHeaderView *QTuiTableView::verticalHeader() const
{
    return m_verticalHeader;
}

int QTuiTableView::columnsLeft() const
{
    if (!m_verticalHeader->isVisible() || !m_model)
        return 0;
    // The width does not change when the view scrolls: the view measures
    // the labels of the rows that show one time, and keeps space for the
    // number of the last row.
    if (m_verticalLabelWidth < 0 && shownRows() > 0) {
        static constexpr int FallbackSampleRows = 100;
        const int visibleRows = geometry().height() - headerHeight();
        const int end = qMin(shownRows(), m_scrollOffset + (visibleRows > 0 ? visibleRows : FallbackSampleRows));
        m_verticalLabelWidth = 1;
        for (int r = m_scrollOffset; r < end; ++r) {
            const QString label = m_model->headerData(logicalRow(r), Qt::Vertical, Qt::DisplayRole).toString();
            m_verticalLabelWidth = qMax(m_verticalLabelWidth, QtTuiUnicode::stringWidth(label));
        }
    }
    const int digits = QString::number(m_model->rowCount(rootIndex())).size();
    // One more cell separates the header from the first column
    return qMax(m_verticalLabelWidth, digits) + 1;
}

void QTuiTableView::updateColumnsLeft()
{
    if (columnsLeft() != m_paintedColumnsLeft)
        markDirty();
}

void QTuiTableView::mousePressEvent(TuiMouseEvent *event)
{
    // Shift+wheel scrolls one column per notch. Some terminals keep
    // Shift+wheel for their own scrollback.
    if (horizontalWheelEvent(event))
        return;
    if (m_model && wheelScroll(event, &m_scrollOffset, geometry().height() - headerHeight(),
                               shownRows()))
        return;
    // The Press before the DoubleClick already made the item current
    if (event->mouseType() == TuiMouseEvent::DoubleClick) {
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return;
        emit doubleClicked(index);
        // No editor for a double click on the header
        const bool onHeader = event->y() - geometry().y() < headerHeight();
        if (!onHeader && (editTriggers() & DoubleClicked) && edit(index))
            return;
        emit activated(index);
        return;
    }
    if (headerResizeMouseEvent(event))
        return;
    // A left drag selects a range, as Shift+click does (see QTuiListView)
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
        return;
    if (shownRows() == 0 || m_model->columnCount(rootIndex()) == 0)
        return;

    const QRect geo = geometry();

    // Calculate clicked row (subtract 1 for header)
    const bool onHeader = event->y() - geo.y() < headerHeight();
    const int clickedLine = (event->y() - geo.y() - headerHeight()) + m_scrollOffset;
    if (drag && onHeader)
        return;
    if (!onHeader && (clickedLine < 0 || clickedLine >= shownRows()))
        return;
    const int clickedRow = logicalRow(clickedLine);

    // The same column positions as paintEvent. A click after the last
    // column goes to the last column that shows. A press on the vertical
    // header does nothing.
    const int clickedCol = columnAt(event->x());
    if (clickedCol < 0)
        return;

    // A header click sorts: a new column ascending, the same column reversed
    if (onHeader) {
        if (m_sortingEnabled) {
            const Qt::SortOrder order = (clickedCol == sortColumn() && sortOrder() == Qt::AscendingOrder)
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
    if (event->button() == Qt::LeftButton && !drag) {
        checkBoxPressEvent(event, currentIndex());
        emit clicked(m_model->index(clickedRow, clickedCol, rootIndex()));
    }
}

void QTuiTableView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model || topLeft.parent() != rootIndex())
        return;

    invalidateColumnWidths();
    const int lastRow = m_model->rowCount(rootIndex()) - 1;
    markRowsDirty(visualRow(qBound(0, topLeft.row(), lastRow)),
                  visualRow(qBound(0, bottomRight.row(), lastRow)), headerHeight());
}

void QTuiTableView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    if (!m_model || parent != rootIndex())
        return;

    shiftHiddenRows(first, last - first + 1);
    invalidateColumnWidths();
    updateColumnsLeft();
    // Mark from the insertion point to the bottom of the view
    markRowsDirty(visualRow(first), INT_MAX, headerHeight());
    clampCurrentIndex();
}

void QTuiTableView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    if (!m_model || parent != rootIndex())
        return;

    shiftHiddenRows(first, first - last - 1);
    invalidateColumnWidths();
    updateColumnsLeft();
    // Mark from the removal point to the bottom of the view
    markRowsDirty(visualRow(first), INT_MAX, headerHeight());
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
                                      qMax(0, m_model->columnCount(rootIndex()) - 1));
    ensureColumnVisible(m_currentCol);
    markDirty();
}

void QTuiTableView::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_UNUSED(first);
    Q_UNUSED(last);

    // The labels of the rows can have a different width
    if (orientation == Qt::Vertical) {
        m_verticalLabelWidth = -1;
        invalidateColumnWidths();
        if (m_verticalHeader->isVisible())
            markDirty();
        return;
    }

    // Header text feeds the column width calculation
    invalidateColumnWidths();

    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;
    markDirty(QRect(geo.x(), geo.y(), geo.width(), headerHeight()));
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

    scrollRowIntoView(visualRow(m_currentRow), geometry().height() - headerHeight());
}

void QTuiTableView::remeasureColumnWidths()
{
    m_verticalLabelWidth = -1;
    QTuiAbstractItemView::remeasureColumnWidths();
}

int QTuiTableView::measureColumn(int column) const
{
    // Sample only the rows in the viewport, not the full model. If the
    // geometry has no data rows yet, sample a bounded number of rows from
    // the top of the model.
    static constexpr int FallbackSampleRows = 100;
    const int rows = shownRows();
    const int visibleDataRows = geometry().height() - headerHeight();
    const int first = qBound(0, m_scrollOffset, qMax(0, rows - 1));
    const int end = qMin(rows, first + (visibleDataRows > 0 ? visibleDataRows : FallbackSampleRows));

    // Keep space for the sort indicator, so a sort does not cut the header
    int width = m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString().length()
                + (m_sortingEnabled ? 2 : 0);
    for (int r = first; r < end; ++r) {
        const QModelIndex index = m_model->index(logicalRow(r), column, rootIndex());
        width = qMax(width, itemDelegateForIndex(index)->sizeHint(index).width());
    }
    return qMax(1, width);
}

void QTuiTableView::clampCurrentIndex()
{
    if (shownRows() == 0) {
        m_currentRow = 0;
        m_currentCol = 0;
        return;
    }
    m_currentRow = shownRowNear(m_currentRow);
    m_currentCol = qBound(0, m_currentCol, qMax(0, m_model->columnCount(rootIndex()) - 1));
}

QRect QTuiTableView::hoverRect() const
{
    // The header, or only the row under the mouse if it has an item
    const QRect geo = geometry();
    const int line = hoverPos().y() - geo.y();
    if (line >= 0 && line < headerHeight())
        return QRect(geo.x(), geo.y(), geo.width(), headerHeight());
    const int row = line - headerHeight() + m_scrollOffset;
    if (row < 0 || row >= shownRows())
        return QRect();
    return QRect(geo.x(), hoverPos().y(), geo.width(), 1);
}

} // namespace QtTUI
