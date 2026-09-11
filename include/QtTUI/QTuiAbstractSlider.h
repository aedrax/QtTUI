#ifndef QTTUI_QTUIABSTRACTSLIDER_H
#define QTTUI_QTUIABSTRACTSLIDER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

// Base class of QTuiSlider, QTuiScrollBar and QTuiDial, like
// QAbstractSlider: an integer value in a range, the single step and the
// orientation
class QTuiAbstractSlider : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
public:
    explicit QTuiAbstractSlider(Qt::Orientation orientation, QTuiWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void setRange(int min, int max);

    int singleStep() const;
    void setSingleStep(int step);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

signals:
    void valueChanged(int value);

protected:
    // The value at a screen position along the orientation
    int valueAt(int x, int y) const;

    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 99;
    int m_singleStep = 1;
    Qt::Orientation m_orientation;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTSLIDER_H
