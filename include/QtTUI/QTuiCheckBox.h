#ifndef QTTUI_QTUICHECKBOX_H
#define QTTUI_QTUICHECKBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractButton.h>
#include <QString>

namespace QtTUI {

class QTuiCheckBox : public QTuiAbstractButton {
    Q_OBJECT
    Q_PROPERTY(int checkState READ checkState WRITE setCheckState NOTIFY stateChanged)
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate)
public:
    enum CheckState { Unchecked = 0, PartiallyChecked = 1, Checked = 2 };

    explicit QTuiCheckBox(const QString &text = QString(), QTuiWidget *parent = nullptr);

    int checkState() const;
    void setCheckState(int state);

    bool isTristate() const;
    void setTristate(bool tristate);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void stateChanged(int state);

protected:
    // A click cycles Unchecked, PartiallyChecked, Checked if tristate
    void nextCheckState() override;
    // setChecked() through the base class updates the check state
    void checkStateSet() override;

private:
    int m_checkState = Unchecked;
    bool m_tristate = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUICHECKBOX_H
