#ifndef QTTUI_QTUI_SIZEGRIP_H
#define QTTUI_QTUI_SIZEGRIP_H

#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

class QTuiSizeGrip : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiSizeGrip(QTuiWidget *parent = nullptr);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QTuiWidget *findTopLevelWindow() const;
};

} // namespace QtTUI

#endif // QTTUI_QTUI_SIZEGRIP_H
