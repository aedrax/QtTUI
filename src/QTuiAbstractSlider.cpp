#include <QtTUI/QTuiAbstractSlider.h>

namespace QtTUI {

QTuiAbstractSlider::QTuiAbstractSlider(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
    setFocusPolicy(Qt::StrongFocus);
}

int QTuiAbstractSlider::value() const
{
    return m_value;
}

void QTuiAbstractSlider::setValue(int value)
{
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

int QTuiAbstractSlider::minimum() const
{
    return m_minimum;
}

void QTuiAbstractSlider::setMinimum(int min)
{
    if (m_minimum == min)
        return;
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
    markDirty();
}

int QTuiAbstractSlider::maximum() const
{
    return m_maximum;
}

void QTuiAbstractSlider::setMaximum(int max)
{
    if (m_maximum == max)
        return;
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
    markDirty();
}

void QTuiAbstractSlider::setRange(int min, int max)
{
    max = qMax(min, max);
    if (m_minimum == min && m_maximum == max)
        return;
    m_minimum = min;
    m_maximum = max;
    setValue(m_value);
    markDirty();
}

int QTuiAbstractSlider::singleStep() const
{
    return m_singleStep;
}

void QTuiAbstractSlider::setSingleStep(int step)
{
    if (step > 0)
        m_singleStep = step;
}

Qt::Orientation QTuiAbstractSlider::orientation() const
{
    return m_orientation;
}

void QTuiAbstractSlider::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation)
        return;
    m_orientation = orientation;
    markDirty();
}

int QTuiAbstractSlider::valueAt(int x, int y) const
{
    const QRect geo = geometry();
    const int value = m_orientation == Qt::Horizontal
        ? m_minimum + (x - geo.x()) * (m_maximum - m_minimum) / qMax(1, geo.width() - 1)
        : m_minimum + (y - geo.y()) * (m_maximum - m_minimum) / qMax(1, geo.height() - 1);
    return qBound(m_minimum, value, m_maximum);
}

} // namespace QtTUI
