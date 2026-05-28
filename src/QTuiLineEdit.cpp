#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

// Cursor position (m_cursorPos) is a UTF-16 code-unit index into m_text,
// always kept on a cluster boundary. The scroll offset (m_scrollOffset) and
// all terminal-facing coordinates are display COLUMNS (wide characters
// occupy two columns; combining marks occupy none). QtTuiUnicode maps
// between the two; for narrow-only text index and column coincide.

QTuiLineEdit::QTuiLineEdit(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

QSize QTuiLineEdit::minimumSize() const
{
    return QSize(1, 1);
}

QString QTuiLineEdit::text() const
{
    return m_text;
}

// Keeps the cursor's display column inside the visible column window,
// accounting for the width of the character under the cursor.
void QTuiLineEdit::adjustScrollToCursor()
{
    const int visibleWidth = geometry().width();
    if (visibleWidth <= 0)
        return;

    const int cursorCol = QtTuiUnicode::columnOfIndex(m_text, m_cursorPos);
    int cursorWidth = 1;
    if (m_cursorPos < m_text.length()) {
        int i = m_cursorPos;
        QtTuiUnicode::Cluster c;
        if (QtTuiUnicode::nextCluster(m_text, i, c))
            cursorWidth = c.width;
    }

    if (cursorCol < m_scrollOffset)
        m_scrollOffset = cursorCol;
    else if (cursorCol + cursorWidth > m_scrollOffset + visibleWidth)
        m_scrollOffset = cursorCol + cursorWidth - visibleWidth;
}

void QTuiLineEdit::setText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    m_cursorPos = m_text.length();
    adjustScrollToCursor();
    emit textChanged(m_text);
    markDirty();
}

int QTuiLineEdit::cursorPosition() const
{
    return m_cursorPos;
}

void QTuiLineEdit::setCursorPosition(int pos)
{
    int clamped = qBound(0, pos, static_cast<int>(m_text.length()));
    // Snap to a cluster boundary so the cursor never lands inside a
    // surrogate pair or combining sequence
    if (clamped > 0 && clamped < m_text.length()) {
        clamped = QtTuiUnicode::indexOfColumn(
            m_text, QtTuiUnicode::columnOfIndex(m_text, clamped));
    }
    if (clamped == m_cursorPos)
        return;
    m_cursorPos = clamped;
    adjustScrollToCursor();
    markDirty();
}

void QTuiLineEdit::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const QString eventText = event->text();

    switch (key) {
    case Qt::Key_Left:
        m_cursorPos = QtTuiUnicode::prevClusterIndex(m_text, m_cursorPos);
        break;
    case Qt::Key_Right:
        m_cursorPos = QtTuiUnicode::nextClusterIndex(m_text, m_cursorPos);
        break;
    case Qt::Key_Home:
        m_cursorPos = 0;
        break;
    case Qt::Key_End:
        m_cursorPos = m_text.length();
        break;
    case Qt::Key_Backspace:
        if (m_cursorPos > 0) {
            // Remove the whole preceding cluster (surrogate pair + marks)
            const int prev = QtTuiUnicode::prevClusterIndex(m_text, m_cursorPos);
            m_text.remove(prev, m_cursorPos - prev);
            m_cursorPos = prev;
            emit textChanged(m_text);
        }
        break;
    case Qt::Key_Delete:
        if (m_cursorPos < m_text.length()) {
            const int next = QtTuiUnicode::nextClusterIndex(m_text, m_cursorPos);
            m_text.remove(m_cursorPos, next - m_cursorPos);
            emit textChanged(m_text);
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit returnPressed();
        break;
    default:
        // Accept printable characters; a leading high surrogate means the
        // parser delivered a non-BMP character (e.g. emoji) as a full pair.
        if (!eventText.isEmpty()
            && (eventText.at(0).isPrint() || eventText.at(0).isHighSurrogate())) {
            // Insert the whole string so surrogate pairs stay together
            m_text.insert(m_cursorPos, eventText);
            m_cursorPos += eventText.length();
            emit textChanged(m_text);
        }
        break;
    }

    adjustScrollToCursor();
    markDirty();
}

void QTuiLineEdit::pasteEvent(TuiPasteEvent *event)
{
    // One line only: a line break or tab becomes a space, and other
    // control characters are removed. A paste never emits returnPressed().
    QString text;
    for (const QChar ch : event->text()) {
        if (ch == QLatin1Char('\n') || ch == QLatin1Char('\t'))
            text += QLatin1Char(' ');
        else if (!ch.isNull() && ch.category() != QChar::Other_Control)
            text += ch;
    }
    if (text.isEmpty())
        return;

    m_text.insert(m_cursorPos, text);
    m_cursorPos += text.length();
    emit textChanged(m_text);
    adjustScrollToCursor();
    markDirty();
}

void QTuiLineEdit::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    const int clickCol = event->x() - geo.x() + m_scrollOffset;
    m_cursorPos = (clickCol <= 0) ? 0 : QtTuiUnicode::indexOfColumn(m_text, clickCol);
    markDirty();
}

QPoint QTuiLineEdit::terminalCursorPos() const
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return QPoint(-1, -1);

    const int col = QtTuiUnicode::columnOfIndex(m_text, m_cursorPos) - m_scrollOffset;
    if (col < 0 || col >= geo.width())
        return QPoint(-1, -1);
    return QPoint(geo.x() + col, geo.y());
}

void QTuiLineEdit::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    const auto theme = themeProperties();
    const bool focused = hasFocus();
    const int visibleWidth = geo.width();
    const int y = geo.y();
    const int xStart = geo.x();

    // Base style from the theme and focus state
    Cell base;
    base.foreground = focused ? theme.focusForeground : theme.foreground;
    base.background = focused ? theme.focusBackground : theme.background;
    base.bold = theme.bold;
    base.italic = theme.italic;
    base.underline = theme.underline;
    base.inverse = false;

    // Fill the whole line first (also resets stale wide/continuation cells)
    for (int i = 0; i < visibleWidth; ++i)
        buffer.backCell(xStart + i, y) = base;

    // Draw the visible clusters at their display columns
    int col = -m_scrollOffset;
    int i = 0;
    QtTuiUnicode::Cluster c;
    while (col < visibleWidth && QtTuiUnicode::nextCluster(m_text, i, c)) {
        const int nextCol = col + c.width;
        if (nextCol <= 0) {
            col = nextCol;
            continue;
        }
        Cell style = base;
        if (focused && c.index == m_cursorPos)
            style.inverse = true;
        if (col >= 0 && nextCol <= visibleWidth) {
            buffer.putChar(xStart + col, y, c.base, c.combining, style);
        } else {
            // A wide cluster straddling the edge: show its visible half as a
            // space (keeping the style so a cursor there stays visible)
            for (int x = qMax(0, col); x < qMin(visibleWidth, nextCol); ++x)
                buffer.putChar(xStart + x, y, U' ', style);
        }
        col = nextCol;
    }

    // Cursor sitting at the end of the text highlights the empty cell there
    if (focused && m_cursorPos == m_text.length()) {
        const int cursorCol = QtTuiUnicode::columnOfIndex(m_text, m_cursorPos)
            - m_scrollOffset;
        if (cursorCol >= 0 && cursorCol < visibleWidth)
            buffer.backCell(xStart + cursorCol, y).inverse = true;
    }
}

} // namespace QtTUI
