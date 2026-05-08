#ifndef QTTUI_QTUISLIDER_H
#define QTTUI_QTUISLIDER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

class QTuiSlider : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
public:
    explicit QTuiSlider(Qt::Orientation orientation = Qt::Horizontal, QTuiWidget *parent = nullptr);

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

    int tickInterval() const;
    void setTickInterval(int interval);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(int value);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 99;
    int m_singleStep = 1;
    Qt::Orientation m_orientation;
    int m_tickInterval = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUISLIDER_H
