#include <QtTUI/QTuiAction.h>
#include <QPointer>

namespace QtTUI {

// Global action registry for shortcut dispatch. QPointer entries guard
// against actions destroyed without running ~QTuiAction (never expected, but
// cheap); the destructor removes entries eagerly.
static QList<QPointer<QTuiAction>> &actionRegistry()
{
    static QList<QPointer<QTuiAction>> registry;
    return registry;
}

QTuiAction::QTuiAction(QObject *parent)
    : QObject(parent)
{
    actionRegistry().append(this);
}

QTuiAction::QTuiAction(const QString &text, QObject *parent)
    : QObject(parent)
    , m_text(text)
{
    actionRegistry().append(this);
}

QTuiAction::QTuiAction(const QString &text, const QString &shortcut, QObject *parent)
    : QObject(parent)
    , m_text(text)
    , m_shortcut(shortcut)
    , m_shortcutSequence(QTuiKeySequence::fromString(shortcut))
{
    actionRegistry().append(this);
}

QTuiAction::~QTuiAction()
{
    actionRegistry().removeAll(this);
}

QList<QTuiAction *> QTuiAction::registeredActions()
{
    QList<QTuiAction *> result;
    result.reserve(actionRegistry().size());
    for (const auto &ptr : actionRegistry()) {
        if (ptr)
            result.append(ptr.data());
    }
    return result;
}

QString QTuiAction::text() const
{
    return m_text;
}

void QTuiAction::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged(m_text);
    }
}

QString QTuiAction::shortcut() const
{
    return m_shortcut;
}

void QTuiAction::setShortcut(const QString &shortcut)
{
    m_shortcut = shortcut;
    m_shortcutSequence = QTuiKeySequence::fromString(shortcut);
}

QTuiKeySequence QTuiAction::shortcutSequence() const
{
    return m_shortcutSequence;
}

bool QTuiAction::isEnabled() const
{
    return m_enabled;
}

void QTuiAction::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(m_enabled);
    }
}

void QTuiAction::trigger()
{
    if (m_enabled)
        emit triggered();
}

} // namespace QtTUI
