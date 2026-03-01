#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiWidget.h>

#include <algorithm>

namespace QtTUI {

QTuiLayout::QTuiLayout(QObject *parent)
    : QObject(parent)
{
}

QTuiLayout::~QTuiLayout()
{
    qDeleteAll(m_items);
}

void QTuiLayout::addItem(QTuiLayoutItem *item)
{
    if (item)
        m_items.append(item);
}

void QTuiLayout::addWidget(QTuiWidget *widget)
{
    if (widget)
        addItem(new QTuiWidgetItem(widget));
}

QTuiLayoutItem *QTuiLayout::itemAt(int index) const
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    return m_items.at(index);
}

int QTuiLayout::count() const
{
    return m_items.size();
}

void QTuiLayout::setContentsMargins(int left, int top, int right, int bottom)
{
    m_marginLeft = std::max(0, left);
    m_marginTop = std::max(0, top);
    m_marginRight = std::max(0, right);
    m_marginBottom = std::max(0, bottom);
}

void QTuiLayout::contentsMargins(int *left, int *top, int *right, int *bottom) const
{
    if (left)
        *left = m_marginLeft;
    if (top)
        *top = m_marginTop;
    if (right)
        *right = m_marginRight;
    if (bottom)
        *bottom = m_marginBottom;
}

QRect QTuiLayout::contentsRect(const QRect &availableArea) const
{
    return availableArea.adjusted(m_marginLeft, m_marginTop,
                                 -m_marginRight, -m_marginBottom);
}

} // namespace QtTUI
