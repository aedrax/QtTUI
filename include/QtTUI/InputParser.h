#ifndef QTTUI_INPUTPARSER_H
#define QTTUI_INPUTPARSER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/TuiEvents.h>
#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QTimer>

namespace QtTUI {

class InputParser : public QObject {
    Q_OBJECT
public:
    explicit InputParser(QObject *eventTarget, QObject *parent = nullptr);

    void processInput(const QByteArray &rawBytes);
    void processResize(const QSize &newSize);

    void setEscapeTimeout(int ms);
    int escapeTimeout() const;

    void setTerminalSize(const QSize &size);
    QSize terminalSize() const;

signals:
    void keyEventParsed(int key, Qt::KeyboardModifiers modifiers, const QString &text);
    void mouseEventParsed(int button, int x, int y, int eventType);
    void resizeEventParsed(int cols, int rows);

private:
    enum class State { Ground, Escape, CSI, CSIParam, SGRMouse, Utf8 };

    void processOneByte(char byte);
    void handleGround(char byte);
    void handleEscape(char byte);
    void handleCSI(char byte);
    void handleCSIParam(char byte);
    void handleSGRMouse(char byte);
    void handleUtf8(char byte);

    void finishCSI(char finalByte);
    void finishSGRMouse(char finalByte);
    void discardBuffer();

    void postKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text = QString());
    void postMouseEvent(TuiMouseEvent::MouseEventType type, Qt::MouseButton button, const QPoint &pos,
                        Qt::KeyboardModifiers modifiers = Qt::NoModifier, int wheelDelta = 0);
    void postResizeEvent(const QSize &size);

    void onEscapeTimeout();

    int clampX(int x) const;
    int clampY(int y) const;

    QObject *m_eventTarget;
    State m_state = State::Ground;
    QByteArray m_buffer;
    QSize m_terminalSize = QSize(80, 24);
    int m_escapeTimeout = 50;
    int m_utf8Expected = 0;
    QTimer m_escapeTimer;
};

} // namespace QtTUI

#endif // QTTUI_INPUTPARSER_H
