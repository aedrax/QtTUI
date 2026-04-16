#include <QtTUI/QTuiGridLayout.h>
#include <QtTUI/QTuiWidget.h>

#include <algorithm>

namespace QtTUI {

QTuiGridLayout::QTuiGridLayout(QObject *parent)
    : QTuiLayout(parent)
{
}

QTuiGridLayout::~QTuiGridLayout()
{
    for (const auto &gi : m_gridItems)
        delete gi.item;
}

void QTuiGridLayout::addWidget(QTuiWidget *widget, int row, int column,
                               int rowSpan, int columnSpan)
{
    if (!widget || row < 0 || column < 0)
        return;
    if (rowSpan < 1) rowSpan = 1;
    if (columnSpan < 1) columnSpan = 1;

    auto *item = new QTuiWidgetItem(widget);
    m_gridItems.append({item, row, column, rowSpan, columnSpan});

    // Invalidate the layout's parent widget so the entire content area is repainted
    if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
        pw->markDirty();
    }
}

void QTuiGridLayout::addLayout(QTuiLayout *layout, int row, int column,
                               int rowSpan, int columnSpan)
{
    if (!layout || row < 0 || column < 0)
        return;
    if (rowSpan < 1) rowSpan = 1;
    if (columnSpan < 1) columnSpan = 1;

    auto *item = new QTuiLayoutItemWrapper(layout);
    m_gridItems.append({item, row, column, rowSpan, columnSpan});

    // Invalidate the layout's parent widget so the entire content area is repainted
    if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
        pw->markDirty();
    }
}

void QTuiGridLayout::removeWidget(QTuiWidget *widget)
{
    if (!widget)
        return;

    for (int i = 0; i < m_gridItems.size(); ++i) {
        auto *widgetItem = dynamic_cast<QTuiWidgetItem *>(m_gridItems[i].item);
        if (widgetItem && widgetItem->widget() == widget) {
            delete m_gridItems[i].item;
            m_gridItems.removeAt(i);

            // Invalidate the layout's parent widget so the entire content area is repainted
            if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
                pw->markDirty();
            }
            return;
        }
    }
}

void QTuiGridLayout::setRowStretch(int row, int stretch)
{
    if (row >= 0 && stretch >= 0)
        m_rowStretches[row] = stretch;
}

void QTuiGridLayout::setColumnStretch(int column, int stretch)
{
    if (column >= 0 && stretch >= 0)
        m_columnStretches[column] = stretch;
}

void QTuiGridLayout::setRowMinimumHeight(int row, int height)
{
    if (row >= 0 && height >= 0)
        m_rowMinHeights[row] = height;
}

void QTuiGridLayout::setColumnMinimumWidth(int column, int width)
{
    if (column >= 0 && width >= 0)
        m_colMinWidths[column] = width;
}

int QTuiGridLayout::rowCount() const
{
    int maxRow = 0;
    for (const auto &gi : m_gridItems)
        maxRow = std::max(maxRow, gi.row + gi.rowSpan);
    // Also account for stretch/min settings beyond items
    for (auto it = m_rowStretches.cbegin(); it != m_rowStretches.cend(); ++it)
        maxRow = std::max(maxRow, it.key() + 1);
    for (auto it = m_rowMinHeights.cbegin(); it != m_rowMinHeights.cend(); ++it)
        maxRow = std::max(maxRow, it.key() + 1);
    return maxRow;
}

int QTuiGridLayout::columnCount() const
{
    int maxCol = 0;
    for (const auto &gi : m_gridItems)
        maxCol = std::max(maxCol, gi.column + gi.columnSpan);
    for (auto it = m_columnStretches.cbegin(); it != m_columnStretches.cend(); ++it)
        maxCol = std::max(maxCol, it.key() + 1);
    for (auto it = m_colMinWidths.cbegin(); it != m_colMinWidths.cend(); ++it)
        maxCol = std::max(maxCol, it.key() + 1);
    return maxCol;
}

void QTuiGridLayout::apply(const QRect &availableArea)
{
    if (m_gridItems.isEmpty())
        return;

    const QRect rect = contentsRect(availableArea);
    if (rect.isEmpty())
        return;

    const int rows = rowCount();
    const int cols = columnCount();
    if (rows == 0 || cols == 0)
        return;

    // --- Pass 1: Measurement ---
    // Compute minimum sizes for each row and column from single-span items
    QVector<int> rowMinH(rows, 0);
    QVector<int> colMinW(cols, 0);

    for (const auto &gi : m_gridItems) {
        if (gi.item->isEmpty())
            continue;
        const QSize minSz = gi.item->minimumSize();
        if (gi.rowSpan == 1)
            rowMinH[gi.row] = std::max(rowMinH[gi.row], minSz.height());
        if (gi.columnSpan == 1)
            colMinW[gi.column] = std::max(colMinW[gi.column], minSz.width());
    }

    // Apply configured minimum sizes
    for (auto it = m_rowMinHeights.cbegin(); it != m_rowMinHeights.cend(); ++it) {
        if (it.key() < rows)
            rowMinH[it.key()] = std::max(rowMinH[it.key()], it.value());
    }
    for (auto it = m_colMinWidths.cbegin(); it != m_colMinWidths.cend(); ++it) {
        if (it.key() < cols)
            colMinW[it.key()] = std::max(colMinW[it.key()], it.value());
    }

    // --- Pass 2: Distribution ---
    // Distribute available space according to stretch factors

    // Row distribution
    QVector<int> rowHeights(rows, 0);
    {
        int totalMinH = 0;
        for (int r = 0; r < rows; ++r)
            totalMinH += rowMinH[r];

        int remaining = rect.height() - totalMinH;
        if (remaining < 0) remaining = 0;

        // Collect stretch factors
        int totalStretch = 0;
        bool hasStretch = false;
        for (int r = 0; r < rows; ++r) {
            int s = m_rowStretches.value(r, 0);
            if (s > 0) hasStretch = true;
            totalStretch += s;
        }

        for (int r = 0; r < rows; ++r)
            rowHeights[r] = rowMinH[r];

        if (remaining > 0) {
            if (hasStretch && totalStretch > 0) {
                // Distribute proportionally to stretch factors
                int distributed = 0;
                for (int r = 0; r < rows; ++r) {
                    int s = m_rowStretches.value(r, 0);
                    if (s > 0) {
                        int extra = remaining * s / totalStretch;
                        rowHeights[r] += extra;
                        distributed += extra;
                    }
                }
                // Give leftover pixels to the last stretched row
                int leftover = remaining - distributed;
                for (int r = rows - 1; r >= 0 && leftover > 0; --r) {
                    if (m_rowStretches.value(r, 0) > 0) {
                        rowHeights[r] += leftover;
                        leftover = 0;
                    }
                }
            } else {
                // No stretch factors: distribute equally
                int perRow = remaining / rows;
                int leftover = remaining - perRow * rows;
                for (int r = 0; r < rows; ++r) {
                    rowHeights[r] += perRow;
                    if (r < leftover)
                        rowHeights[r] += 1;
                }
            }
        }
    }

    // Column distribution
    QVector<int> colWidths(cols, 0);
    {
        int totalMinW = 0;
        for (int c = 0; c < cols; ++c)
            totalMinW += colMinW[c];

        int remaining = rect.width() - totalMinW;
        if (remaining < 0) remaining = 0;

        int totalStretch = 0;
        bool hasStretch = false;
        for (int c = 0; c < cols; ++c) {
            int s = m_columnStretches.value(c, 0);
            if (s > 0) hasStretch = true;
            totalStretch += s;
        }

        for (int c = 0; c < cols; ++c)
            colWidths[c] = colMinW[c];

        if (remaining > 0) {
            if (hasStretch && totalStretch > 0) {
                int distributed = 0;
                for (int c = 0; c < cols; ++c) {
                    int s = m_columnStretches.value(c, 0);
                    if (s > 0) {
                        int extra = remaining * s / totalStretch;
                        colWidths[c] += extra;
                        distributed += extra;
                    }
                }
                int leftover = remaining - distributed;
                for (int c = cols - 1; c >= 0 && leftover > 0; --c) {
                    if (m_columnStretches.value(c, 0) > 0) {
                        colWidths[c] += leftover;
                        leftover = 0;
                    }
                }
            } else {
                int perCol = remaining / cols;
                int leftover = remaining - perCol * cols;
                for (int c = 0; c < cols; ++c) {
                    colWidths[c] += perCol;
                    if (c < leftover)
                        colWidths[c] += 1;
                }
            }
        }
    }

    // Compute cumulative positions
    QVector<int> rowY(rows);
    rowY[0] = rect.y();
    for (int r = 1; r < rows; ++r)
        rowY[r] = rowY[r - 1] + rowHeights[r - 1];

    QVector<int> colX(cols);
    colX[0] = rect.x();
    for (int c = 1; c < cols; ++c)
        colX[c] = colX[c - 1] + colWidths[c - 1];

    // Assign geometry to each grid item (supporting spanning)
    for (const auto &gi : m_gridItems) {
        if (gi.item->isEmpty())
            continue;

        int x = colX[gi.column];
        int y = rowY[gi.row];

        int w = 0;
        for (int c = gi.column; c < gi.column + gi.columnSpan && c < cols; ++c)
            w += colWidths[c];

        int h = 0;
        for (int r = gi.row; r < gi.row + gi.rowSpan && r < rows; ++r)
            h += rowHeights[r];

        gi.item->setGeometry(QRect(x, y, w, h));
    }
}

} // namespace QtTUI
