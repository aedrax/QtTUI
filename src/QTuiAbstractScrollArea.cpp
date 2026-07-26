#include <QtTUI/QTuiAbstractScrollArea.h>

namespace QtTUI {

QTuiAbstractScrollArea::QTuiAbstractScrollArea(QTuiWidget *parent)
    : QTuiFrame(parent)
{
}

int QTuiAbstractScrollArea::scrollOffset() const
{
    return m_scrollOffset;
}

void QTuiAbstractScrollArea::scrollRowIntoView(int row, int visibleRows)
{
    if (visibleRows <= 0)
        return;
    if (row < m_scrollOffset)
        m_scrollOffset = row;
    else if (row >= m_scrollOffset + visibleRows)
        m_scrollOffset = row - visibleRows + 1;
}

} // namespace QtTUI
