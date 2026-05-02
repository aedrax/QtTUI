#ifndef QTTUI_QTUIRADIOBUTTON_H
#define QTTUI_QTUIRADIOBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiRadioButton : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit QTuiRadioButton(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    bool isChecked() const;
    void setChecked(bool checked);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void toggled(bool checked);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    QString m_text;
    bool m_checked = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIRADIOBUTTON_H
