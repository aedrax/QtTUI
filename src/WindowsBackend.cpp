#ifdef _WIN32

#include "WindowsBackend.h"

namespace QtTUI {

WindowsBackend::WindowsBackend(QObject *parent)
    : TerminalBackend(parent)
{
    m_hStdin = GetStdHandle(STD_INPUT_HANDLE);
    m_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

WindowsBackend::~WindowsBackend()
{
    if (m_alternateScreen)
        exitAlternateScreen();
    if (m_rawMode)
        exitRawMode();
}

bool WindowsBackend::enterRawMode()
{
    if (m_rawMode)
        return true;

    if (!GetConsoleMode(m_hStdin, &m_originalInMode)) {
        qWarning("WindowsBackend: failed to get console input mode");
        return false;
    }
    if (!GetConsoleMode(m_hStdout, &m_originalOutMode)) {
        qWarning("WindowsBackend: failed to get console output mode");
        return false;
    }

    DWORD inMode = m_originalInMode;
    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    inMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    DWORD outMode = m_originalOutMode;
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;

    if (!SetConsoleMode(m_hStdin, inMode)) {
        qWarning("WindowsBackend: failed to set console input mode");
        return false;
    }
    if (!SetConsoleMode(m_hStdout, outMode)) {
        SetConsoleMode(m_hStdin, m_originalInMode);
        qWarning("WindowsBackend: failed to set console output mode");
        return false;
    }

    m_rawMode = true;
    return true;
}

bool WindowsBackend::exitRawMode()
{
    if (!m_rawMode)
        return true;

    bool ok = true;
    if (!SetConsoleMode(m_hStdin, m_originalInMode)) {
        qWarning("WindowsBackend: failed to restore console input mode");
        ok = false;
    }
    if (!SetConsoleMode(m_hStdout, m_originalOutMode)) {
        qWarning("WindowsBackend: failed to restore console output mode");
        ok = false;
    }

    m_rawMode = false;
    return ok;
}

bool WindowsBackend::enterAlternateScreen()
{
    if (m_alternateScreen)
        return true;

    const QByteArray seq("\x1b[?1049h");
    DWORD written = 0;
    if (!WriteConsoleA(m_hStdout, seq.constData(), seq.size(), &written, nullptr)) {
        qWarning("WindowsBackend: failed to enter alternate screen");
        return false;
    }

    m_alternateScreen = true;
    return true;
}

bool WindowsBackend::exitAlternateScreen()
{
    if (!m_alternateScreen)
        return true;

    const QByteArray seq("\x1b[?1049l");
    DWORD written = 0;
    if (!WriteConsoleA(m_hStdout, seq.constData(), seq.size(), &written, nullptr)) {
        qWarning("WindowsBackend: failed to exit alternate screen");
        return false;
    }

    m_alternateScreen = false;
    return true;
}

QSize WindowsBackend::terminalSize() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(m_hStdout, &csbi)) {
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return QSize(cols, rows);
    }
    qWarning("WindowsBackend: GetConsoleScreenBufferInfo failed, returning default 80x24");
    return QSize(80, 24);
}

qint64 WindowsBackend::write(const QByteArray &data)
{
    DWORD written = 0;
    if (WriteConsoleA(m_hStdout, data.constData(), data.size(), &written, nullptr))
        return static_cast<qint64>(written);
    return -1;
}

QByteArray WindowsBackend::read()
{
    INPUT_RECORD records[128];
    DWORD numRead = 0;

    if (!GetNumberOfConsoleInputEvents(m_hStdin, &numRead) || numRead == 0)
        return {};

    if (!ReadConsoleInput(m_hStdin, records, 128, &numRead))
        return {};

    QByteArray result;
    for (DWORD i = 0; i < numRead; ++i) {
        if (records[i].EventType == KEY_EVENT && records[i].Event.KeyEvent.bKeyDown) {
            char ch = records[i].Event.KeyEvent.uChar.AsciiChar;
            if (ch != 0)
                result.append(ch);
        } else if (records[i].EventType == WINDOW_BUFFER_SIZE_EVENT) {
            auto &sizeEvent = records[i].Event.WindowBufferSizeEvent;
            emit resized(QSize(sizeEvent.dwSize.X, sizeEvent.dwSize.Y));
        }
    }
    return result;
}

} // namespace QtTUI

#endif // _WIN32
