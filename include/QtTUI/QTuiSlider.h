#ifndef QTTUI_QTUISLIDER_H
#define QTTUI_QTUISLIDER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractSlider.h>

namespace QtTUI {

class QTuiSlider : public QTuiAbstractSlider {
    Q_OBJECT
public:
    explicit QTuiSlider(Qt::Orientation orientation = Qt::Horizontal, QTuiWidget *parent = nullptr);

    int tickInterval() const;
    void setTickInterval(int interval);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    int m_tickInterval = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUISLIDER_H
