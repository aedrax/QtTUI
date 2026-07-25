#ifndef _WIN32

#include "PosixBackend.h"

#include <QSocketNotifier>

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace QtTUI {

namespace {

// State shared with async signal handlers. Signal handlers cannot touch the
// Qt machinery (heap, mutexes), so everything they need lives in plain
// file-scope statics.
int g_signalPipe[2] = {-1, -1};
struct termios g_savedTermios;
volatile sig_atomic_t g_rawModeActive = 0;
volatile sig_atomic_t g_altScreenActive = 0;

const int g_fatalSignals[] = {SIGTERM, SIGHUP, SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
const int g_fatalSignalCount = static_cast<int>(sizeof(g_fatalSignals) / sizeof(g_fatalSignals[0]));
struct sigaction g_previousFatalActions[g_fatalSignalCount];
bool g_fatalHandlersInstalled = false;

struct sigaction g_previousWinchAction;
struct sigaction g_previousContAction;

// Async-signal-safe: undo exactly what enterRawMode()/enterAlternateScreen()
// enabled, using only write() and tcsetattr().
void restoreTerminalFromSignal()
{
    if (g_rawModeActive) {
        // Mirror exitRawMode(): disable SGR mouse reporting, re-show the cursor
        static const char disableMouse[] = "\x1b[<u\x1b[?2004l\x1b[?1006l\x1b[?1003l\x1b[?1002l\x1b[?1000l\x1b[?25h";
        ssize_t r = ::write(STDOUT_FILENO, disableMouse, sizeof(disableMouse) - 1);
        (void)r;
    }
    if (g_altScreenActive) {
        // Mirror exitAlternateScreen()
        static const char leaveAltScreen[] = "\x1b[?1049l";
        ssize_t r = ::write(STDOUT_FILENO, leaveAltScreen, sizeof(leaveAltScreen) - 1);
        (void)r;
    }
    if (g_rawModeActive)
        ::tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
    g_rawModeActive = 0;
    g_altScreenActive = 0;
}

void writeToSignalPipe(char byte)
{
    if (g_signalPipe[1] >= 0) {
        ssize_t r = ::write(g_signalPipe[1], &byte, 1);
        (void)r; // a full pipe means a wakeup is already pending
    }
}

} // namespace

PosixBackend *PosixBackend::s_instance = nullptr;

PosixBackend::PosixBackend(QObject *parent)
    : TerminalBackend(parent)
{
    s_instance = this;
    installSignalHandlers();
}

PosixBackend::~PosixBackend()
{
    if (m_alternateScreen)
        exitAlternateScreen();
    if (m_rawMode)
        exitRawMode();
    uninstallSignalHandlers();
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

    if (!applyRawTermios())
        return false;

    g_savedTermios = m_originalTermios;
    g_rawModeActive = 1;
    m_rawMode = true;

    installFatalSignalHandlers();

    return true;
}

bool PosixBackend::applyRawTermios()
{
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

    // Enable SGR mouse reporting at the QTTUI_MOUSE level (default: buttons
    // and any motion), bracketed paste, and the kitty keyboard protocol level 1
    // (CSI u for keys that are otherwise ambiguous). A terminal without
    // the protocol ignores the push, and the pop on exit.
    const QByteArray enableMouse = mouseEnableSequence() + "\x1b[?2004h\x1b[>1u";
    ::write(STDOUT_FILENO, enableMouse.constData(), enableMouse.size());

    return true;
}

bool PosixBackend::exitRawMode()
{
    if (!m_rawMode)
        return true;

    uninstallFatalSignalHandlers();

    // Disable SGR mouse reporting and re-show the cursor (the render pass
    // hides it while frames are written)
    const QByteArray disableMouse("\x1b[<u\x1b[?2004l\x1b[?1006l\x1b[?1003l\x1b[?1002l\x1b[?1000l\x1b[?25h");
    ::write(STDOUT_FILENO, disableMouse.constData(), disableMouse.size());

    g_rawModeActive = 0;

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
    g_altScreenActive = 1;
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
    g_altScreenActive = 0;
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
    const char *buf = data.constData();
    const qint64 size = data.size();
    qint64 total = 0;

    while (total < size) {
        ssize_t n = ::write(STDOUT_FILENO, buf + total, static_cast<size_t>(size - total));
        if (n > 0) {
            total += n;
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // stdout should be blocking, but be safe: wait until writable
            struct pollfd pfd;
            pfd.fd = STDOUT_FILENO;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            if (::poll(&pfd, 1, -1) < 0 && errno != EINTR)
                return total > 0 ? total : -1;
            continue;
        }
        return total > 0 ? total : -1;
    }

    return total;
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
    // Async-signal-safe: just wake the event loop; the actual size query and
    // signal emission happen in onSignalPipeReadable().
    writeToSignalPipe('w');
}

void PosixBackend::handleSigcont(int /*sig*/)
{
    writeToSignalPipe('c');
}

void PosixBackend::handleFatalSignal(int sig)
{
    restoreTerminalFromSignal();

    // Re-install the previous disposition and re-raise so the default
    // behavior (exit status, core dump) is preserved.
    for (int i = 0; i < g_fatalSignalCount; ++i) {
        if (g_fatalSignals[i] == sig) {
            ::sigaction(sig, &g_previousFatalActions[i], nullptr);
            break;
        }
    }
    ::raise(sig);
}

void PosixBackend::installSignalHandlers()
{
    // Self-pipe: the only thing the SIGWINCH/SIGCONT handlers do is write one
    // byte here; the notifier on the read end does the real work on the
    // event loop.
    if (g_signalPipe[0] < 0) {
        if (::pipe(g_signalPipe) == 0) {
            for (int fd : g_signalPipe) {
                ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
                ::fcntl(fd, F_SETFD, FD_CLOEXEC);
            }
        } else {
            g_signalPipe[0] = g_signalPipe[1] = -1;
            qWarning("PosixBackend: failed to create signal pipe");
        }
    }

    if (g_signalPipe[0] >= 0) {
        m_signalNotifier = new QSocketNotifier(g_signalPipe[0], QSocketNotifier::Read, this);
        connect(m_signalNotifier, &QSocketNotifier::activated,
                this, &PosixBackend::onSignalPipeReadable);
    }

    struct sigaction sa;
    sa.sa_handler = &PosixBackend::handleSigwinch;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    ::sigaction(SIGWINCH, &sa, &g_previousWinchAction);

    sa.sa_handler = &PosixBackend::handleSigcont;
    ::sigaction(SIGCONT, &sa, &g_previousContAction);
}

void PosixBackend::uninstallSignalHandlers()
{
    ::sigaction(SIGWINCH, &g_previousWinchAction, nullptr);
    ::sigaction(SIGCONT, &g_previousContAction, nullptr);

    delete m_signalNotifier;
    m_signalNotifier = nullptr;

    if (g_signalPipe[0] >= 0) {
        ::close(g_signalPipe[0]);
        ::close(g_signalPipe[1]);
        g_signalPipe[0] = g_signalPipe[1] = -1;
    }
}

void PosixBackend::installFatalSignalHandlers()
{
    if (g_fatalHandlersInstalled)
        return;

    struct sigaction sa;
    sa.sa_handler = &PosixBackend::handleFatalSignal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    for (int i = 0; i < g_fatalSignalCount; ++i)
        ::sigaction(g_fatalSignals[i], &sa, &g_previousFatalActions[i]);

    g_fatalHandlersInstalled = true;
}

void PosixBackend::uninstallFatalSignalHandlers()
{
    if (!g_fatalHandlersInstalled)
        return;

    for (int i = 0; i < g_fatalSignalCount; ++i)
        ::sigaction(g_fatalSignals[i], &g_previousFatalActions[i], nullptr);

    g_fatalHandlersInstalled = false;
}

void PosixBackend::onSignalPipeReadable()
{
    // Drain everything queued so a burst of signals produces one update.
    char buf[64];
    bool winch = false;
    bool cont = false;
    for (;;) {
        ssize_t n = ::read(g_signalPipe[0], buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i) {
                if (buf[i] == 'w')
                    winch = true;
                else if (buf[i] == 'c')
                    cont = true;
            }
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        break;
    }

    if (cont)
        reestablishAfterContinue(); // emits resized() itself
    else if (winch)
        emit resized(terminalSize());
}

bool PosixBackend::suspend()
{
    if (!m_rawMode && !m_alternateScreen)
        return false;

    // Undo the terminal modes, but keep m_rawMode and m_alternateScreen:
    // reestablishAfterContinue() uses them to set the modes again
    static const char restore[] = "\x1b[<u\x1b[?2004l\x1b[?1006l\x1b[?1003l\x1b[?1002l\x1b[?1000l\x1b[?25h\x1b[?1049l";
    ::write(STDOUT_FILENO, restore, sizeof(restore) - 1);
    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_originalTermios);
    g_rawModeActive = 0;
    g_altScreenActive = 0;

    // Stop the process group, as the terminal driver does for Ctrl+Z.
    // kill() returns after SIGCONT.
    ::kill(0, SIGTSTP);

    // If SIGTSTP is ignored, no SIGCONT arrives, so set the modes again
    // here. The SIGCONT path does the same work again, which is harmless.
    g_altScreenActive = m_alternateScreen ? 1 : 0;
    reestablishAfterContinue();
    return true;
}

void PosixBackend::reestablishAfterContinue()
{
    // The shell usually resets the terminal while we are stopped; re-apply
    // whatever we had enabled, from the state we already track.
    if (m_rawMode) {
        applyRawTermios();
        g_rawModeActive = 1;
    }
    if (m_alternateScreen) {
        const QByteArray seq("\x1b[?1049h");
        ::write(STDOUT_FILENO, seq.constData(), seq.size());
    }

    // Force a relayout + full repaint through the normal resize path.
    emit resized(terminalSize());
}

} // namespace QtTUI

#endif // !_WIN32
