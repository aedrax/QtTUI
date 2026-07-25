#ifndef QTTUI_TERMINALBACKEND_H
#define QTTUI_TERMINALBACKEND_H

#include <QtTUI/QtTUIGlobal.h>
#include <QByteArray>
#include <QObject>
#include <QSize>

#include <memory>

namespace QtTUI {

class TerminalBackend : public QObject {
    Q_OBJECT
public:
    explicit TerminalBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
    ~TerminalBackend() override = default;

    virtual bool enterRawMode() = 0;
    virtual bool exitRawMode() = 0;
    virtual bool enterAlternateScreen() = 0;
    virtual bool exitAlternateScreen() = 0;

    virtual QSize terminalSize() const = 0;

    // Gives the terminal back to the shell and stops the process (job
    // control, as Ctrl+Z does in a normal program). Returns after the
    // process continues, with the terminal modes set again. The default
    // does nothing and returns false.
    virtual bool suspend() { return false; }

    virtual qint64 write(const QByteArray &data) = 0;
    virtual QByteArray read() = 0;

signals:
    void resized(QSize newSize);
};

// The escape sequence that turns on mouse reporting at the level that the
// QTTUI_MOUSE environment variable asks for:
//   off   - no mouse reporting; the terminal keeps its own text selection
//   click - presses, releases and the wheel (?1000)
//   drag  - also motion while a button is down (?1002)
//   all   - also motion without a button, for hover (?1003; the default)
// The backends always turn off all mouse modes when they give the terminal
// back, so the turn-off sequence does not depend on the level.
inline QByteArray mouseEnableSequence()
{
    const QByteArray level = qgetenv("QTTUI_MOUSE").toLower();
    if (level == "off" || level == "none" || level == "0")
        return QByteArray();
    if (level == "click")
        return QByteArrayLiteral("\x1b[?1000h\x1b[?1006h");
    if (level == "drag")
        return QByteArrayLiteral("\x1b[?1002h\x1b[?1006h");
    return QByteArrayLiteral("\x1b[?1003h\x1b[?1006h");
}

// Factory: creates the platform-appropriate backend at compile time
std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent = nullptr);

} // namespace QtTUI

#endif // QTTUI_TERMINALBACKEND_H
