#ifndef QTTUI_QTUIDOUBLESPINBOX_H
#define QTTUI_QTUIDOUBLESPINBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractSpinBox.h>

namespace QtTUI {

class QTuiDoubleSpinBox : public QTuiAbstractSpinBox {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
public:
    explicit QTuiDoubleSpinBox(QTuiWidget *parent = nullptr);

    double value() const;
    void setValue(double value);

    double minimum() const;
    void setMinimum(double min);
    double maximum() const;
    void setMaximum(double max);
    void setRange(double min, double max);

    double singleStep() const;
    void setSingleStep(double step);

    int decimals() const;
    void setDecimals(int decimals);

    void stepBy(int steps) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(double value);

protected:
    QString valueText() const override;

private:

    double m_value = 0.0;
    double m_minimum = 0.0;
    double m_maximum = 99.99;
    double m_singleStep = 1.0;
    int m_decimals = 2;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDOUBLESPINBOX_H
