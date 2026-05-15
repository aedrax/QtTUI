#include <QtTUI/QTuiStackedWidget.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiStackedWidget::QTuiStackedWidget(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

int QTuiStackedWidget::addWidget(QTuiWidget *widget)
{
    if (!widget)
        return -1;

    const int index = m_widgets.size();
    m_widgets.append(widget);
    widget->setParent(this);

    if (m_widgets.size() == 1) {
        m_currentIndex = 0;
        widget->setVisible(true);
        emit currentChanged(0);
    } else {
        widget->setVisible(false);
    }

    return index;
}

void QTuiStackedWidget::removeWidget(QTuiWidget *widget)
{
    if (!widget)
        return;

    const int index = m_widgets.indexOf(widget);
    if (index < 0)
        return;

    m_widgets.removeAt(index);
    widget->setVisible(false);

    if (m_widgets.isEmpty()) {
        m_currentIndex = -1;
        emit currentChanged(-1);
    } else if (index == m_currentIndex) {
        // Removed the current widget, show the previous one (or first)
        m_currentIndex = qMin(index, m_widgets.size() - 1);
        m_widgets.at(m_currentIndex)->setVisible(true);
        emit currentChanged(m_currentIndex);
    } else if (index < m_currentIndex) {
        // Removed before current, adjust index without signal
        --m_currentIndex;
    }
}

int QTuiStackedWidget::count() const
{
    return m_widgets.size();
}

QTuiWidget *QTuiStackedWidget::widget(int index) const
{
    if (index < 0 || index >= m_widgets.size())
        return nullptr;
    return m_widgets.at(index);
}

int QTuiStackedWidget::indexOf(QTuiWidget *widget) const
{
    return m_widgets.indexOf(widget);
}

int QTuiStackedWidget::currentIndex() const
{
    return m_currentIndex;
}

void QTuiStackedWidget::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_widgets.size())
        return;
    if (index == m_currentIndex)
        return;

    // Hide old widget and mark it dirty
    if (m_currentIndex >= 0 && m_currentIndex < m_widgets.size()) {
        QTuiWidget *oldWidget = m_widgets.at(m_currentIndex);
        oldWidget->setVisible(false);
        oldWidget->markDirty();
    }

    m_currentIndex = index;

    // Show new widget and mark it dirty
    m_widgets.at(m_currentIndex)->setVisible(true);
    m_widgets.at(m_currentIndex)->markDirty();

    emit currentChanged(m_currentIndex);
}

QTuiWidget *QTuiStackedWidget::currentWidget() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_widgets.size())
        return nullptr;
    return m_widgets.at(m_currentIndex);
}

void QTuiStackedWidget::setCurrentWidget(QTuiWidget *widget)
{
    const int index = m_widgets.indexOf(widget);
    if (index >= 0)
        setCurrentIndex(index);
}

QSize QTuiStackedWidget::minimumSize() const
{
    // Any page can become current, so reserve space for the largest one
    QSize size(0, 0);
    for (auto *w : m_widgets)
        size = size.expandedTo(w->minimumSize());
    return size;
}

void QTuiStackedWidget::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.foreground;
    const Color bg = theme.background;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // Clear the widget area
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int px = x0 + col;
            int py = y0 + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
            }
        }
    }

    // Paint only the current widget
    if (m_currentIndex >= 0 && m_currentIndex < m_widgets.size()) {
        QTuiWidget *current = m_widgets.at(m_currentIndex);
        current->setGeometry(geo);
        if (current->isVisible())
            current->paintEvent(buffer);
    }
}

} // namespace QtTUI
