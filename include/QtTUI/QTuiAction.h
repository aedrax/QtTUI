#ifndef QTTUI_QTUIACTION_H
#define QTTUI_QTUIACTION_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiKeySequence.h>
#include <QList>
#include <QObject>
#include <QString>

namespace QtTUI {

class QTuiAction : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString shortcut READ shortcut WRITE setShortcut)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit QTuiAction(QObject *parent = nullptr);
    explicit QTuiAction(const QString &text, QObject *parent = nullptr);
    QTuiAction(const QString &text, const QString &shortcut, QObject *parent = nullptr);
    ~QTuiAction() override;

    QString text() const;
    void setText(const QString &text);

    QString shortcut() const;
    void setShortcut(const QString &shortcut);

    // Parsed form of shortcut(), cached by setShortcut(); invalid (never
    // matching) when the shortcut string is empty or unparseable.
    QTuiKeySequence shortcutSequence() const;

    // Every live QTuiAction, in creation order. QTuiMainWindow walks this
    // list to dispatch global shortcuts (backlog #9).
    static QList<QTuiAction *> registeredActions();

    bool isEnabled() const;
    void setEnabled(bool enabled);

    // A checkable action has an on/off state, as in QAction. trigger()
    // inverts the state first. Menus and tool bars show the state as
    // "[x]" or "[ ]" before the text. Only a checkable action can be
    // checked.
    bool isCheckable() const;
    void setCheckable(bool checkable);
    bool isChecked() const;
    void setChecked(bool checked);

    void trigger();

signals:
    // checked is the new state of a checkable action, else false
    void triggered(bool checked = false);
    void toggled(bool checked);
    void textChanged(const QString &text);
    void enabledChanged(bool enabled);

private:
    // QTuiShortcut sets the sequence directly, without a string
    friend class QTuiShortcut;

    QString m_text;
    QString m_shortcut;
    QTuiKeySequence m_shortcutSequence;
    bool m_enabled = true;
    bool m_checkable = false;
    bool m_checked = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIACTION_H
