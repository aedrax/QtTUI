#include <QtTUI/QTuiKeySequence.h>
#include <QStringList>

namespace QtTUI {

QTuiKeySequence::QTuiKeySequence(int key, Qt::KeyboardModifiers modifiers)
    : m_key(key)
    , m_modifiers(modifiers)
{
}

// Returns 0 for unrecognized names.
static int parseKeyName(const QString &name)
{
    if (name.isEmpty())
        return 0;

    // Single character: letters and digits (matches InputParser, which
    // reports printable keys as the uppercased code point).
    if (name.length() == 1) {
        const QChar ch = name.at(0);
        if (ch.isLetter() || ch.isDigit())
            return ch.toUpper().unicode();
        return 0;
    }

    const QString lower = name.toLower();

    // Function keys F1-F35
    if (lower.startsWith(QLatin1Char('f'))) {
        bool ok = false;
        const int n = lower.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 35)
            return Qt::Key_F1 + (n - 1);
        return 0;
    }

    if (lower == QLatin1String("escape") || lower == QLatin1String("esc"))
        return Qt::Key_Escape;
    if (lower == QLatin1String("tab"))
        return Qt::Key_Tab;
    if (lower == QLatin1String("backspace"))
        return Qt::Key_Backspace;
    if (lower == QLatin1String("delete") || lower == QLatin1String("del"))
        return Qt::Key_Delete;
    if (lower == QLatin1String("enter") || lower == QLatin1String("return"))
        return Qt::Key_Return;
    if (lower == QLatin1String("space"))
        return Qt::Key_Space;
    if (lower == QLatin1String("home"))
        return Qt::Key_Home;
    if (lower == QLatin1String("end"))
        return Qt::Key_End;
    if (lower == QLatin1String("pgup") || lower == QLatin1String("pageup"))
        return Qt::Key_PageUp;
    if (lower == QLatin1String("pgdown") || lower == QLatin1String("pagedown"))
        return Qt::Key_PageDown;
    if (lower == QLatin1String("insert") || lower == QLatin1String("ins"))
        return Qt::Key_Insert;
    if (lower == QLatin1String("up"))
        return Qt::Key_Up;
    if (lower == QLatin1String("down"))
        return Qt::Key_Down;
    if (lower == QLatin1String("left"))
        return Qt::Key_Left;
    if (lower == QLatin1String("right"))
        return Qt::Key_Right;

    return 0;
}

QTuiKeySequence QTuiKeySequence::fromString(const QString &str)
{
    const QString trimmed = str.trimmed();
    if (trimmed.isEmpty())
        return QTuiKeySequence();

    // Every '+'-separated segment must be non-empty ("Ctrl++" is invalid).
    const QStringList parts = trimmed.split(QLatin1Char('+'));

    Qt::KeyboardModifiers mods = Qt::NoModifier;
    for (int i = 0; i < parts.size() - 1; ++i) {
        const QString mod = parts.at(i).trimmed().toLower();
        if (mod == QLatin1String("ctrl") || mod == QLatin1String("control"))
            mods |= Qt::ControlModifier;
        else if (mod == QLatin1String("alt"))
            mods |= Qt::AltModifier;
        else if (mod == QLatin1String("shift"))
            mods |= Qt::ShiftModifier;
        else if (mod == QLatin1String("meta"))
            mods |= Qt::MetaModifier;
        else
            return QTuiKeySequence(); // unknown modifier -> invalid
    }

    const int key = parseKeyName(parts.last().trimmed());
    if (key == 0)
        return QTuiKeySequence();

    return QTuiKeySequence(key, mods);
}

// Keypad Enter and Return are interchangeable for shortcut purposes.
static int normalizeKey(int key)
{
    return (key == Qt::Key_Enter) ? int(Qt::Key_Return) : key;
}

bool QTuiKeySequence::matches(int key, Qt::KeyboardModifiers modifiers) const
{
    if (!isValid())
        return false;
    return normalizeKey(m_key) == normalizeKey(key) && m_modifiers == modifiers;
}

bool QTuiKeySequence::isBareTextKey() const
{
    if (!isValid())
        return false;
    if (m_modifiers & ~Qt::KeyboardModifiers(Qt::ShiftModifier))
        return false;
    // Printable ASCII range; InputParser reports these keys with the typed
    // character in TuiKeyEvent::text(), so an editor would consume them.
    return m_key >= 0x20 && m_key <= 0x7E;
}

} // namespace QtTUI
