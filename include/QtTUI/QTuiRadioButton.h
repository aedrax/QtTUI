#ifndef QTTUI_QTUIRADIOBUTTON_H
#define QTTUI_QTUIRADIOBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractButton.h>
#include <QString>

namespace QtTUI {

class QTuiRadioButton : public QTuiAbstractButton {
    Q_OBJECT
public:
    explicit QTuiRadioButton(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

protected:
    // A click only checks the button. QTuiButtonGroup unchecks the others.
    void nextCheckState() override;
};

} // namespace QtTUI

#endif // QTTUI_QTUIRADIOBUTTON_H
