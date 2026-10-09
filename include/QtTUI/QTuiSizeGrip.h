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
    // The arrow keys change the size of the window by one cell
    void keyPressEvent(TuiKeyEvent *event) override;
    // A drag moves the bottom-right corner of the window with the mouse
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    // The window that the grip resizes, as in Qt: the nearest MDI
    // sub-window, dialog or main window that contains the grip
    QTuiWidget *findTopLevelWindow() const;
    void resizeWindow(QTuiWidget *window, const QSize &size);

    QPoint m_pressPos;
    QSize m_pressSize;
};

} // namespace QtTUI

#endif // QTTUI_QTUI_SIZEGRIP_H
