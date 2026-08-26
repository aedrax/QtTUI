#include <QtTUI/QTuiColumnView.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiColumnView::QTuiColumnView(QTuiWidget *parent)
    : QTuiAbstractItemView(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Layouts resize views through setGeometry(); no resize events are
    // delivered to child widgets. Re-clamp the column scroll offset whenever
    // our geometry changes so the active column stays visible.
    connect(this, &QTuiWidget::geometryChanged, this, [this](const QRect &) {
        if (!m_model)
            return;
        scrollToVisible();
        markDirty();
    });
}

void QTuiColumnView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_columns.clear();
    m_activeColumn = 0;
    m_scrollOffset = 0;

    if (m_model) {
        connectModel();
        rebuildColumns();
    }
    resetSelectionModel();
    markDirty();
}

QModelIndex QTuiColumnView::currentIndex() const
{
    if (m_columns.isEmpty() || m_activeColumn < 0 || m_activeColumn >= m_columns.size())
        return QModelIndex();

    const Column &col = m_columns.at(m_activeColumn);
    if (!m_model)
        return QModelIndex();

    int rowCount = m_model->rowCount(col.rootIndex);
    if (col.currentRow < 0 || col.currentRow >= rowCount)
        return QModelIndex();

    return m_model->index(col.currentRow, 0, col.rootIndex);
}

void QTuiColumnView::setCurrentIndex(const QModelIndex &index)
{
    if (!m_model || !index.isValid())
        return;

    // Build the path from root to this index
    QList<QModelIndex> path;
    QModelIndex current = index;
    while (current.isValid()) {
        path.prepend(current);
        current = current.parent();
    }

    // Rebuild columns following the path
    m_columns.clear();
    Column rootCol;
    rootCol.rootIndex = QModelIndex();
    rootCol.currentRow = 0;
    m_columns.append(rootCol);

    for (int i = 0; i < path.size(); ++i) {
        const QModelIndex &pathIdx = path.at(i);
        // Set the current row in the last column
        Column &lastCol = m_columns.last();
        lastCol.currentRow = pathIdx.row();

        // If this item has children and there's more path to follow, add a column
        if (i < path.size() - 1 && m_model->hasChildren(pathIdx)) {
            Column childCol;
            childCol.rootIndex = pathIdx;
            childCol.currentRow = 0;
            m_columns.append(childCol);
        }
    }

    m_activeColumn = m_columns.size() - 1;
    scrollToVisible();
    markDirty();
    selectCurrent(Qt::NoModifier, false);
}

int QTuiColumnView::defaultColumnWidth() const
{
    return m_defaultColumnWidth;
}

void QTuiColumnView::setDefaultColumnWidth(int width)
{
    if (width > 0 && width != m_defaultColumnWidth) {
        m_defaultColumnWidth = width;
        markDirty();
    }
}

QList<int> QTuiColumnView::columnWidths() const
{
    return m_columnWidths;
}

void QTuiColumnView::setColumnWidths(const QList<int> &widths)
{
    if (m_columnWidths == widths)
        return;
    m_columnWidths = widths;
    markDirty();
}

QSize QTuiColumnView::minimumSize() const
{
    return QSize(m_defaultColumnWidth, 3);
}

int QTuiColumnView::columnWidth(int columnIndex) const
{
    if (columnIndex >= 0 && columnIndex < m_columnWidths.size())
        return m_columnWidths.at(columnIndex);
    return m_defaultColumnWidth;
}

void QTuiColumnView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0 || !m_model)
        return;

    const auto theme = themeProperties();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    // Clear the entire area first
    for (int y = 0; y < viewHeight; ++y) {
        for (int x = 0; x < viewWidth; ++x) {
            Cell &cell = buffer.backCell(geo.x() + x, geo.y() + y);
            cell.setChar(U' ');
            cell.foreground = theme.foreground;
            cell.background = theme.background;
            cell.bold = false;
            cell.inverse = false;
        }
    }

    // Draw each visible column
    int xOffset = 0;
    for (int ci = m_scrollOffset; ci < m_columns.size(); ++ci) {
        int colW = columnWidth(ci);
        if (xOffset >= viewWidth)
            break;

        // Clamp column width to remaining space (reserve 1 for separator if not last)
        int drawWidth = qMin(colW, viewWidth - xOffset);
        bool isActiveCol = (ci == m_activeColumn);

        const Column &col = m_columns.at(ci);
        int rowCount = m_model->rowCount(col.rootIndex);

        for (int vr = 0; vr < viewHeight; ++vr) {
            int screenY = geo.y() + vr;

            if (vr < rowCount) {
                QModelIndex idx = m_model->index(vr, 0, col.rootIndex);
                QString text = m_model->data(idx, Qt::DisplayRole).toString();
                bool hasChildren = m_model->hasChildren(idx);
                bool isCurrent = (isActiveCol && vr == col.currentRow);

                // Append arrow indicator for items with children
                if (hasChildren)
                    text += QStringLiteral(" ▶");

                for (int x = 0; x < drawWidth; ++x) {
                    Cell &cell = buffer.backCell(geo.x() + xOffset + x, screenY);
                    cell.setChar((x < text.length()) ? text.at(x).unicode() : u' ');
                    cell.bold = false;

                    if (isCurrent || isSelected(idx)) {
                        cell.foreground = theme.focusForeground;
                        cell.background = theme.focusBackground;
                    } else {
                        cell.foreground = theme.foreground;
                        cell.background = theme.background;
                    }
                    cell.inverse = isCurrent;
                }
            }
            // Empty rows already cleared above
        }

        xOffset += drawWidth;

        // Draw column separator if there's space
        if (xOffset < viewWidth && ci < m_columns.size() - 1) {
            for (int vr = 0; vr < viewHeight; ++vr) {
                Cell &cell = buffer.backCell(geo.x() + xOffset, geo.y() + vr);
                cell.setChar(0x2502); // │ vertical line
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.bold = false;
                cell.inverse = false;
            }
            ++xOffset;
        }
    }
}

void QTuiColumnView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_model || m_columns.isEmpty())
        return;

    if (selectionKeyPressEvent(event))
        return;

    Column &activeCol = m_columns[m_activeColumn];
    int rowCount = m_model->rowCount(activeCol.rootIndex);
    const QModelIndex previous = currentIndex();

    switch (event->key()) {
    case Qt::Key_Up:
        if (activeCol.currentRow > 0) {
            --activeCol.currentRow;
            // When moving up/down, rebuild child columns for new selection
            rebuildColumns();
            markDirty();
        }
        break;

    case Qt::Key_Down:
        if (activeCol.currentRow < rowCount - 1) {
            ++activeCol.currentRow;
            rebuildColumns();
            markDirty();
        }
        break;

    case Qt::Key_Right: {
        // Move into child column if current item has children
        QModelIndex idx = currentIndex();
        if (idx.isValid() && m_model->hasChildren(idx)) {
            // If there's already a child column, just move active to it
            if (m_activeColumn + 1 < m_columns.size()) {
                ++m_activeColumn;
                scrollToVisible();
                markDirty();
            }
        }
        break;
    }

    case Qt::Key_Left:
        if (m_activeColumn > 0) {
            --m_activeColumn;
            // Trim columns to the right of the new active column's child
            rebuildColumns();
            scrollToVisible();
            markDirty();
        }
        break;

    case Qt::Key_Return:
    case Qt::Key_Enter: {
        QModelIndex idx = currentIndex();
        if (idx.isValid())
            emit activated(idx);
        break;
    }

    default:
        break;
    }

    if (currentIndex() != previous)
        selectCurrent(event->modifiers(), false);
}

// --- Model signal slots ---

void QTuiColumnView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    // Data changed but structure didn't, just repaint
    markDirty();
}

void QTuiColumnView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);
    rebuildColumns();
    markDirty();
}

void QTuiColumnView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);
    rebuildColumns();
    markDirty();
}

void QTuiColumnView::onLayoutChanged()
{
    // Rows may have been reordered wholesale (e.g. proxy sort). Rebuild the
    // column chain, which re-clamps each column's current row.
    rebuildColumns();
    markDirty();
}

void QTuiColumnView::onModelReset()
{
    m_columns.clear();
    m_activeColumn = 0;
    m_scrollOffset = 0;
    rebuildColumns();
    markDirty();
}

// --- Private helpers ---

void QTuiColumnView::connectModel()
{
    if (!m_model)
        return;

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &QTuiColumnView::onDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QTuiColumnView::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QTuiColumnView::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::layoutChanged,
            this, &QTuiColumnView::onLayoutChanged);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiColumnView::onModelReset);
    // Guard against the model being deleted while still set on the view
    connect(m_model, &QObject::destroyed, this, [this]() {
        m_model = nullptr;
        onModelReset();
    });
}

void QTuiColumnView::disconnectModel()
{
    if (!m_model)
        return;

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &QTuiColumnView::onDataChanged);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &QTuiColumnView::onRowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &QTuiColumnView::onRowsRemoved);
    disconnect(m_model, &QAbstractItemModel::layoutChanged,
               this, &QTuiColumnView::onLayoutChanged);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiColumnView::onModelReset);
    disconnect(m_model, &QObject::destroyed, this, nullptr);
}

void QTuiColumnView::rebuildColumns()
{
    if (!m_model)
        return;

    // Start from the root column and rebuild the chain based on selections.
    // Preserve columns up to and including the active column, then rebuild children.
    if (m_columns.isEmpty()) {
        Column rootCol;
        rootCol.rootIndex = QModelIndex();
        rootCol.currentRow = 0;
        m_columns.append(rootCol);
        m_activeColumn = 0;
    }

    // Clamp active column
    if (m_activeColumn >= m_columns.size())
        m_activeColumn = m_columns.size() - 1;

    // Remove columns after the active column
    while (m_columns.size() > m_activeColumn + 1)
        m_columns.removeLast();

    // From the active column, if the selected item has children, add a child column
    const Column &active = m_columns.at(m_activeColumn);
    int rowCount = m_model->rowCount(active.rootIndex);

    if (rowCount > 0) {
        // Clamp current row
        if (m_columns[m_activeColumn].currentRow >= rowCount)
            m_columns[m_activeColumn].currentRow = rowCount - 1;
        if (m_columns[m_activeColumn].currentRow < 0)
            m_columns[m_activeColumn].currentRow = 0;

        QModelIndex selectedIdx = m_model->index(m_columns[m_activeColumn].currentRow, 0,
                                                  active.rootIndex);
        if (selectedIdx.isValid() && m_model->hasChildren(selectedIdx)) {
            Column childCol;
            childCol.rootIndex = selectedIdx;
            childCol.currentRow = 0;
            m_columns.append(childCol);
        }
    }

    scrollToVisible();
}

void QTuiColumnView::scrollToVisible()
{
    int viewWidth = geometry().width();
    if (viewWidth <= 0)
        return;

    // Calculate total width of columns from scrollOffset to activeColumn
    // Ensure the active column is visible
    if (m_activeColumn < m_scrollOffset)
        m_scrollOffset = m_activeColumn;

    // Check if active column fits in view
    int usedWidth = 0;
    for (int ci = m_scrollOffset; ci <= m_activeColumn && ci < m_columns.size(); ++ci) {
        int colW = columnWidth(ci);
        // Add separator width
        if (ci < m_columns.size() - 1)
            colW += 1;
        usedWidth += colW;
    }

    // If active column doesn't fit, scroll right
    while (usedWidth > viewWidth && m_scrollOffset < m_activeColumn) {
        int removedW = columnWidth(m_scrollOffset);
        if (m_scrollOffset < m_columns.size() - 1)
            removedW += 1;
        usedWidth -= removedW;
        ++m_scrollOffset;
    }
}

} // namespace QtTUI
