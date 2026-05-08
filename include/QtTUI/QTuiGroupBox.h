#ifndef QTTUI_QTUIGROUPBOX_H
#define QTTUI_QTUIGROUPBOX_H

#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiLayout;

class QTuiGroupBox : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit QTuiGroupBox(const QString &title = QString(), QTuiWidget *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool isChecked() const;
    void setChecked(bool checked);

    void setLayout(QTuiLayout *layout);
    QTuiLayout *layout() const;

    QRect contentArea() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void toggled(bool checked);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    void updateChildrenEnabled();

    QString m_title;
    bool m_checkable = false;
    bool m_checked = true;
    QTuiLayout *m_layout = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUIGROUPBOX_H
