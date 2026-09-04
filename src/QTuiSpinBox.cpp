#include <QtTUI/QTuiSpinBox.h>

namespace QtTUI {

QTuiSpinBox::QTuiSpinBox(QTuiWidget *parent)
    : QTuiAbstractSpinBox(parent)
{
}

int QTuiSpinBox::value() const
{
    return m_value;
}

void QTuiSpinBox::setValue(int value)
{
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

int QTuiSpinBox::minimum() const
{
    return m_minimum;
}

void QTuiSpinBox::setMinimum(int min)
{
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value); // re-clamp
}

int QTuiSpinBox::maximum() const
{
    return m_maximum;
}

void QTuiSpinBox::setMaximum(int max)
{
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value); // re-clamp
}

void QTuiSpinBox::setRange(int min, int max)
{
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value); // re-clamp
}

int QTuiSpinBox::singleStep() const
{
    return m_singleStep;
}

void QTuiSpinBox::setSingleStep(int step)
{
    if (step > 0)
        m_singleStep = step;
}

bool QTuiSpinBox::wrapping() const
{
    return m_wrapping;
}

void QTuiSpinBox::setWrapping(bool wrapping)
{
    m_wrapping = wrapping;
}

void QTuiSpinBox::stepBy(int steps)
{
    qint64 newValue = qint64(m_value) + qint64(steps) * m_singleStep;
    if (newValue > m_maximum)
        newValue = m_wrapping ? m_minimum : m_maximum;
    else if (newValue < m_minimum)
        newValue = m_wrapping ? m_maximum : m_minimum;
    setValue(int(newValue));
}

QString QTuiSpinBox::valueText() const
{
    return QString::number(m_value);
}

QSize QTuiSpinBox::minimumSize() const
{
    return minimumSizeFor(QString::number(m_minimum), QString::number(m_maximum));
}

} // namespace QtTUI
