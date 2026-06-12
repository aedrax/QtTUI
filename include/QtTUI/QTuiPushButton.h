#ifndef QTTUI_QTUIPUSHBUTTON_H
#define QTTUI_QTUIPUSHBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractButton.h>
#include <QString>

namespace QtTUI {

// Bordered command button. Text, checkable state, the pressed look and
// click handling come from QTuiAbstractButton.
class QTuiPushButton : public QTuiAbstractButton {
    Q_OBJECT
public:
    explicit QTuiPushButton(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString icon() const;
    void setIcon(const QString &icon);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

private:
    QString m_icon;
};

} // namespace QtTUI

#endif // QTTUI_QTUIPUSHBUTTON_H
