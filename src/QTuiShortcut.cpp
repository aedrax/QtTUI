#include <QtTUI/QTuiShortcut.h>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

// The action is a child of the widget, not of the shortcut: the shortcut
// dispatch reads the owner widget from the parent of the action.
QTuiShortcut::QTuiShortcut(const QTuiKeySequence &key, QTuiWidget *parent)
    : QObject(parent)
    , m_action(new QTuiAction(parent))
{
    m_action->m_shortcutSequence = key;
    connect(m_action.data(), &QTuiAction::triggered, this, &QTuiShortcut::activated);
}

QTuiShortcut::~QTuiShortcut()
{
    // Null if the parent widget deleted the action first
    delete m_action.data();
}

QTuiKeySequence QTuiShortcut::key() const
{
    return m_action ? m_action->shortcutSequence() : QTuiKeySequence();
}

void QTuiShortcut::setKey(const QTuiKeySequence &key)
{
    if (m_action)
        m_action->m_shortcutSequence = key;
}

bool QTuiShortcut::isEnabled() const
{
    return m_action && m_action->isEnabled();
}

void QTuiShortcut::setEnabled(bool enabled)
{
    if (m_action)
        m_action->setEnabled(enabled);
}

} // namespace QtTUI
