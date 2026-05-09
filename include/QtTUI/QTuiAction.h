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

    void trigger();

signals:
    void triggered();
    void textChanged(const QString &text);
    void enabledChanged(bool enabled);

private:
    QString m_text;
    QString m_shortcut;
    QTuiKeySequence m_shortcutSequence;
    bool m_enabled = true;
};

} // namespace QtTUI

#endif // QTTUI_QTUIACTION_H
