#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiTreeView::QTuiTreeView(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

void QTuiTreeView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_currentRow = 0;
    m_scrollOffset = 0;
    m_expandedItems.clear();
    m_flatList.clear();

    if (m_model) {
        connectModel();
        rebuildFlatList();
    }
}

QAbstractItemModel *QTuiTreeView::model() const
{
    return m_model;
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
    if (row >= 0) {
        m_currentRow = row;
        scrollToVisible();
    }
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
    markDirtyFromRowToBottom(toggledRow >= 0 ? toggledRow : 0);

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
    markDirtyFromRowToBottom(toggledRow >= 0 ? toggledRow : 0);

    emit collapsed(index);
}

bool QTuiTreeView::isExpanded(const QModelIndex &index) const
{
    return m_expandedItems.contains(QPersistentModelIndex(index));
}

void QTuiTreeView::setItemDelegate(QTuiItemDelegate *delegate)
{
    m_delegate = delegate;
}

QTuiItemDelegate *QTuiTreeView::itemDelegate() const
{
    return m_delegate;
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

        // Determine style - use focus colors only when widget has focus
        Cell style;
        style.bold = false;
        style.inverse = false;
        if (isCurrent && hasFocus()) {
            style.foreground = theme.focusForeground;
            style.background = theme.focusBackground;
        } else {
            style.foreground = theme.foreground;
            style.background = theme.background;
        }

        // Fill the entire row with the appropriate background (for current row highlighting)
        if (isCurrent && hasFocus()) {
            for (int x = 0; x < viewWidth; ++x) {
                Cell &cell = buffer.backCell(geo.x() + x, screenY);
                cell.character = QLatin1Char(' ');
                cell.foreground = theme.focusForeground;
                cell.background = theme.focusBackground;
                cell.bold = false;
                cell.inverse = false;
            }
        }

        // Compute indentation offset
        int indentOffset = item.depth * m_indentWidth;

        // Render indentation spaces
        for (int i = 0; i < indentOffset && i < viewWidth; ++i) {
            Cell &cell = buffer.backCell(geo.x() + i, screenY);
            cell.character = QLatin1Char(' ');
            cell.foreground = style.foreground;
            cell.background = style.background;
            cell.bold = false;
            cell.inverse = false;
        }

        // Render expand/collapse indicator
        int indicatorOffset = indentOffset;
        if (hasChildren) {
            // Use multi-byte indicator characters: ▶ (collapsed) or ▼ (expanded)
            QChar indicatorChar = item.expanded ? QChar(0x25BC) : QChar(0x25B6);
            if (indicatorOffset < viewWidth) {
                Cell &cell = buffer.backCell(geo.x() + indicatorOffset, screenY);
                cell.character = indicatorChar;
                cell.foreground = style.foreground;
                cell.background = style.background;
                cell.bold = false;
                cell.inverse = false;
            }
            indicatorOffset++;
            // Space after indicator
            if (indicatorOffset < viewWidth) {
                Cell &cell = buffer.backCell(geo.x() + indicatorOffset, screenY);
                cell.character = QLatin1Char(' ');
                cell.foreground = style.foreground;
                cell.background = style.background;
                cell.bold = false;
                cell.inverse = false;
            }
            indicatorOffset++;
        } else {
            // No indicator for leaves, but reserve the same space for alignment
            for (int i = 0; i < 2 && indicatorOffset < viewWidth; ++i, ++indicatorOffset) {
                Cell &cell = buffer.backCell(geo.x() + indicatorOffset, screenY);
                cell.character = QLatin1Char(' ');
                cell.foreground = style.foreground;
                cell.background = style.background;
                cell.bold = false;
                cell.inverse = false;
            }
        }

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

    switch (event->key()) {
    case Qt::Key_Up:
        if (m_currentRow > 0) {
            --m_currentRow;
            scrollToVisible();
        }
        break;

    case Qt::Key_Down:
        if (m_currentRow < m_flatList.size() - 1) {
            ++m_currentRow;
            scrollToVisible();
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
}

// --- Model signal slots ---

void QTuiTreeView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(bottomRight);

    if (!m_model)
        return;

    const QRect geo = geometry();
    const int viewHeight = geo.height();
    if (viewHeight <= 0)
        return;

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + viewHeight - 1;

    // Map model indices to flat rows and mark visible ones dirty
    int dirtyTop = -1;
    int dirtyBottom = -1;

    for (int i = 0; i < m_flatList.size(); ++i) {
        const QModelIndex &idx = m_flatList.at(i).index;
        // Check if this flat row corresponds to a changed index
        if (idx.parent() == topLeft.parent() &&
            idx.row() >= topLeft.row() && idx.row() <= bottomRight.row()) {
            // This flat row is affected by the data change
            if (i >= firstVisibleRow && i <= lastVisibleRow) {
                if (dirtyTop < 0)
                    dirtyTop = i;
                dirtyBottom = i;
            }
        }
    }

    if (dirtyTop < 0)
        return; // No visible rows affected

    int screenTop = dirtyTop - m_scrollOffset;
    int screenBottom = dirtyBottom - m_scrollOffset;

    QRect dirtyRect(geo.x(), geo.y() + screenTop,
                    geo.width(), screenBottom - screenTop + 1);
    markDirty(dirtyRect);
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

    // Mark from affected row to bottom of viewport
    const QRect geo = geometry();
    const int viewHeight = geo.height();
    if (viewHeight <= 0)
        return;

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + viewHeight - 1;

    // If no affected row found, or it's below viewport, no marking needed
    if (affectedFlatRow < 0)
        affectedFlatRow = 0;

    if (affectedFlatRow > lastVisibleRow)
        return;

    int dirtyTop = qMax(affectedFlatRow, firstVisibleRow) - m_scrollOffset;
    QRect dirtyRect(geo.x(), geo.y() + dirtyTop,
                    geo.width(), viewHeight - dirtyTop);
    markDirty(dirtyRect);
}

void QTuiTreeView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(last);

    // Find the flat row corresponding to the removal point before rebuild
    int affectedFlatRow = -1;
    for (int i = 0; i < m_flatList.size(); ++i) {
        const QModelIndex &idx = m_flatList.at(i).index;
        if (idx.parent() == parent && idx.row() >= first) {
            affectedFlatRow = i;
            break;
        }
    }

    rebuildFlatList();

    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();

    // Mark from affected row to bottom of viewport
    const QRect geo = geometry();
    const int viewHeight = geo.height();
    if (viewHeight <= 0)
        return;

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + viewHeight - 1;

    if (affectedFlatRow < 0)
        affectedFlatRow = 0;

    if (affectedFlatRow > lastVisibleRow)
        return;

    int dirtyTop = qMax(affectedFlatRow, firstVisibleRow) - m_scrollOffset;
    QRect dirtyRect(geo.x(), geo.y() + dirtyTop,
                    geo.width(), viewHeight - dirtyTop);
    markDirty(dirtyRect);
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
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &QTuiTreeView::onRowsRemoved);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiTreeView::onModelReset);
}

void QTuiTreeView::connectModel()
{
    if (!m_model)
        return;

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &QTuiTreeView::onDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QTuiTreeView::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QTuiTreeView::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiTreeView::onModelReset);
}

void QTuiTreeView::rebuildFlatList()
{
    m_flatList.clear();
    if (!m_model)
        return;
    buildFlatListRecursive(QModelIndex(), 0);
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
    int visibleRows = geometry().height();
    if (visibleRows <= 0)
        return;

    if (m_currentRow < m_scrollOffset)
        m_scrollOffset = m_currentRow;
    else if (m_currentRow >= m_scrollOffset + visibleRows)
        m_scrollOffset = m_currentRow - visibleRows + 1;
}

int QTuiTreeView::flatRowForIndex(const QModelIndex &index) const
{
    for (int i = 0; i < m_flatList.size(); ++i) {
        if (m_flatList.at(i).index == index)
            return i;
    }
    return -1;
}

void QTuiTreeView::markDirtyFromRowToBottom(int flatRow)
{
    const QRect geo = geometry();
    const int viewHeight = geo.height();
    if (viewHeight <= 0)
        return;

    const int firstVisibleRow = m_scrollOffset;
    const int lastVisibleRow = m_scrollOffset + viewHeight - 1;

    // If the affected row is below the visible viewport, nothing to mark
    if (flatRow > lastVisibleRow)
        return;

    int dirtyTop = qMax(flatRow, firstVisibleRow) - m_scrollOffset;
    QRect dirtyRect(geo.x(), geo.y() + dirtyTop,
                    geo.width(), viewHeight - dirtyTop);
    markDirty(dirtyRect);
}

} // namespace QtTUI
