#ifndef QTTUI_QTUITOOLBUTTON_H
#define QTTUI_QTUITOOLBUTTON_H

#include <QtTUI/QTuiPushButton.h>

namespace QtTUI {

class QTuiMenu; // Forward declaration - QTuiMenu doesn't exist yet

class QTuiToolButton : public QTuiPushButton {
    Q_OBJECT
public:
    explicit QTuiToolButton(const QString &text = QString(), QTuiWidget *parent = nullptr);

    void setMenu(QTuiMenu *menu);
    QTuiMenu *menu() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QTuiMenu *m_menu = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUITOOLBUTTON_H
