#ifndef QTTUI_QTUIPUSHBUTTON_H
#define QTTUI_QTUIPUSHBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiPushButton : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit QTuiPushButton(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    QString icon() const;
    void setIcon(const QString &icon);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool isChecked() const;
    void setChecked(bool checked);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void clicked();
    void toggled(bool checked);
    void textChanged(const QString &text);
    void enabledChanged(bool enabled);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QString m_text;
    QString m_icon;
    bool m_enabled = true;
    bool m_checkable = false;
    bool m_checked = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIPUSHBUTTON_H
