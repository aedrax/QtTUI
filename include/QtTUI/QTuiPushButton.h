#ifndef QTTUI_QTUIPUSHBUTTON_H
#define QTTUI_QTUIPUSHBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

class QTimer;

namespace QtTUI {

class QTuiPushButton : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit QTuiPushButton(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    QString icon() const;
    void setIcon(const QString &icon);

    // Enabled state (isEnabled/setEnabled and the enabledChanged signal) is
    // inherited from QTuiWidget.

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool isChecked() const;
    void setChecked(bool checked);

    // Pressed look. Terminals do not report key releases, so a click shows
    // the pressed look for a short time (see animateClick()).
    bool isDown() const;
    void setDown(bool down);

    // Shows the pressed look, emits clicked() (and toggles a checkable
    // button), then releases the button after a short time.
    void animateClick();

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void clicked();
    void toggled(bool checked);
    void textChanged(const QString &text);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    QString m_text;
    QString m_icon;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_down = false;
    QTimer *m_releaseTimer;
};

} // namespace QtTUI

#endif // QTTUI_QTUIPUSHBUTTON_H
