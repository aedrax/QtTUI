#include <QtTUI/QTuiTreeView.h>
#include <climits>
#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/AnsiRenderer.h>

namespace QtTUI {

QTuiTreeView::QTuiTreeView(QTuiWidget *parent)
    : QTuiAbstractItemView(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Different from Qt: the header row is hidden by default. As in Qt,
    // the last column gets the free width, so a tree with one column uses
    // the full width.
    createColumnHeader();
    m_header->setVisible(false);
    m_header->setStretchLastSection(true);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp scrolling whenever our geometry
    // changes so the current row stays visible after a shrink or grow.
    const auto clampScroll = [this]() {
        if (!m_model)
            return;
        if (viewRows() > 0)
            m_scrollOffset = qBound(0, m_scrollOffset, qMax(0, int(m_flatList.size()) - viewRows()));
        scrollToVisible();
        ensureColumnVisible(m_currentCol);
        markDirty();
    };
    connect(this, &QTuiWidget::geometryChanged, this, clampScroll);
    const auto headerChanged = [this, clampScroll]() {
        invalidateColumnWidths();
        clampScroll();
        updateEditorGeometry();
        markDirty();
    };
    connect(m_header, &QTuiWidget::visibilityChanged, this, headerChanged);
    connect(m_header, &QTuiHeaderView::sectionResized, this, headerChanged);
    connect(m_header, &QTuiHeaderView::geometriesChanged, this, headerChanged);
}

void QTuiTreeView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_currentRow = 0;
    m_currentCol = 0;
    m_scrollOffset = 0;
    m_horizontalScrollOffset = 0;
    remeasureColumnWidths();
    m_pendingRemovalFlatRow = -1;
    m_expandedItems.clear();
    m_hiddenItems.clear();
    m_flatList.clear();
    resetRootIndex();

    if (m_model) {
        connectModel();
        rebuildFlatList();
    }
    m_header->setModel(m_model);
    resetSelectionModel();
}

QModelIndex QTuiTreeView::currentIndex() const
{
    if (m_flatList.isEmpty() || m_currentRow < 0 || m_currentRow >= m_flatList.size())
        return QModelIndex();
    return m_flatList.at(m_currentRow).index.siblingAtColumn(m_currentCol);
}

void QTuiTreeView::setCurrentIndex(const QModelIndex &index)
{
    int row = flatRowForIndex(index);
    if (row < 0)
        return;
    // A hidden column cannot be current
    const int col = isColumnHidden(index.column()) ? m_currentCol : index.column();
    if (row != m_currentRow || col != m_currentCol) {
        m_currentRow = row;
        m_currentCol = col;
        scrollToVisible();
        ensureColumnVisible(m_currentCol);
        markDirty();
    }
    selectCurrent(Qt::NoModifier, false);
}

void QTuiTreeView::expand(const QModelIndex &anyColumn)
{
    // The items of column 0 have the children
    const QModelIndex index = anyColumn.siblingAtColumn(0);
    if (!m_model || !index.isValid())
        return;

    if (!m_model->hasChildren(index))
        return;

    QPersistentModelIndex persistent(index);
    if (m_expandedItems.contains(persistent))
        return;

    m_expandedItems.insert(persistent);

    // Find the flat row of the toggled node before rebuild
    int toggledRow = flatRowForIndex(index);
    const QModelIndex current = currentIndex();

    rebuildFlatList();

    // The current item stays current: find its row after the rebuild
    const int currentRow = flatRowForIndex(current);
    if (currentRow >= 0)
        m_currentRow = currentRow;
    const int row = flatRowForIndex(index);
    scrollToVisible();

    // Mark from toggled row to bottom of visible area as dirty
    markRowsDirty(toggledRow >= 0 ? toggledRow : 0, INT_MAX, headerHeight());
    // The children have an indent and can be wider than the rows that the
    // view measured: the columns get wider for one page of them, never
    // narrower. A scroll does not change the widths.
    for (int c = 0; row >= 0 && c < m_model->columnCount(rootIndex()); ++c) {
        if (!isColumnHidden(c))
            growColumnWidth(c, measureRows(c, row + 1, viewRows()));
    }

    emit expanded(index);
}

void QTuiTreeView::collapse(const QModelIndex &anyColumn)
{
    const QModelIndex index = anyColumn.siblingAtColumn(0);
    if (!m_model || !index.isValid())
        return;

    QPersistentModelIndex persistent(index);
    if (!m_expandedItems.contains(persistent))
        return;

    // Find the flat row of the toggled node before rebuild
    int toggledRow = flatRowForIndex(index);
    const QModelIndex current = currentIndex();

    m_expandedItems.remove(persistent);
    rebuildFlatList();
    // The current item stays current. If it was below the collapsed item,
    // that item becomes current, as in Qt.
    int row = flatRowForIndex(current);
    if (row < 0)
        row = flatRowForIndex(index);
    if (row >= 0)
        m_currentRow = row;
    // Clamp current row if it's now beyond the list
    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();

    // Mark from toggled row to bottom of visible area as dirty
    markRowsDirty(toggledRow >= 0 ? toggledRow : 0, INT_MAX, headerHeight());
    if (currentIndex() != current) {
        selectCurrent(Qt::NoModifier, false);
        markDirty();
    }

    emit collapsed(index);
}

void QTuiTreeView::setRootIndex(const QModelIndex &index)
{
    QTuiAbstractItemView::setRootIndex(index);
    m_header->setRootIndex(rootIndex());
    m_currentRow = 0;
    m_currentCol = 0;
    m_horizontalScrollOffset = 0;
    remeasureColumnWidths();
    rebuildFlatList();
    selectCurrent(Qt::NoModifier, false);
}

bool QTuiTreeView::isRowHidden(int row, const QModelIndex &parent) const
{
    return m_model && m_hiddenItems.contains(QPersistentModelIndex(m_model->index(row, 0, parent)));
}

void QTuiTreeView::setRowHidden(int row, const QModelIndex &parent, bool hide)
{
    const QModelIndex index = m_model ? m_model->index(row, 0, parent) : QModelIndex();
    if (!index.isValid() || hide == isRowHidden(row, parent))
        return;
    if (hide)
        m_hiddenItems.insert(QPersistentModelIndex(index));
    else
        m_hiddenItems.remove(QPersistentModelIndex(index));

    // Keep the current item. If it is now hidden, the item at its row
    // becomes current.
    const QModelIndex current = currentIndex();
    rebuildFlatList();
    const int currentRow = flatRowForIndex(current);
    m_currentRow = currentRow >= 0 ? currentRow
                                   : qBound(0, m_currentRow, qMax(0, int(m_flatList.size()) - 1));
    m_scrollOffset = qBound(0, m_scrollOffset, qMax(0, int(m_flatList.size()) - viewRows()));
    scrollToVisible();
    if (currentRow < 0)
        selectCurrent(Qt::NoModifier, false);
    markDirty();
}

bool QTuiTreeView::isExpanded(const QModelIndex &index) const
{
    return m_expandedItems.contains(QPersistentModelIndex(index.siblingAtColumn(0)));
}

void QTuiTreeView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (!m_model || index.model() != m_model)
        return;
    // Expand the parents, so that the item has a row
    QList<QModelIndex> parents;
    for (QModelIndex p = index.parent(); p.isValid() && p != rootIndex(); p = p.parent())
        parents.prepend(p);
    for (const QModelIndex &p : parents)
        expand(p);
    ensureColumnVisible(index.column());
    scrollToRow(flatRowForIndex(index), viewRows(), m_flatList.size(), hint);
}

QRect QTuiTreeView::visualRect(const QModelIndex &index) const
{
    const int row = flatRowForIndex(index);
    const QRect geo = geometry();
    const int line = row - m_scrollOffset;
    if (row < 0 || line < 0 || line >= viewRows())
        return QRect();
    for (const ColumnSpan &span : visibleColumns()) {
        if (span.column != index.column())
            continue;
        // In column 0, the text starts after the indentation and the 2
        // cells of the indicator
        const int x = span.column == 0 ? m_flatList.at(row).depth * m_indentWidth + 2 : 0;
        if (x >= span.width)
            return QRect();
        return QRect(span.x + x, geo.y() + headerHeight() + line, span.width - x, 1);
    }
    return QRect();
}

void QTuiTreeView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    // Clear background first
    clearBackground(buffer, geo);

    if (!m_model)
        return;

    const auto theme = themeProperties();
    paintHeaderRow(buffer, theme);
    // The column positions, shared with the mouse hit test
    const QList<ColumnSpan> spans = visibleColumns();
    const int viewRight = geo.right() + 1;
    const int rows = viewRows();

    for (int vr = 0; vr < rows; ++vr) {
        int flatRow = m_scrollOffset + vr;
        int screenY = geo.y() + headerHeight() + vr;

        if (flatRow >= m_flatList.size())
            break;

        const FlattenedItem &item = m_flatList.at(flatRow);
        // The separators and the rest of an alternate row have its shade
        if (AnsiRenderer::colorMode() != AnsiRenderer::NoColor && isAlternateRow(item.index))
            fillRect(buffer, QRect(geo.x(), screenY, geo.width(), 1),
                     cellStyle(theme.foreground, alternateBackground(theme)));

        for (const ColumnSpan &span : spans) {
            const QModelIndex index = item.index.siblingAtColumn(span.column);
            const bool isCurrent = flatRow == m_currentRow && span.column == m_currentCol;
            const Cell style = itemStyle(index, isCurrent, theme);
            QRect cell(span.x, screenY, span.width, 1);

            // Fill the cell with the item style first (for highlighting)
            if (style.background != theme.background || style.underline)
                fillRect(buffer, cell, style);

            if (span.column == 0) {
                // The indentation and the expand/collapse indicator use the
                // item colors without bold or inverse. The indicator is ▶
                // (collapsed) or ▼ (expanded) and a space; a leaf gets two
                // spaces for alignment.
                Cell plain = style;
                plain.bold = false;
                plain.inverse = false;
                const int indentOffset = item.depth * m_indentWidth;
                const int indicatorOffset = indentOffset + 2;
                fillRect(buffer, QRect(span.x, screenY, qMin(indicatorOffset, span.width), 1), plain);
                if (m_model->hasChildren(item.index))
                    renderText(buffer, span.x + indentOffset, screenY,
                               item.expanded ? QStringLiteral("▼") : QStringLiteral("▶"),
                               span.width - indentOffset, plain);
                cell.setLeft(span.x + indicatorOffset);
            }

            // The delegate writes the item in the remaining area
            paintItem(buffer, cell, index, style);

            // The separator between two selected cells has the selection
            // color, so a selected row is one bar
            const int next = movedColumn(span.column, 1);
            const int separatorX = span.x + span.width;
            if (next != span.column && separatorX < viewRight && isSelected(index)
                && isSelected(item.index.siblingAtColumn(next)))
                fillRect(buffer, QRect(separatorX, screenY, 1, 1), cellStyle(theme.foreground, style.background));
        }
    }
}

void QTuiTreeView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_model || m_flatList.isEmpty())
        return;
    if (editKeyPressEvent(event) || selectionKeyPressEvent(event))
        return;
    // Alt+Left and Alt+Right make the current column narrower or wider
    if (resizeColumnKeyEvent(event, m_currentCol))
        return;
    const QModelIndex previous = currentIndex();
    CursorAction action;
    Qt::KeyboardModifiers modifiers = event->modifiers();
    int key = event->key();
    if (cursorAction(event, &action, &modifiers)) {
        if (action == MoveLeft || action == MoveRight) {
            key = action == MoveLeft ? Qt::Key_Left : Qt::Key_Right;
        } else {
            m_currentRow = movedRow(action, m_currentRow, m_flatList.size(), viewRows());
            scrollToVisible();
            markDirty();
            key = 0;
        }
    }

    switch (key) {
    case Qt::Key_Right: {
        const FlattenedItem &item = m_flatList.at(m_currentRow);
        if (m_model->hasChildren(item.index)) {
            if (!item.expanded) {
                expand(item.index);
            } else if (m_currentRow < m_flatList.size() - 1) {
                // Already expanded: move to first child
                ++m_currentRow;
                scrollToVisible();
                markDirty();
            }
        } else {
            // An item without children: move to the next column, as in Qt
            m_currentCol = movedColumn(m_currentCol, 1);
            ensureColumnVisible(m_currentCol);
            markDirty();
        }
        break;
    }

    case Qt::Key_Left: {
        const FlattenedItem &item = m_flatList.at(m_currentRow);
        const int column = movedColumn(m_currentCol, -1);
        if (column != m_currentCol) {
            // Not in the first column: move to the column before
            m_currentCol = column;
            ensureColumnVisible(m_currentCol);
            markDirty();
        } else if (item.expanded && m_model->hasChildren(item.index)) {
            collapse(item.index);
        } else if (item.depth > 0) {
            // Move to parent
            QModelIndex parentIdx = item.index.parent();
            int parentRow = flatRowForIndex(parentIdx);
            if (parentRow >= 0) {
                m_currentRow = parentRow;
                scrollToVisible();
                markDirty();
            }
        }
        break;
    }

    case Qt::Key_Return:
    case Qt::Key_Enter: {
        const FlattenedItem &item = m_flatList.at(m_currentRow);
        if (m_model->hasChildren(item.index)) {
            // Toggle expand/collapse
            if (item.expanded)
                collapse(item.index);
            else
                expand(item.index);
        }
        emit activated(item.index);
        break;
    }

    default:
        break;
    }

    if (currentIndex() != previous)
        selectCurrent(modifiers, false);
}

void QTuiTreeView::mousePressEvent(TuiMouseEvent *event)
{
    // Shift+wheel scrolls one column per notch
    if (horizontalWheelEvent(event))
        return;
    if (wheelScroll(event, &m_scrollOffset, viewRows(), m_flatList.size()))
        return;
    const bool onHeader = event->y() - geometry().y() < headerHeight();
    // The Press before the DoubleClick already made the item current
    if (event->mouseType() == TuiMouseEvent::DoubleClick) {
        const QModelIndex index = currentIndex();
        if (!index.isValid() || onHeader)
            return;
        emit doubleClicked(index);
        if ((editTriggers() & DoubleClicked) && edit(index))
            return;
        // A double click on an item with children expands or collapses it
        if (m_model && m_model->hasChildren(index.siblingAtColumn(0)))
            isExpanded(index) ? collapse(index) : expand(index);
        emit activated(index);
        return;
    }
    // A left drag on the separator after a header resizes that column
    if (headerResizeMouseEvent(event))
        return;
    // A left drag selects a range, as Shift+click does (see QTuiListView)
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
        return;
    if (!m_model || m_flatList.isEmpty() || onHeader)
        return;

    const QRect geo = geometry();
    int clickedRow = (event->y() - geo.y() - headerHeight()) + m_scrollOffset;
    const int clickedCol = columnAt(event->x());
    if (clickedRow >= 0 && clickedRow < m_flatList.size() && clickedCol >= 0) {
        m_currentRow = clickedRow;
        m_currentCol = clickedCol;
        clickCurrent(event, drag);
    }
}

bool QTuiTreeView::isAlternateRow(const QModelIndex &index) const
{
    if (!alternatingRowColors())
        return false;
    // ponytail: a search in the rows that show for each item, so a paint
    // costs the square of the view height. Give the row to itemStyle() if
    // a view is high enough to see it.
    const QModelIndex first = index.siblingAtColumn(0);
    const int end = qMin(int(m_flatList.size()), m_scrollOffset + viewRows());
    for (int row = qMax(0, m_scrollOffset); row < end; ++row) {
        if (m_flatList.at(row).index == first)
            return row & 1;
    }
    return false;
}

QItemSelection QTuiTreeView::selectionRange(const QModelIndex &from, const QModelIndex &to) const
{
    // The visible rows between the two items, at any depth
    const int a = flatRowForIndex(from);
    const int b = flatRowForIndex(to);
    if (a < 0 || b < 0)
        return QItemSelection(to, to);
    const int left = qMin(from.column(), to.column());
    const int right = qMax(from.column(), to.column());
    QItemSelection range;
    for (int row = qMin(a, b); row <= qMax(a, b); ++row) {
        const QModelIndex index = m_flatList.at(row).index;
        range.select(index.siblingAtColumn(left), index.siblingAtColumn(right));
    }
    return range;
}

void QTuiTreeView::selectAll()
{
    if (!selectionModel() || m_flatList.isEmpty() || selectionMode() < MultiSelection)
        return;
    const int lastColumn = qMax(0, m_model->columnCount(rootIndex()) - 1);
    selectionModel()->select(selectionRange(m_flatList.first().index,
                                            m_flatList.last().index.siblingAtColumn(lastColumn)),
                             QItemSelectionModel::ClearAndSelect);
}

// --- Model signal slots ---

void QTuiTreeView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    // Find the visible flat rows of the changed indexes
    invalidateColumnWidths();
    const int lastVisibleRow = qMin(int(m_flatList.size()), m_scrollOffset + viewRows()) - 1;
    int dirtyTop = -1;
    int dirtyBottom = -1;
    for (int i = m_scrollOffset; i <= lastVisibleRow; ++i) {
        const QModelIndex &idx = m_flatList.at(i).index;
        if (idx.parent() == topLeft.parent() &&
            idx.row() >= topLeft.row() && idx.row() <= bottomRight.row()) {
            if (dirtyTop < 0)
                dirtyTop = i;
            dirtyBottom = i;
        }
    }
    if (dirtyTop >= 0)
        markRowsDirty(dirtyTop, dirtyBottom, headerHeight());
}

void QTuiTreeView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    QModelIndex currentIdx;
    if (m_currentRow >= 0 && m_currentRow < m_flatList.size())
        currentIdx = m_flatList.at(m_currentRow).index;

    // Find the flat row corresponding to the insertion point before rebuild
    int affectedFlatRow = -1;
    for (int i = 0; i < m_flatList.size(); ++i) {
        const QModelIndex &idx = m_flatList.at(i).index;
        if (idx.parent() == parent && idx.row() >= first) {
            affectedFlatRow = i;
            break;
        }
    }

    rebuildFlatList();

    if (currentIdx.isValid()) {
        int row = flatRowForIndex(currentIdx);
        if (row >= 0)
            m_currentRow = row;
    }
    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();

    // Mark from the affected row to the bottom of the view
    markRowsDirty(qMax(0, affectedFlatRow), INT_MAX, headerHeight());
}

void QTuiTreeView::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(last);

    // Find the flat row corresponding to the removal point while the flat
    // list's indexes (and their internal pointers) are still valid. Doing
    // this in onRowsRemoved would call parent() on indexes whose internal
    // pointers may already have been freed by the model.
    m_pendingRemovalFlatRow = -1;
    for (int i = 0; i < m_flatList.size(); ++i) {
        const QModelIndex &idx = m_flatList.at(i).index;
        if (idx.parent() == parent && idx.row() >= first) {
            m_pendingRemovalFlatRow = i;
            break;
        }
    }
}

void QTuiTreeView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    // Captured before the removal, while indexes were still valid
    int affectedFlatRow = m_pendingRemovalFlatRow;
    m_pendingRemovalFlatRow = -1;

    // Rebuild directly from the model; the old flat list is stale
    rebuildFlatList();

    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();

    // Mark from the affected row to the bottom of the view
    markRowsDirty(qMax(0, affectedFlatRow), INT_MAX, headerHeight());
}

void QTuiTreeView::onColumnsChanged()
{
    if (!m_model)
        return;
    remeasureColumnWidths();
    const int lastColumn = qMax(0, m_model->columnCount(rootIndex()) - 1);
    m_currentCol = qBound(0, m_currentCol, lastColumn);
    m_horizontalScrollOffset = qBound(0, m_horizontalScrollOffset, lastColumn);
    ensureColumnVisible(m_currentCol);
    markDirty();
}

void QTuiTreeView::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_UNUSED(first);
    Q_UNUSED(last);
    if (orientation != Qt::Horizontal)
        return;
    invalidateColumnWidths();
    const QRect geo = geometry();
    if (headerHeight() > 0 && !geo.isEmpty())
        markDirty(QRect(geo.x(), geo.y(), geo.width(), headerHeight()));
}

void QTuiTreeView::onLayoutChanged()
{
    // Rows may have been reordered wholesale (e.g. proxy sort). Try to keep
    // the current item; the expanded set uses persistent indexes and updates
    // automatically.
    QModelIndex currentIdx;
    if (m_currentRow >= 0 && m_currentRow < m_flatList.size())
        currentIdx = m_flatList.at(m_currentRow).index;

    rebuildFlatList();
    invalidateColumnWidths();

    int row = flatRowForIndex(currentIdx);
    if (row >= 0)
        m_currentRow = row;
    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();
    markDirty();
}

void QTuiTreeView::onModelReset()
{
    m_currentRow = 0;
    m_currentCol = 0;
    m_scrollOffset = 0;
    m_horizontalScrollOffset = 0;
    remeasureColumnWidths();
    m_expandedItems.clear();
    m_hiddenItems.clear();
    m_flatList.clear();
    rebuildFlatList();
    markDirty();
}

// --- Private helpers ---

void QTuiTreeView::disconnectModel()
{
    if (!m_model)
        return;

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &QTuiTreeView::onDataChanged);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &QTuiTreeView::onRowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
               this, &QTuiTreeView::onRowsAboutToBeRemoved);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &QTuiTreeView::onRowsRemoved);
    disconnect(m_model, &QAbstractItemModel::columnsInserted,
               this, &QTuiTreeView::onColumnsChanged);
    disconnect(m_model, &QAbstractItemModel::columnsRemoved,
               this, &QTuiTreeView::onColumnsChanged);
    disconnect(m_model, &QAbstractItemModel::headerDataChanged,
               this, &QTuiTreeView::onHeaderDataChanged);
    disconnect(m_model, &QAbstractItemModel::layoutChanged,
               this, &QTuiTreeView::onLayoutChanged);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiTreeView::onModelReset);
    disconnect(m_model, &QObject::destroyed, this, nullptr);
}

void QTuiTreeView::connectModel()
{
    if (!m_model)
        return;

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &QTuiTreeView::onDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QTuiTreeView::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &QTuiTreeView::onRowsAboutToBeRemoved);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QTuiTreeView::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::columnsInserted,
            this, &QTuiTreeView::onColumnsChanged);
    connect(m_model, &QAbstractItemModel::columnsRemoved,
            this, &QTuiTreeView::onColumnsChanged);
    connect(m_model, &QAbstractItemModel::headerDataChanged,
            this, &QTuiTreeView::onHeaderDataChanged);
    connect(m_model, &QAbstractItemModel::layoutChanged,
            this, &QTuiTreeView::onLayoutChanged);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiTreeView::onModelReset);
    // Guard against the model being deleted while still set on the view
    connect(m_model, &QObject::destroyed, this, [this]() {
        m_model = nullptr;
        onModelReset();
    });
}

void QTuiTreeView::rebuildFlatList()
{
    m_flatList.clear();
    if (!m_model)
        return;
    buildFlatListRecursive(rootIndex(), 0);
    updateEditorGeometry();
}

void QTuiTreeView::buildFlatListRecursive(const QModelIndex &parent, int depth)
{
    const int rowCount = m_model->rowCount(parent);
    for (int r = 0; r < rowCount; ++r) {
        QModelIndex idx = m_model->index(r, 0, parent);
        if (!m_hiddenItems.isEmpty() && m_hiddenItems.contains(QPersistentModelIndex(idx)))
            continue;
        bool isExpanded = m_expandedItems.contains(QPersistentModelIndex(idx));

        FlattenedItem item;
        item.index = idx;
        item.depth = depth;
        item.expanded = isExpanded;
        m_flatList.append(item);

        if (isExpanded && m_model->hasChildren(idx))
            buildFlatListRecursive(idx, depth + 1);
    }
}

void QTuiTreeView::scrollToVisible()
{
    // A scroll changes all lines
    const int offset = m_scrollOffset;
    scrollRowIntoView(m_currentRow, viewRows());
    if (m_scrollOffset != offset)
        markDirty();
}

int QTuiTreeView::viewRows() const
{
    return geometry().height() - headerHeight();
}

QTuiHeaderView *QTuiTreeView::header() const
{
    return m_header;
}

bool QTuiTreeView::isHeaderHidden() const
{
    return !m_header->isVisible();
}

void QTuiTreeView::setHeaderHidden(bool hide)
{
    m_header->setVisible(!hide);
}

int QTuiTreeView::measureColumn(int column) const
{
    // The header and the items that show
    return qMax(QTuiAbstractItemView::measureColumn(column),
                measureRows(column, m_scrollOffset, viewRows()));
}

int QTuiTreeView::measureRows(int column, int first, int count) const
{
    // If the geometry has no rows yet, a bounded number of items
    static constexpr int FallbackSampleRows = 100;
    first = qMax(0, first);
    const int end = qMin(int(m_flatList.size()), first + (count > 0 ? count : FallbackSampleRows));
    int width = 1;
    for (int r = first; r < end; ++r) {
        const FlattenedItem &item = m_flatList.at(r);
        const QModelIndex index = item.index.siblingAtColumn(column);
        const int indent = column == 0 ? item.depth * m_indentWidth + 2 : 0;
        width = qMax(width, indent + itemDelegateForIndex(index)->sizeHint(index).width());
    }
    return width;
}

int QTuiTreeView::flatRowForIndex(const QModelIndex &index) const
{
    const QModelIndex first = index.siblingAtColumn(0);
    for (int i = 0; i < m_flatList.size(); ++i) {
        if (m_flatList.at(i).index == first)
            return i;
    }
    return -1;
}

QRect QTuiTreeView::hoverRect() const
{
    // The header, or only the row under the mouse if it has an item
    const QRect geo = geometry();
    const int line = hoverPos().y() - geo.y() - headerHeight();
    if (line < 0)
        return QRect(geo.x(), geo.y(), geo.width(), headerHeight());
    const int row = line + m_scrollOffset;
    if (row < 0 || row >= m_flatList.size())
        return QRect();
    return QRect(geo.x(), hoverPos().y(), geo.width(), 1);
}

} // namespace QtTUI
