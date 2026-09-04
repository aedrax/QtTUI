#include <QtTUI/QTuiDoubleSpinBox.h>

namespace QtTUI {

QTuiDoubleSpinBox::QTuiDoubleSpinBox(QTuiWidget *parent)
    : QTuiAbstractSpinBox(parent)
{
}

double QTuiDoubleSpinBox::value() const
{
    return m_value;
}

void QTuiDoubleSpinBox::setValue(double value)
{
    double clamped = qBound(m_minimum, value, m_maximum);
    if (!qFuzzyCompare(m_value, clamped)) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

double QTuiDoubleSpinBox::minimum() const
{
    return m_minimum;
}

void QTuiDoubleSpinBox::setMinimum(double min)
{
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
}

double QTuiDoubleSpinBox::maximum() const
{
    return m_maximum;
}

void QTuiDoubleSpinBox::setMaximum(double max)
{
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
}

void QTuiDoubleSpinBox::setRange(double min, double max)
{
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value);
}

double QTuiDoubleSpinBox::singleStep() const
{
    return m_singleStep;
}

void QTuiDoubleSpinBox::setSingleStep(double step)
{
    if (step > 0.0)
        m_singleStep = step;
}

int QTuiDoubleSpinBox::decimals() const
{
    return m_decimals;
}

void QTuiDoubleSpinBox::setDecimals(int decimals)
{
    if (decimals >= 0 && m_decimals != decimals) {
        m_decimals = decimals;
        markDirty();
    }
}

void QTuiDoubleSpinBox::stepBy(int steps)
{
    setValue(qBound(m_minimum, m_value + steps * m_singleStep, m_maximum));
}

QString QTuiDoubleSpinBox::valueText() const
{
    return QString::number(m_value, 'f', m_decimals);
}

QSize QTuiDoubleSpinBox::minimumSize() const
{
    return minimumSizeFor(QString::number(m_minimum, 'f', m_decimals),
                          QString::number(m_maximum, 'f', m_decimals));
}

} // namespace QtTUI
