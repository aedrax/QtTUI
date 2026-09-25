#include <QtTUI/QTuiTreeView.h>
#include <climits>
#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiTreeView::QTuiTreeView(QTuiWidget *parent)
    : QTuiAbstractItemView(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp scrolling whenever our geometry
    // changes so the current row stays visible after a shrink or grow.
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        if (!m_model)
            return;
        const int visibleRows = geometry().height();
        if (visibleRows > 0) {
            m_scrollOffset = qBound(0, m_scrollOffset,
                                    qMax(0, int(m_flatList.size()) - visibleRows));
        }
        scrollToVisible();
        markDirty();
    });
}

void QTuiTreeView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_currentRow = 0;
    m_scrollOffset = 0;
    m_pendingRemovalFlatRow = -1;
    m_expandedItems.clear();
    m_flatList.clear();

    if (m_model) {
        connectModel();
        rebuildFlatList();
    }
    resetSelectionModel();
}

QModelIndex QTuiTreeView::currentIndex() const
{
    if (m_flatList.isEmpty() || m_currentRow < 0 || m_currentRow >= m_flatList.size())
        return QModelIndex();
    return m_flatList.at(m_currentRow).index;
}

void QTuiTreeView::setCurrentIndex(const QModelIndex &index)
{
    int row = flatRowForIndex(index);
    if (row < 0)
        return;
    if (row != m_currentRow) {
        m_currentRow = row;
        scrollToVisible();
        markDirty();
    }
    selectCurrent(Qt::NoModifier, false);
}

void QTuiTreeView::expand(const QModelIndex &index)
{
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

    rebuildFlatList();

    // Restore current row to point to the same index after rebuild
    int row = flatRowForIndex(index);
    if (row >= 0)
        m_currentRow = row;
    scrollToVisible();

    // Mark from toggled row to bottom of visible area as dirty
    markRowsDirty(toggledRow >= 0 ? toggledRow : 0, INT_MAX);

    emit expanded(index);
}

void QTuiTreeView::collapse(const QModelIndex &index)
{
    if (!m_model || !index.isValid())
        return;

    QPersistentModelIndex persistent(index);
    if (!m_expandedItems.contains(persistent))
        return;

    // Find the flat row of the toggled node before rebuild
    int toggledRow = flatRowForIndex(index);

    m_expandedItems.remove(persistent);
    rebuildFlatList();
    int row = flatRowForIndex(index);
    if (row >= 0)
        m_currentRow = row;
    // Clamp current row if it's now beyond the list
    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();

    // Mark from toggled row to bottom of visible area as dirty
    markRowsDirty(toggledRow >= 0 ? toggledRow : 0, INT_MAX);

    emit collapsed(index);
}

bool QTuiTreeView::isExpanded(const QModelIndex &index) const
{
    return m_expandedItems.contains(QPersistentModelIndex(index));
}

void QTuiTreeView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    if (!m_model || index.model() != m_model)
        return;
    // Expand the parents, so that the item has a row
    QList<QModelIndex> parents;
    for (QModelIndex p = index.parent(); p.isValid(); p = p.parent())
        parents.prepend(p);
    for (const QModelIndex &p : parents)
        expand(p);
    scrollToRow(flatRowForIndex(index), geometry().height(), m_flatList.size(), hint);
}

QRect QTuiTreeView::visualRect(const QModelIndex &index) const
{
    const int row = flatRowForIndex(index);
    const QRect geo = geometry();
    const int line = row - m_scrollOffset;
    if (row < 0 || line < 0 || line >= geo.height())
        return QRect();
    // The text starts after the indentation and the 2 cells of the indicator
    const int x = m_flatList.at(row).depth * m_indentWidth + 2;
    if (x >= geo.width())
        return QRect();
    return QRect(geo.x() + x, geo.y() + line, geo.width() - x, 1);
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
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    for (int vr = 0; vr < viewHeight; ++vr) {
        int flatRow = m_scrollOffset + vr;
        int screenY = geo.y() + vr;

        if (flatRow >= m_flatList.size())
            break;

        const FlattenedItem &item = m_flatList.at(flatRow);
        bool isCurrent = (flatRow == m_currentRow);
        bool hasChildren = m_model->hasChildren(item.index);

        const Cell style = itemStyle(item.index, isCurrent, theme);

        // Fill the entire row with the item style first (for highlighting)
        if (style.background != theme.background || style.underline)
            fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1), style);

        // The indentation and the expand/collapse indicator use the item
        // colors without bold or inverse. The indicator is ▶ (collapsed) or
        // ▼ (expanded) and a space; a leaf gets two spaces for alignment.
        Cell plain = style;
        plain.bold = false;
        plain.inverse = false;
        const int indentOffset = item.depth * m_indentWidth;
        const int indicatorOffset = indentOffset + 2;
        fillRect(buffer, QRect(geo.x(), screenY, qMin(indicatorOffset, viewWidth), 1), plain);
        if (hasChildren)
            renderText(buffer, geo.x() + indentOffset, screenY,
                       item.expanded ? QStringLiteral("▼") : QStringLiteral("▶"),
                       viewWidth - indentOffset, plain);

        // Render item text with truncation
        if (m_delegate) {
            // Let delegate paint into the remaining area
            int remainingWidth = viewWidth - indicatorOffset;
            if (remainingWidth > 0) {
                QRect delegateRect(geo.x() + indicatorOffset, screenY,
                                   remainingWidth, 1);
                m_delegate->paint(buffer, delegateRect, item.index);
            }
        } else {
            QString text = m_model->data(item.index, Qt::DisplayRole).toString();
            int remainingWidth = viewWidth - indicatorOffset;
            if (remainingWidth > 0) {
                renderTextTruncated(buffer, geo.x() + indicatorOffset, screenY,
                                    text, remainingWidth, style);
            }
        }
    }
}

void QTuiTreeView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_model || m_flatList.isEmpty())
        return;
    if (editKeyPressEvent(event) || selectionKeyPressEvent(event))
        return;
    const int previousRow = m_currentRow;

    switch (event->key()) {
    case Qt::Key_Up:
        if (m_currentRow > 0) {
            --m_currentRow;
            scrollToVisible();
            markDirty();
        }
        break;

    case Qt::Key_Down:
        if (m_currentRow < m_flatList.size() - 1) {
            ++m_currentRow;
            scrollToVisible();
            markDirty();
        }
        break;

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
        }
        break;
    }

    case Qt::Key_Left: {
        const FlattenedItem &item = m_flatList.at(m_currentRow);
        if (item.expanded && m_model->hasChildren(item.index)) {
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

    if (m_currentRow != previousRow)
        selectCurrent(event->modifiers(), false);
}

void QTuiTreeView::mousePressEvent(TuiMouseEvent *event)
{
    // The Press before the DoubleClick already made the item current
    if (event->mouseType() == TuiMouseEvent::DoubleClick) {
        const QModelIndex index = currentIndex();
        if (!index.isValid())
            return;
        emit doubleClicked(index);
        if ((editTriggers() & DoubleClicked) && edit(index))
            return;
        // A double click on an item with children expands or collapses it
        if (m_model && m_model->hasChildren(index))
            isExpanded(index) ? collapse(index) : expand(index);
        emit activated(index);
        return;
    }
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    if (!m_model || m_flatList.isEmpty())
        return;

    const QRect geo = geometry();
    int clickedRow = (event->y() - geo.y()) + m_scrollOffset;
    if (clickedRow >= 0 && clickedRow < m_flatList.size()) {
        // Another button keeps a selection that contains the item
        m_currentRow = clickedRow;
        markDirty();
        if (event->button() == Qt::LeftButton)
            selectCurrent(event->modifiers(), true);
        else if (!isSelected(currentIndex()))
            selectCurrent(Qt::NoModifier, true);
    }
}

QItemSelection QTuiTreeView::selectionRange(const QModelIndex &from, const QModelIndex &to) const
{
    // The visible rows between the two items, at any depth
    const int a = flatRowForIndex(from);
    const int b = flatRowForIndex(to);
    if (a < 0 || b < 0)
        return QItemSelection(to, to);
    QItemSelection range;
    for (int row = qMin(a, b); row <= qMax(a, b); ++row) {
        const QModelIndex index = m_flatList.at(row).index;
        range.select(index, index);
    }
    return range;
}

void QTuiTreeView::selectAll()
{
    if (!selectionModel() || m_flatList.isEmpty() || selectionMode() < MultiSelection)
        return;
    selectionModel()->select(selectionRange(m_flatList.first().index, m_flatList.last().index),
                             QItemSelectionModel::ClearAndSelect);
}

// --- Model signal slots ---

void QTuiTreeView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    // Find the visible flat rows of the changed indexes
    const int lastVisibleRow = qMin(int(m_flatList.size()), m_scrollOffset + geometry().height()) - 1;
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
        markRowsDirty(dirtyTop, dirtyBottom);
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
    markRowsDirty(qMax(0, affectedFlatRow), INT_MAX);
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
    markRowsDirty(qMax(0, affectedFlatRow), INT_MAX);
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

    int row = currentIdx.isValid() ? flatRowForIndex(currentIdx) : -1;
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
    m_scrollOffset = 0;
    m_expandedItems.clear();
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
    buildFlatListRecursive(QModelIndex(), 0);
    updateEditorGeometry();
}

void QTuiTreeView::buildFlatListRecursive(const QModelIndex &parent, int depth)
{
    const int rowCount = m_model->rowCount(parent);
    for (int r = 0; r < rowCount; ++r) {
        QModelIndex idx = m_model->index(r, 0, parent);
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
    scrollRowIntoView(m_currentRow, geometry().height());
}

int QTuiTreeView::flatRowForIndex(const QModelIndex &index) const
{
    for (int i = 0; i < m_flatList.size(); ++i) {
        if (m_flatList.at(i).index == index)
            return i;
    }
    return -1;
}

} // namespace QtTUI
