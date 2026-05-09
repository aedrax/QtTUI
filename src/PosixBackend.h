#ifndef QTTUI_POSIXBACKEND_H
#define QTTUI_POSIXBACKEND_H

#ifndef _WIN32

#include <QtTUI/TerminalBackend.h>
#include <termios.h>

class QSocketNotifier;

namespace QtTUI {

class PosixBackend : public TerminalBackend {
    Q_OBJECT
public:
    explicit PosixBackend(QObject *parent = nullptr);
    ~PosixBackend() override;

    bool enterRawMode() override;
    bool exitRawMode() override;
    bool enterAlternateScreen() override;
    bool exitAlternateScreen() override;

    QSize terminalSize() const override;

    qint64 write(const QByteArray &data) override;
    QByteArray read() override;

private:
    static void handleSigwinch(int sig);
    static void handleSigcont(int sig);
    static void handleFatalSignal(int sig);

    void installSignalHandlers();
    void uninstallSignalHandlers();
    void installFatalSignalHandlers();
    void uninstallFatalSignalHandlers();
    void onSignalPipeReadable();
    void reestablishAfterContinue();
    bool applyRawTermios();

    struct termios m_originalTermios;
    bool m_rawMode = false;
    bool m_alternateScreen = false;
    QSocketNotifier *m_signalNotifier = nullptr;

    static PosixBackend *s_instance;
};

} // namespace QtTUI

#endif // !_WIN32
#endif // QTTUI_POSIXBACKEND_H
