#include <QtTUI/QTuiUndoStack.h>

namespace QtTUI {

// --- QTuiUndoCommand ---

QTuiUndoCommand::QTuiUndoCommand(const QString &text)
    : m_text(text)
{
}

QTuiUndoCommand::~QTuiUndoCommand() = default;

void QTuiUndoCommand::undo() {}
void QTuiUndoCommand::redo() {}

QString QTuiUndoCommand::text() const { return m_text; }
void QTuiUndoCommand::setText(const QString &text) { m_text = text; }

// --- QTuiUndoStack ---

QTuiUndoStack::QTuiUndoStack(QObject *parent)
    : QObject(parent)
{
}

QTuiUndoStack::~QTuiUndoStack()
{
    qDeleteAll(m_commands);
}

void QTuiUndoStack::push(QTuiUndoCommand *cmd)
{
    // Remove any commands above current index (discard redo history)
    while (m_commands.size() > m_index)
        delete m_commands.takeLast();

    m_commands.append(cmd);
    cmd->redo();
    m_index = m_commands.size();

    emit indexChanged(m_index);
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
}

void QTuiUndoStack::undo()
{
    if (!canUndo())
        return;

    --m_index;
    m_commands.at(m_index)->undo();

    emit indexChanged(m_index);
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
}

void QTuiUndoStack::redo()
{
    if (!canRedo())
        return;

    m_commands.at(m_index)->redo();
    ++m_index;

    emit indexChanged(m_index);
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
}

int QTuiUndoStack::count() const { return m_commands.size(); }
int QTuiUndoStack::index() const { return m_index; }

void QTuiUndoStack::setIndex(int idx)
{
    if (idx < 0) idx = 0;
    if (idx > m_commands.size()) idx = m_commands.size();
    if (idx == m_index) return;

    while (m_index > idx) {
        --m_index;
        m_commands.at(m_index)->undo();
    }
    while (m_index < idx) {
        m_commands.at(m_index)->redo();
        ++m_index;
    }

    emit indexChanged(m_index);
    emit canUndoChanged(canUndo());
    emit canRedoChanged(canRedo());
}

QString QTuiUndoStack::text(int idx) const
{
    if (idx < 0 || idx >= m_commands.size())
        return QString();
    return m_commands.at(idx)->text();
}

bool QTuiUndoStack::canUndo() const { return m_index > 0; }
bool QTuiUndoStack::canRedo() const { return m_index < m_commands.size(); }

void QTuiUndoStack::clear()
{
    qDeleteAll(m_commands);
    m_commands.clear();
    m_index = 0;

    emit indexChanged(m_index);
    emit canUndoChanged(false);
    emit canRedoChanged(false);
}

} // namespace QtTUI
