#ifndef QTTUI_QTUIBUTTONGROUP_H
#define QTTUI_QTUIBUTTONGROUP_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QList>

namespace QtTUI {

class QTuiWidget;

class QTuiButtonGroup : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool exclusive READ exclusive WRITE setExclusive)
public:
    explicit QTuiButtonGroup(QObject *parent = nullptr);

    void addButton(QTuiWidget *button, int id = -1);
    void removeButton(QTuiWidget *button);
    QList<QTuiWidget *> buttons() const;

    int id(QTuiWidget *button) const;
    QTuiWidget *button(int id) const;
    QTuiWidget *checkedButton() const;
    int checkedId() const;

    bool exclusive() const;
    void setExclusive(bool exclusive);

signals:
    void buttonClicked(int id);
    void buttonToggled(int id, bool checked);

private slots:
    void onButtonToggled(bool checked);
    void onButtonClicked();
    void onCheckBoxStateChanged(int state);

private:
    struct ButtonEntry {
        QTuiWidget *button;
        int id;
    };

    int findId(QTuiWidget *button) const;
    void handleToggle(QTuiWidget *button, bool checked);

    QList<ButtonEntry> m_buttons;
    bool m_exclusive = true;
    int m_nextId = -2;
};

} // namespace QtTUI

#endif // QTTUI_QTUIBUTTONGROUP_H
