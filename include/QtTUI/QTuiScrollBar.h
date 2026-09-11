#ifndef QTTUI_QTUISCROLLBAR_H
#define QTTUI_QTUISCROLLBAR_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractSlider.h>

namespace QtTUI {

class QTuiScrollBar : public QTuiAbstractSlider {
    Q_OBJECT
public:
    explicit QTuiScrollBar(Qt::Orientation orientation, QTuiWidget *parent = nullptr);

    int pageStep() const;
    void setPageStep(int step);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    int m_pageStep = 10;
};

} // namespace QtTUI

#endif // QTTUI_QTUISCROLLBAR_H
