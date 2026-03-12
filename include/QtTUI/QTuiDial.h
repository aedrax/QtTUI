#ifndef QTTUI_QTUIDIAL_H
#define QTTUI_QTUIDIAL_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

class QTuiDial : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
public:
    explicit QTuiDial(QTuiWidget *parent = nullptr);

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

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void valueChanged(int value);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    void stepUp();
    void stepDown();

    int m_value = 0;
    int m_minimum = 0;
    int m_maximum = 99;
    int m_singleStep = 1;
    bool m_wrapping = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDIAL_H
