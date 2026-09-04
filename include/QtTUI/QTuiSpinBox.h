#ifndef QTTUI_QTUISPINBOX_H
#define QTTUI_QTUISPINBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractSpinBox.h>

namespace QtTUI {

class QTuiSpinBox : public QTuiAbstractSpinBox {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
public:
    explicit QTuiSpinBox(QTuiWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void setRange(int min, int max);

    int singleStep() const;
    void setSingleStep(int step);

    bool wrapping() const;
    void setWrapping(bool wrapping);

    void stepBy(int steps) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(int value);

protected:
    QString valueText() const override;

private:

    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 99;
    int m_singleStep = 1;
    bool m_wrapping = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUISPINBOX_H
