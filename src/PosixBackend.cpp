#ifndef _WIN32

#include "PosixBackend.h"

#include <QCoreApplication>
#include <QSocketNotifier>

#include <csignal>
#include <sys/ioctl.h>
#include <unistd.h>

namespace QtTUI {

PosixBackend *PosixBackend::s_instance = nullptr;

PosixBackend::PosixBackend(QObject *parent)
    : TerminalBackend(parent)
{
    s_instance = this;
    installSigwinchHandler();
}

PosixBackend::~PosixBackend()
{
    if (m_alternateScreen)
        exitAlternateScreen();
    if (m_rawMode)
        exitRawMode();
    if (s_instance == this)
        s_instance = nullptr;
}

bool PosixBackend::enterRawMode()
{
    if (m_rawMode)
        return true;

    if (::tcgetattr(STDIN_FILENO, &m_originalTermios) != 0) {
        qWarning("PosixBackend: failed to get terminal attributes");
        return false;
    }

    struct termios raw = m_originalTermios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        qWarning("PosixBackend: failed to set raw mode");
        return false;
    }

    m_rawMode = true;
    return true;
}

bool PosixBackend::exitRawMode()
{
    if (!m_rawMode)
        return true;

    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_originalTermios) != 0) {
        qWarning("PosixBackend: failed to restore terminal attributes");
        return false;
    }

    m_rawMode = false;
    return true;
}

bool PosixBackend::enterAlternateScreen()
{
    if (m_alternateScreen)
        return true;

    const QByteArray seq("\x1b[?1049h");
    if (::write(STDOUT_FILENO, seq.constData(), seq.size()) < 0) {
        qWarning("PosixBackend: failed to enter alternate screen");
        return false;
    }

    m_alternateScreen = true;
    return true;
}

bool PosixBackend::exitAlternateScreen()
{
    if (!m_alternateScreen)
        return true;

    const QByteArray seq("\x1b[?1049l");
    if (::write(STDOUT_FILENO, seq.constData(), seq.size()) < 0) {
        qWarning("PosixBackend: failed to exit alternate screen");
        return false;
    }

    m_alternateScreen = false;
    return true;
}

QSize PosixBackend::terminalSize() const
{
    struct winsize ws;
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        return QSize(ws.ws_col, ws.ws_row);
    }
    qWarning("PosixBackend: ioctl TIOCGWINSZ failed, returning default 80x24");
    return QSize(80, 24);
}

qint64 PosixBackend::write(const QByteArray &data)
{
    return ::write(STDOUT_FILENO, data.constData(), data.size());
}

QByteArray PosixBackend::read()
{
    char buf[4096];
    ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0)
        return QByteArray(buf, static_cast<int>(n));
    return {};
}

void PosixBackend::handleSigwinch(int /*sig*/)
{
    if (s_instance) {
        QSize size = s_instance->terminalSize();
        QMetaObject::invokeMethod(s_instance, [s_instance = s_instance, size]() {
            emit s_instance->resized(size);
        }, Qt::QueuedConnection);
    }
}

void PosixBackend::installSigwinchHandler()
{
    struct sigaction sa;
    sa.sa_handler = &PosixBackend::handleSigwinch;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    ::sigaction(SIGWINCH, &sa, nullptr);
}

} // namespace QtTUI

#endif // !_WIN32
