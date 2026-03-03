#include <QtTUI/QTuiLayoutItem.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLayout.h>

namespace QtTUI {

// --- QTuiLayoutItem ---

QTuiSizePolicy QTuiLayoutItem::sizePolicy() const
{
    return m_sizePolicy;
}

void QTuiLayoutItem::setSizePolicy(const QTuiSizePolicy &policy)
{
    m_sizePolicy = policy;
}

// --- QTuiWidgetItem ---

QTuiWidgetItem::QTuiWidgetItem(QTuiWidget *widget)
    : m_widget(widget)
{
}

QSize QTuiWidgetItem::sizeHint() const
{
    if (!m_widget)
        return QSize(0, 0);
    return m_widget->minimumSize();
}

QSize QTuiWidgetItem::minimumSize() const
{
    if (!m_widget)
        return QSize(0, 0);
    return m_widget->minimumSize();
}

QSize QTuiWidgetItem::maximumSize() const
{
    return QSize(16777215, 16777215);
}

void QTuiWidgetItem::setGeometry(const QRect &rect)
{
    if (m_widget)
        m_widget->setGeometry(rect);
}

QRect QTuiWidgetItem::geometry() const
{
    if (!m_widget)
        return QRect();
    return m_widget->geometry();
}

bool QTuiWidgetItem::isEmpty() const
{
    return !m_widget || !m_widget->isVisible();
}

QTuiWidget *QTuiWidgetItem::widget() const
{
    return m_widget;
}

// --- QTuiSpacerItem ---

QTuiSpacerItem::QTuiSpacerItem(int width, int height,
                               QTuiSizePolicy::Policy hPolicy,
                               QTuiSizePolicy::Policy vPolicy)
    : m_width(width)
    , m_height(height)
{
    m_sizePolicy = QTuiSizePolicy(hPolicy, vPolicy);
}

QSize QTuiSpacerItem::sizeHint() const
{
    return QSize(m_width, m_height);
}

QSize QTuiSpacerItem::minimumSize() const
{
    const auto hPolicy = m_sizePolicy.horizontalPolicy();
    const auto vPolicy = m_sizePolicy.verticalPolicy();

    int w = (hPolicy == QTuiSizePolicy::Fixed ||
             hPolicy == QTuiSizePolicy::Minimum ||
             hPolicy == QTuiSizePolicy::Expanding ||
             hPolicy == QTuiSizePolicy::MinimumExpanding)
            ? m_width : 0;

    int h = (vPolicy == QTuiSizePolicy::Fixed ||
             vPolicy == QTuiSizePolicy::Minimum ||
             vPolicy == QTuiSizePolicy::Expanding ||
             vPolicy == QTuiSizePolicy::MinimumExpanding)
            ? m_height : 0;

    return QSize(w, h);
}

QSize QTuiSpacerItem::maximumSize() const
{
    const auto hPolicy = m_sizePolicy.horizontalPolicy();
    const auto vPolicy = m_sizePolicy.verticalPolicy();

    int w = (hPolicy == QTuiSizePolicy::Fixed) ? m_width : 16777215;
    int h = (vPolicy == QTuiSizePolicy::Fixed) ? m_height : 16777215;

    return QSize(w, h);
}

void QTuiSpacerItem::setGeometry(const QRect &rect)
{
    m_geometry = rect;
}

QRect QTuiSpacerItem::geometry() const
{
    return m_geometry;
}

bool QTuiSpacerItem::isEmpty() const
{
    return false;
}

// --- QTuiLayoutItemWrapper ---

QTuiLayoutItemWrapper::QTuiLayoutItemWrapper(QTuiLayout *layout)
    : m_layout(layout)
{
}

QSize QTuiLayoutItemWrapper::sizeHint() const
{
    return QSize(0, 0);
}

QSize QTuiLayoutItemWrapper::minimumSize() const
{
    return QSize(0, 0);
}

QSize QTuiLayoutItemWrapper::maximumSize() const
{
    return QSize(16777215, 16777215);
}

void QTuiLayoutItemWrapper::setGeometry(const QRect &rect)
{
    m_geometry = rect;
    if (m_layout)
        m_layout->apply(rect);
}

QRect QTuiLayoutItemWrapper::geometry() const
{
    return m_geometry;
}

bool QTuiLayoutItemWrapper::isEmpty() const
{
    return !m_layout || m_layout->count() == 0;
}

QTuiLayout *QTuiLayoutItemWrapper::layout() const
{
    return m_layout;
}

} // namespace QtTUI
