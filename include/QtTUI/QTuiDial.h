#ifndef QTTUI_QTUIDIAL_H
#define QTTUI_QTUIDIAL_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractSlider.h>

namespace QtTUI {

class QTuiDial : public QTuiAbstractSlider {
    Q_OBJECT
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
public:
    explicit QTuiDial(QTuiWidget *parent = nullptr);

    bool wrapping() const;
    void setWrapping(bool wrapping);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    void stepUp();
    void stepDown();

    bool m_wrapping = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDIAL_H
