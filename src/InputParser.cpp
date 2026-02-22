#include <QtTUI/InputParser.h>
#include <QCoreApplication>
#include <QDebug>

namespace QtTUI {

InputParser::InputParser(QObject *eventTarget, QObject *parent)
    : QObject(parent)
    , m_eventTarget(eventTarget)
{
}

void InputParser::processInput(const QByteArray &rawBytes)
{
    for (int i = 0; i < rawBytes.size(); ++i) {
        processOneByte(rawBytes[i]);
    }
}

void InputParser::processResize(const QSize &newSize)
{
    postResizeEvent(newSize);
}

void InputParser::setEscapeTimeout(int ms)
{
    m_escapeTimeout = ms;
}

int InputParser::escapeTimeout() const
{
    return m_escapeTimeout;
}

void InputParser::setTerminalSize(const QSize &size)
{
    m_terminalSize = size;
}

QSize InputParser::terminalSize() const
{
    return m_terminalSize;
}

void InputParser::processOneByte(char byte)
{
    switch (m_state) {
    case State::Ground:  handleGround(byte);    break;
    case State::Escape:  handleEscape(byte);    break;
    case State::CSI:     handleCSI(byte);       break;
    case State::CSIParam:handleCSIParam(byte);  break;
    case State::SGRMouse:handleSGRMouse(byte);  break;
    case State::Utf8:    handleUtf8(byte);      break;
    }
}

void InputParser::handleGround(char byte)
{
    auto ubyte = static_cast<unsigned char>(byte);

    // ESC starts an escape sequence
    if (byte == '\x1b') {
        m_state = State::Escape;
        m_buffer.clear();
        return;
    }

    // Ctrl+letter: 0x01-0x1A (except special cases)
    if (ubyte >= 0x01 && ubyte <= 0x1A) {
        if (ubyte == '\t') {
            postKeyEvent(Qt::Key_Tab, Qt::NoModifier, QStringLiteral("\t"));
        } else if (ubyte == '\r' || ubyte == '\n') {
            postKeyEvent(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        } else if (ubyte == 0x08) {
            postKeyEvent(Qt::Key_Backspace, Qt::NoModifier);
        } else {
            int key = Qt::Key_A + (ubyte - 0x01);
            postKeyEvent(key, Qt::ControlModifier);
        }
        return;
    }

    // DEL = Backspace
    if (ubyte == 0x7F) {
        postKeyEvent(Qt::Key_Backspace, Qt::NoModifier);
        return;
    }

    // Printable ASCII
    if (ubyte >= 0x20 && ubyte <= 0x7E) {
        QChar ch(byte);
        int key = ch.toUpper().unicode();
        Qt::KeyboardModifiers mods = Qt::NoModifier;
        if (ch.isUpper())
            mods |= Qt::ShiftModifier;
        postKeyEvent(key, mods, QString(ch));
        return;
    }

    // UTF-8 leading byte
    if (ubyte >= 0xC0) {
        m_buffer.clear();
        m_buffer.append(byte);
        if ((ubyte & 0xE0) == 0xC0)      m_utf8Expected = 2;
        else if ((ubyte & 0xF0) == 0xE0)  m_utf8Expected = 3;
        else if ((ubyte & 0xF8) == 0xF0)  m_utf8Expected = 4;
        else {
            postKeyEvent(Qt::Key_unknown, Qt::NoModifier, QString(QChar(0xFFFD)));
            return;
        }
        m_state = State::Utf8;
        return;
    }

    // Stray continuation byte
    if ((ubyte & 0xC0) == 0x80) {
        postKeyEvent(Qt::Key_unknown, Qt::NoModifier, QString(QChar(0xFFFD)));
        return;
    }
}

void InputParser::handleUtf8(char byte)
{
    auto ubyte = static_cast<unsigned char>(byte);

    // Must be a continuation byte
    if ((ubyte & 0xC0) != 0x80) {
        // Invalid: emit replacement for what we had, re-process this byte
        postKeyEvent(Qt::Key_unknown, Qt::NoModifier, QString(QChar(0xFFFD)));
        m_buffer.clear();
        m_state = State::Ground;
        processOneByte(byte);
        return;
    }

    m_buffer.append(byte);

    if (m_buffer.size() == m_utf8Expected) {
        QString decoded = QString::fromUtf8(m_buffer);
        m_buffer.clear();
        m_state = State::Ground;
        if (decoded.isEmpty() || decoded[0] == QChar(0xFFFD)) {
            postKeyEvent(Qt::Key_unknown, Qt::NoModifier, QString(QChar(0xFFFD)));
        } else {
            QChar ch = decoded[0];
            postKeyEvent(ch.unicode(), Qt::NoModifier, decoded);
        }
    }
}

void InputParser::handleEscape(char byte)
{
    if (byte == '[') {
        m_state = State::CSI;
        m_buffer.clear();
        return;
    }

    // SS3: ESC O followed by P/Q/R/S for F1-F4
    if (byte == 'O') {
        // Transition to a state that reads one more byte for SS3
        // Reuse CSIParam with empty buffer; the final byte will be P/Q/R/S
        m_state = State::CSIParam;
        m_buffer.clear();
        return;
    }

    // Alt+printable character
    auto ubyte = static_cast<unsigned char>(byte);
    if (ubyte >= 0x20 && ubyte <= 0x7E) {
        QChar ch(byte);
        int key = ch.toUpper().unicode();
        Qt::KeyboardModifiers mods = Qt::AltModifier;
        if (ch.isUpper())
            mods |= Qt::ShiftModifier;
        postKeyEvent(key, mods, QString(ch));
        m_state = State::Ground;
        return;
    }

    // ESC ESC -> emit Escape, stay in Escape state for the new one
    if (byte == '\x1b') {
        postKeyEvent(Qt::Key_Escape, Qt::NoModifier);
        m_buffer.clear();
        return;
    }

    // Standalone ESC followed by something unexpected
    postKeyEvent(Qt::Key_Escape, Qt::NoModifier);
    m_state = State::Ground;
    processOneByte(byte);
}

void InputParser::handleCSI(char byte)
{
    // SGR mouse prefix '<'
    if (byte == '<') {
        m_state = State::SGRMouse;
        m_buffer.clear();
        return;
    }

    // Start accumulating parameters
    m_buffer.clear();
    m_state = State::CSIParam;

    // If this is already a final byte (0x40-0x7E), finish immediately
    if (byte >= 0x40 && byte <= 0x7E) {
        finishCSI(byte);
        m_buffer.clear();
        m_state = State::Ground;
    } else {
        m_buffer.append(byte);
    }
}

void InputParser::handleCSIParam(char byte)
{
    // Final bytes: 0x40-0x7E
    if (byte >= 0x40 && byte <= 0x7E) {
        finishCSI(byte);
        m_buffer.clear();
        m_state = State::Ground;
        return;
    }

    // Parameter/intermediate bytes: digits, semicolons, etc.
    if (byte >= 0x20 && byte <= 0x3F) {
        m_buffer.append(byte);
        return;
    }

    // Unexpected byte
    qDebug() << "InputParser: unexpected byte in CSI sequence:" << static_cast<int>(byte);
    discardBuffer();
}

void InputParser::handleSGRMouse(char byte)
{
    // SGR mouse: accumulate until 'M' (press) or 'm' (release)
    if (byte == 'M' || byte == 'm') {
        finishSGRMouse(byte);
        m_buffer.clear();
        m_state = State::Ground;
        return;
    }

    if ((byte >= '0' && byte <= '9') || byte == ';') {
        m_buffer.append(byte);
        return;
    }

    qDebug() << "InputParser: unexpected byte in SGR mouse sequence:" << static_cast<int>(byte);
    discardBuffer();
}

void InputParser::finishCSI(char finalByte)
{
    // Parse accumulated parameters
    QList<int> params;
    if (!m_buffer.isEmpty()) {
        const auto parts = m_buffer.split(';');
        for (const auto &part : parts) {
            bool ok;
            int val = part.toInt(&ok);
            params.append(ok ? val : 0);
        }
    }

    // xterm-style modifiers: param value - 1 encodes modifier bits
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    if (params.size() >= 2 && params.last() > 1) {
        int modParam = params.last() - 1;
        if (modParam & 1) mods |= Qt::ShiftModifier;
        if (modParam & 2) mods |= Qt::AltModifier;
        if (modParam & 4) mods |= Qt::ControlModifier;
    }

    switch (finalByte) {
    // Arrow keys
    case 'A': postKeyEvent(Qt::Key_Up, mods); break;
    case 'B': postKeyEvent(Qt::Key_Down, mods); break;
    case 'C': postKeyEvent(Qt::Key_Right, mods); break;
    case 'D': postKeyEvent(Qt::Key_Left, mods); break;

    // Home / End (xterm-style)
    case 'H': postKeyEvent(Qt::Key_Home, mods); break;
    case 'F': postKeyEvent(Qt::Key_End, mods); break;

    // F1-F4 via SS3 (ESC O P/Q/R/S)
    case 'P': postKeyEvent(Qt::Key_F1, mods); break;
    case 'Q': postKeyEvent(Qt::Key_F2, mods); break;
    case 'R': postKeyEvent(Qt::Key_F3, mods); break;
    case 'S': postKeyEvent(Qt::Key_F4, mods); break;

    // Backtab: CSI Z
    case 'Z': postKeyEvent(Qt::Key_Backtab, Qt::ShiftModifier); break;

    // Tilde-terminated sequences
    case '~': {
        int code = params.isEmpty() ? 0 : params.first();
        switch (code) {
        case 1:  postKeyEvent(Qt::Key_Home, mods); break;
        case 2:  postKeyEvent(Qt::Key_Insert, mods); break;
        case 3:  postKeyEvent(Qt::Key_Delete, mods); break;
        case 4:  postKeyEvent(Qt::Key_End, mods); break;
        case 5:  postKeyEvent(Qt::Key_PageUp, mods); break;
        case 6:  postKeyEvent(Qt::Key_PageDown, mods); break;
        case 11: postKeyEvent(Qt::Key_F1, mods); break;
        case 12: postKeyEvent(Qt::Key_F2, mods); break;
        case 13: postKeyEvent(Qt::Key_F3, mods); break;
        case 14: postKeyEvent(Qt::Key_F4, mods); break;
        case 15: postKeyEvent(Qt::Key_F5, mods); break;
        case 17: postKeyEvent(Qt::Key_F6, mods); break;
        case 18: postKeyEvent(Qt::Key_F7, mods); break;
        case 19: postKeyEvent(Qt::Key_F8, mods); break;
        case 20: postKeyEvent(Qt::Key_F9, mods); break;
        case 21: postKeyEvent(Qt::Key_F10, mods); break;
        case 23: postKeyEvent(Qt::Key_F11, mods); break;
        case 24: postKeyEvent(Qt::Key_F12, mods); break;
        default:
            qDebug() << "InputParser: unknown CSI ~ code:" << code;
            break;
        }
        break;
    }

    default:
        qDebug() << "InputParser: unknown CSI final byte:" << finalByte
                 << "params:" << params;
        break;
    }
}

void InputParser::finishSGRMouse(char finalByte)
{
    // SGR mouse format: Btn;X;Y  finalByte is M (press) or m (release)
    const auto parts = m_buffer.split(';');
    if (parts.size() != 3) {
        qDebug() << "InputParser: malformed SGR mouse sequence:" << m_buffer;
        return;
    }

    bool btnOk, xOk, yOk;
    int btnCode = parts[0].toInt(&btnOk);
    int x = parts[1].toInt(&xOk);
    int y = parts[2].toInt(&yOk);

    if (!btnOk || !xOk || !yOk) {
        qDebug() << "InputParser: invalid SGR mouse parameters:" << m_buffer;
        return;
    }

    // SGR mouse coords are 1-based; convert to 0-based and clamp
    x = clampX(x - 1);
    y = clampY(y - 1);

    // Decode button from low 2 bits
    Qt::MouseButton button = Qt::NoButton;
    int btnId = btnCode & 0x03;
    bool motion = (btnCode & 32) != 0;

    if (btnId == 0)      button = Qt::LeftButton;
    else if (btnId == 1)  button = Qt::MiddleButton;
    else if (btnId == 2)  button = Qt::RightButton;

    TuiMouseEvent::MouseEventType type;
    if (motion) {
        type = TuiMouseEvent::Move;
    } else if (finalByte == 'M') {
        type = TuiMouseEvent::Press;
    } else {
        type = TuiMouseEvent::Release;
    }

    postMouseEvent(type, button, QPoint(x, y));
}

void InputParser::discardBuffer()
{
    qDebug() << "InputParser: discarding incomplete sequence:" << m_buffer.toHex();
    m_buffer.clear();
    m_state = State::Ground;
}

void InputParser::postKeyEvent(int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    if (m_eventTarget) {
        auto *event = new TuiKeyEvent(key, modifiers, text);
        QCoreApplication::postEvent(m_eventTarget, event);
    }
    emit keyEventParsed(key, modifiers, text);
}

void InputParser::postMouseEvent(TuiMouseEvent::MouseEventType type,
                                  Qt::MouseButton button, const QPoint &pos)
{
    if (m_eventTarget) {
        auto *event = new TuiMouseEvent(type, button, pos);
        QCoreApplication::postEvent(m_eventTarget, event);
    }
    emit mouseEventParsed(static_cast<int>(button), pos.x(), pos.y(), static_cast<int>(type));
}

void InputParser::postResizeEvent(const QSize &size)
{
    if (m_eventTarget) {
        auto *event = new TuiResizeEvent(size, m_terminalSize);
        QCoreApplication::postEvent(m_eventTarget, event);
    }
    m_terminalSize = size;
    emit resizeEventParsed(size.width(), size.height());
}

int InputParser::clampX(int x) const
{
    return std::clamp(x, 0, std::max(0, m_terminalSize.width() - 1));
}

int InputParser::clampY(int y) const
{
    return std::clamp(y, 0, std::max(0, m_terminalSize.height() - 1));
}

} // namespace QtTUI
