#ifndef QTTUI_TUIEVENTS_H
#define QTTUI_TUIEVENTS_H

#include <QtTUI/QtTUIGlobal.h>
#include <QEvent>
#include <QPoint>
#include <QSize>
#include <QString>
#include <Qt>

namespace QtTUI {

// Custom event type IDs (registered at runtime)
class TuiEventTypes {
public:
    static QEvent::Type keyEventType();
    static QEvent::Type mouseEventType();
    static QEvent::Type resizeEventType();
    static QEvent::Type pasteEventType();
};

// Lightweight key event for TUI (no Qt::Gui dependency)
class TuiKeyEvent : public QEvent {
public:
    TuiKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text = QString())
        : QEvent(TuiEventTypes::keyEventType())
        , m_key(key)
        , m_modifiers(modifiers)
        , m_text(text)
    {
    }

    int key() const { return m_key; }
    Qt::KeyboardModifiers modifiers() const { return m_modifiers; }
    QString text() const { return m_text; }

private:
    int m_key;
    Qt::KeyboardModifiers m_modifiers;
    QString m_text;
};

// Lightweight mouse event for TUI (no Qt::Gui dependency)
class TuiMouseEvent : public QEvent {
public:
    enum MouseEventType { Press, Release, Move, Wheel };

    TuiMouseEvent(MouseEventType mouseType, Qt::MouseButton button, const QPoint &pos,
                  Qt::KeyboardModifiers modifiers = Qt::NoModifier, int wheelDelta = 0)
        : QEvent(TuiEventTypes::mouseEventType())
        , m_mouseType(mouseType)
        , m_button(button)
        , m_pos(pos)
        , m_modifiers(modifiers)
        , m_wheelDelta(wheelDelta)
    {
    }

    MouseEventType mouseType() const { return m_mouseType; }
    Qt::MouseButton button() const { return m_button; }
    QPoint pos() const { return m_pos; }
    int x() const { return m_pos.x(); }
    int y() const { return m_pos.y(); }
    Qt::KeyboardModifiers modifiers() const { return m_modifiers; }
    // Wheel events only: +120 per notch away from the user, -120 toward (Qt convention)
    int wheelDelta() const { return m_wheelDelta; }

private:
    MouseEventType m_mouseType;
    Qt::MouseButton m_button;
    QPoint m_pos;
    Qt::KeyboardModifiers m_modifiers;
    int m_wheelDelta;
};

// Lightweight resize event for TUI (no Qt::Gui dependency)
class TuiResizeEvent : public QEvent {
public:
    TuiResizeEvent(const QSize &size, const QSize &oldSize = QSize())
        : QEvent(TuiEventTypes::resizeEventType())
        , m_size(size)
        , m_oldSize(oldSize)
    {
    }

    QSize size() const { return m_size; }
    QSize oldSize() const { return m_oldSize; }

private:
    QSize m_size;
    QSize m_oldSize;
};

// Text pasted into the terminal, received as one block through bracketed
// paste mode. Line endings are "\n". A pasted newline is text, not an
// Enter key, so a paste cannot submit or activate anything.
class TuiPasteEvent : public QEvent {
public:
    explicit TuiPasteEvent(const QString &text)
        : QEvent(TuiEventTypes::pasteEventType())
        , m_text(text)
    {
    }

    QString text() const { return m_text; }

private:
    QString m_text;
};

} // namespace QtTUI

#endif // QTTUI_TUIEVENTS_H
