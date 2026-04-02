#include <QtTUI/QTuiAction.h>

namespace QtTUI {

QTuiAction::QTuiAction(QObject *parent)
    : QObject(parent)
{
}

QTuiAction::QTuiAction(const QString &text, QObject *parent)
    : QObject(parent)
    , m_text(text)
{
}

QTuiAction::QTuiAction(const QString &text, const QString &shortcut, QObject *parent)
    : QObject(parent)
    , m_text(text)
    , m_shortcut(shortcut)
{
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
