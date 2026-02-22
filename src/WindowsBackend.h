#ifndef QTTUI_WINDOWSBACKEND_H
#define QTTUI_WINDOWSBACKEND_H

#ifdef _WIN32

#include <QtTUI/TerminalBackend.h>
#include <windows.h>

namespace QtTUI {

class WindowsBackend : public TerminalBackend {
    Q_OBJECT
public:
    explicit WindowsBackend(QObject *parent = nullptr);
    ~WindowsBackend() override;

    bool enterRawMode() override;
    bool exitRawMode() override;
    bool enterAlternateScreen() override;
    bool exitAlternateScreen() override;

    QSize terminalSize() const override;

    qint64 write(const QByteArray &data) override;
    QByteArray read() override;

private:
    HANDLE m_hStdin = INVALID_HANDLE_VALUE;
    HANDLE m_hStdout = INVALID_HANDLE_VALUE;
    DWORD m_originalInMode = 0;
    DWORD m_originalOutMode = 0;
    bool m_rawMode = false;
    bool m_alternateScreen = false;
};

} // namespace QtTUI

#endif // _WIN32
#endif // QTTUI_WINDOWSBACKEND_H
