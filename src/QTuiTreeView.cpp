#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
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
    rebuildFlatList();
    // Restore current row to point to the same index after rebuild
    int row = flatRowForIndex(index);
    if (row >= 0)
        m_currentRow = row;
    scrollToVisible();
    emit expanded(index);
}

void QTuiTreeView::collapse(const QModelIndex &index)
{
    if (!m_model || !index.isValid())
        return;

    QPersistentModelIndex persistent(index);
    if (!m_expandedItems.contains(persistent))
        return;

    m_expandedItems.remove(persistent);
    rebuildFlatList();
    int row = flatRowForIndex(index);
    if (row >= 0)
        m_currentRow = row;
    // Clamp current row if it's now beyond the list
    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();
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
    if (geo.width() <= 0 || geo.height() <= 0 || !m_model)
        return;

    const auto theme = themeProperties();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    for (int vr = 0; vr < viewHeight; ++vr) {
        int flatRow = m_scrollOffset + vr;
        int screenY = geo.y() + vr;
        bool isCurrent = (flatRow == m_currentRow && flatRow < m_flatList.size());

        if (flatRow < m_flatList.size()) {
            const FlattenedItem &item = m_flatList.at(flatRow);
            bool hasChildren = m_model->hasChildren(item.index);

            // Build display string: indentation + expand indicator + text
            QString indent(item.depth * m_indentWidth, QLatin1Char(' '));
            QString indicator;
            if (hasChildren)
                indicator = item.expanded ? QStringLiteral("▼ ") : QStringLiteral("▶ ");
            else
                indicator = QStringLiteral("  ");

            QString text;
            if (m_delegate) {
                // Delegate painting: let delegate handle the cell area after indent+indicator
                int prefixLen = indent.length() + indicator.length();
                // Write prefix first
                for (int x = 0; x < viewWidth; ++x) {
                    Cell &cell = buffer.backCell(geo.x() + x, screenY);
                    QString prefix = indent + indicator;
                    cell.character = (x < prefix.length()) ? prefix.at(x) : QLatin1Char(' ');
                    cell.bold = false;
                    if (isCurrent) {
                        cell.foreground = theme.focusForeground;
                        cell.background = theme.focusBackground;
                    } else {
                        cell.foreground = theme.foreground;
                        cell.background = theme.background;
                    }
                    cell.inverse = isCurrent;
                }
                // Let delegate paint into the remaining area
                QRect delegateRect(geo.x() + prefixLen, screenY,
                                   viewWidth - prefixLen, 1);
                m_delegate->paint(buffer, delegateRect, item.index);
            } else {
                text = m_model->data(item.index, Qt::DisplayRole).toString();
                QString line = indent + indicator + text;

                for (int x = 0; x < viewWidth; ++x) {
                    Cell &cell = buffer.backCell(geo.x() + x, screenY);
                    cell.character = (x < line.length()) ? line.at(x) : QLatin1Char(' ');
                    cell.bold = false;
                    if (isCurrent) {
                        cell.foreground = theme.focusForeground;
                        cell.background = theme.focusBackground;
                    } else {
                        cell.foreground = theme.foreground;
                        cell.background = theme.background;
                    }
                    cell.inverse = isCurrent;
                }
            }
        } else {
            // Empty row below the last item
            for (int x = 0; x < viewWidth; ++x) {
                Cell &cell = buffer.backCell(geo.x() + x, screenY);
                cell.character = QLatin1Char(' ');
                cell.bold = false;
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.inverse = false;
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
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    // Data changed but structure didn't, no need to rebuild flat list
}

void QTuiTreeView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    QModelIndex currentIdx;
    if (m_currentRow >= 0 && m_currentRow < m_flatList.size())
        currentIdx = m_flatList.at(m_currentRow).index;

    rebuildFlatList();

    if (currentIdx.isValid()) {
        int row = flatRowForIndex(currentIdx);
        if (row >= 0)
            m_currentRow = row;
    }
    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();
}

void QTuiTreeView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    rebuildFlatList();

    if (m_currentRow >= m_flatList.size())
        m_currentRow = qMax(0, m_flatList.size() - 1);
    scrollToVisible();
}

void QTuiTreeView::onModelReset()
{
    m_currentRow = 0;
    m_scrollOffset = 0;
    m_expandedItems.clear();
    m_flatList.clear();
    rebuildFlatList();
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

} // namespace QtTUI
