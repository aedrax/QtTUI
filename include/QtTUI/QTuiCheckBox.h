#ifndef QTTUI_QTUICHECKBOX_H
#define QTTUI_QTUICHECKBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiCheckBox : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(int checkState READ checkState WRITE setCheckState NOTIFY stateChanged)
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate)
public:
    enum CheckState { Unchecked = 0, PartiallyChecked = 1, Checked = 2 };

    explicit QTuiCheckBox(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    int checkState() const;
    void setCheckState(int state);

    bool isChecked() const;
    void setChecked(bool checked);

    bool isTristate() const;
    void setTristate(bool tristate);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void stateChanged(int state);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QString m_text;
    int m_checkState = Unchecked;
    bool m_tristate = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUICHECKBOX_H
