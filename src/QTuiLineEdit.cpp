#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiLineEdit::QTuiLineEdit(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

QString QTuiLineEdit::text() const
{
    return m_text;
}

void QTuiLineEdit::setText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    m_cursorPos = m_text.length();
    // Adjust scroll offset
    int visibleWidth = geometry().width();
    if (visibleWidth > 0 && m_cursorPos >= m_scrollOffset + visibleWidth)
        m_scrollOffset = m_cursorPos - visibleWidth + 1;
    if (m_cursorPos < m_scrollOffset)
        m_scrollOffset = m_cursorPos;
    emit textChanged(m_text);
}

int QTuiLineEdit::cursorPosition() const
{
    return m_cursorPos;
}

void QTuiLineEdit::setCursorPosition(int pos)
{
    m_cursorPos = qBound(0, pos, m_text.length());
}

void QTuiLineEdit::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const QString eventText = event->text();

    switch (key) {
    case Qt::Key_Left:
        m_cursorPos = qMax(0, m_cursorPos - 1);
        break;
    case Qt::Key_Right:
        m_cursorPos = qMin(m_text.length(), m_cursorPos + 1);
        break;
    case Qt::Key_Home:
        m_cursorPos = 0;
        break;
    case Qt::Key_End:
        m_cursorPos = m_text.length();
        break;
    case Qt::Key_Backspace:
        if (m_cursorPos > 0) {
            m_text.remove(m_cursorPos - 1, 1);
            --m_cursorPos;
            emit textChanged(m_text);
        }
        break;
    case Qt::Key_Delete:
        if (m_cursorPos < m_text.length()) {
            m_text.remove(m_cursorPos, 1);
            emit textChanged(m_text);
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit returnPressed();
        break;
    default:
        if (!eventText.isEmpty() && eventText.at(0).isPrint()) {
            m_text.insert(m_cursorPos, eventText.at(0));
            ++m_cursorPos;
            emit textChanged(m_text);
        }
        break;
    }

    // Adjust scroll offset to keep cursor visible
    int visibleWidth = geometry().width();
    if (visibleWidth > 0) {
        if (m_cursorPos < m_scrollOffset)
            m_scrollOffset = m_cursorPos;
        else if (m_cursorPos >= m_scrollOffset + visibleWidth)
            m_scrollOffset = m_cursorPos - visibleWidth + 1;
    }
}

void QTuiLineEdit::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    int visibleWidth = geo.width();
    int y = geo.y();
    int xStart = geo.x();

    for (int i = 0; i < visibleWidth; ++i) {
        int textIdx = m_scrollOffset + i;
        Cell &cell = buffer.backCell(xStart + i, y);
        if (textIdx < m_text.length()) {
            cell.character = m_text.at(textIdx);
        } else {
            cell.character = QLatin1Char(' ');
        }
        // Highlight cursor position
        if (hasFocus() && textIdx == m_cursorPos) {
            cell.inverse = true;
        } else {
            cell.inverse = false;
        }
    }
}

} // namespace QtTUI
