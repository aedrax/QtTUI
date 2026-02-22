#include "MockTerminalBackend.h"

namespace QtTUI {

MockTerminalBackend::MockTerminalBackend(QObject *parent)
    : TerminalBackend(parent)
{
}

bool MockTerminalBackend::enterRawMode()
{
    m_rawMode = true;
    return true;
}

bool MockTerminalBackend::exitRawMode()
{
    m_rawMode = false;
    return true;
}

bool MockTerminalBackend::enterAlternateScreen()
{
    m_alternateScreen = true;
    return true;
}

bool MockTerminalBackend::exitAlternateScreen()
{
    m_alternateScreen = false;
    return true;
}

QSize MockTerminalBackend::terminalSize() const
{
    return m_size;
}

qint64 MockTerminalBackend::write(const QByteArray &data)
{
    m_outputBuffer.append(data);
    return data.size();
}

QByteArray MockTerminalBackend::read()
{
    QByteArray data = m_inputBuffer;
    m_inputBuffer.clear();
    return data;
}

void MockTerminalBackend::setTerminalSize(const QSize &size)
{
    m_size = size;
}

void MockTerminalBackend::emitResized(const QSize &size)
{
    m_size = size;
    emit resized(size);
}

void MockTerminalBackend::feedInput(const QByteArray &data)
{
    m_inputBuffer.append(data);
}

QByteArray MockTerminalBackend::writtenData() const
{
    return m_outputBuffer;
}

void MockTerminalBackend::clearWrittenData()
{
    m_outputBuffer.clear();
}

bool MockTerminalBackend::isRawMode() const
{
    return m_rawMode;
}

bool MockTerminalBackend::isAlternateScreen() const
{
    return m_alternateScreen;
}

} // namespace QtTUI
