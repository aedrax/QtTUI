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

    virtual qint64 write(const QByteArray &data) = 0;
    virtual QByteArray read() = 0;

signals:
    void resized(QSize newSize);
};

// Factory: creates the platform-appropriate backend at compile time
std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent = nullptr);

} // namespace QtTUI

#endif // QTTUI_TERMINALBACKEND_H
