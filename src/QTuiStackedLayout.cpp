#include <QtTUI/QTuiStackedLayout.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

QTuiStackedLayout::QTuiStackedLayout(QObject *parent)
    : QTuiLayout(parent)
{
}

int QTuiStackedLayout::addWidget(QTuiWidget *widget)
{
    if (!widget)
        return -1;

    const int index = m_widgets.size();
    m_widgets.append(widget);

    if (m_widgets.size() == 1) {
        // First widget: make it current and visible
        m_currentIndex = 0;
        widget->setVisible(true);
        emit currentChanged(0);
    } else {
        // Hide non-current widgets
        widget->setVisible(false);
    }

    return index;
}

QTuiWidget *QTuiStackedLayout::widget(int index) const
{
    if (index < 0 || index >= m_widgets.size())
        return nullptr;
    return m_widgets.at(index);
}

int QTuiStackedLayout::currentIndex() const
{
    return m_currentIndex;
}

void QTuiStackedLayout::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_widgets.size())
        return;
    if (index == m_currentIndex)
        return;

    // Hide the old current widget
    if (m_currentIndex >= 0 && m_currentIndex < m_widgets.size())
        m_widgets.at(m_currentIndex)->setVisible(false);

    m_currentIndex = index;

    // Show the new current widget
    m_widgets.at(m_currentIndex)->setVisible(true);

    emit currentChanged(m_currentIndex);
}

QTuiWidget *QTuiStackedLayout::currentWidget() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_widgets.size())
        return nullptr;
    return m_widgets.at(m_currentIndex);
}

void QTuiStackedLayout::setCurrentWidget(QTuiWidget *widget)
{
    const int index = m_widgets.indexOf(widget);
    if (index >= 0)
        setCurrentIndex(index);
}

void QTuiStackedLayout::apply(const QRect &availableArea)
{
    if (m_widgets.isEmpty())
        return;

    const QRect rect = contentsRect(availableArea);

    for (int i = 0; i < m_widgets.size(); ++i) {
        if (i == m_currentIndex) {
            m_widgets.at(i)->setGeometry(rect);
            m_widgets.at(i)->setVisible(true);
        } else {
            m_widgets.at(i)->setVisible(false);
        }
    }
}

} // namespace QtTUI
