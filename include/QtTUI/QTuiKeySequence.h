#ifndef QTTUI_QTUIKEYSEQUENCE_H
#define QTTUI_QTUIKEYSEQUENCE_H

#include <QtTUI/QtTUIGlobal.h>
#include <QString>
#include <Qt>

namespace QtTUI {

// Minimal single-chord key sequence for shortcut dispatch (QtGui's
// QKeySequence is unavailable in a Core-only build).
//
// Grammar accepted by fromString(): zero or more '+'-separated modifier names
// followed by exactly one key name, e.g. "Ctrl+S", "Ctrl+Shift+Z", "Alt+F4",
// "F5", "Q". All names are case-insensitive; surrounding whitespace per
// segment is ignored.
//
//   Modifiers: Ctrl/Control, Alt, Shift, Meta
//   Keys:      single letters A-Z, digits 0-9, F1-F35, and the named keys
//              Escape/Esc, Tab, Backspace, Delete/Del, Enter/Return, Space,
//              Home, End, PgUp/PageUp, PgDown/PageDown, Insert/Ins,
//              Up, Down, Left, Right
//
// Anything else parses to an invalid sequence, which never matches.
class QTuiKeySequence {
public:
    QTuiKeySequence() = default;
    QTuiKeySequence(int key, Qt::KeyboardModifiers modifiers);

    static QTuiKeySequence fromString(const QString &str);

    bool isValid() const { return m_key != 0; }
    int key() const { return m_key; }
    Qt::KeyboardModifiers modifiers() const { return m_modifiers; }

    // Exact-modifier match: Ctrl+S does not match Ctrl+Shift+S. Key_Enter
    // (keypad) is treated as Key_Return so "Enter"/"Return" match both.
    bool matches(int key, Qt::KeyboardModifiers modifiers) const;

    // True if the key on its own would insert text when typed (letters,
    // digits, space, punctuation) and the sequence has no modifier beyond
    // Shift. Such shortcuts must not fire while a text-editing widget has
    // focus (see QTuiMainWindow's shortcut dispatch).
    bool isBareTextKey() const;

    bool operator==(const QTuiKeySequence &other) const
    {
        return m_key == other.m_key && m_modifiers == other.m_modifiers;
    }

private:
    int m_key = 0; // 0 = invalid/empty
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;
};

} // namespace QtTUI

#endif // QTTUI_QTUIKEYSEQUENCE_H
