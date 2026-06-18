#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiClipboard.h>
#include <QtTUI/QTuiCompleter.h>

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
    // The completion popup closes when the line edit loses focus
    connect(this, &QTuiWidget::focusLost, this, [this]() {
        if (m_completer)
            m_completer->hidePopup();
    });
}

QTuiCompleter *QTuiLineEdit::completer() const
{
    return m_completer;
}

void QTuiLineEdit::setCompleter(QTuiCompleter *completer)
{
    if (m_completer)
        m_completer->hidePopup();
    m_completer = completer;
}

// The character that stands for each character in Password mode
static constexpr char16_t kPasswordChar = u'\u2022'; // •

QTuiLineEdit::EchoMode QTuiLineEdit::echoMode() const
{
    return m_echoMode;
}

void QTuiLineEdit::setEchoMode(EchoMode mode)
{
    if (m_echoMode == mode)
        return;
    m_echoMode = mode;
    m_scrollOffset = 0;
    adjustScrollToCursor();
    markDirty();
}

QString QTuiLineEdit::displayText() const
{
    switch (m_echoMode) {
    case Normal:
        return m_text;
    case NoEcho:
        return QString();
    case Password:
        return QString(displayIndex(m_text.length()), QChar(kPasswordChar));
    }
    return m_text;
}

int QTuiLineEdit::displayIndex(int textIndex) const
{
    switch (m_echoMode) {
    case Normal:
        return textIndex;
    case NoEcho:
        return 0;
    case Password: {
        // One mask character per cluster before textIndex
        int count = 0;
        for (int i = 0; i < textIndex; i = QtTuiUnicode::nextClusterIndex(m_text, i))
            ++count;
        return count;
    }
    }
    return textIndex;
}

int QTuiLineEdit::textIndex(int displayIndex) const
{
    switch (m_echoMode) {
    case Normal:
        return displayIndex;
    case NoEcho:
        return m_text.length();
    case Password: {
        int i = 0;
        for (int n = 0; n < displayIndex && i < m_text.length(); ++n)
            i = QtTuiUnicode::nextClusterIndex(m_text, i);
        return i;
    }
    }
    return displayIndex;
}

QString QTuiLineEdit::placeholderText() const
{
    return m_placeholderText;
}

void QTuiLineEdit::setPlaceholderText(const QString &text)
{
    if (m_placeholderText == text)
        return;
    m_placeholderText = text;
    if (m_text.isEmpty())
        markDirty();
}

bool QTuiLineEdit::isReadOnly() const
{
    return m_readOnly;
}

void QTuiLineEdit::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool QTuiLineEdit::replaceRange(int start, int end, const QString &text)
{
    if (m_readOnly)
        return false;
    if (start == end && text.isEmpty())
        return false;
    m_text.replace(start, end - start, text);
    m_cursorPos = start + text.length();
    emit textChanged(m_text);
    return true;
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

    const QString shown = displayText();
    const int cursor = displayIndex(m_cursorPos);
    const int cursorCol = QtTuiUnicode::columnOfIndex(shown, cursor);
    int cursorWidth = 1;
    if (cursor < shown.length()) {
        int i = cursor;
        QtTuiUnicode::Cluster c;
        if (QtTuiUnicode::nextCluster(shown, i, c))
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

    // An open completion popup takes Up, Down, Enter and Escape
    if (m_completer && m_completer->handleKey(key))
        return;
    const QString oldText = m_text;

    // Ctrl+V pastes the application clipboard
    if (key == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        insertPastedText(QTuiApplication::clipboard()->text());
        return;
    }

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
        // Remove the whole preceding cluster (surrogate pair + marks)
        if (m_cursorPos > 0)
            replaceRange(QtTuiUnicode::prevClusterIndex(m_text, m_cursorPos), m_cursorPos, QString());
        break;
    case Qt::Key_Delete:
        if (m_cursorPos < m_text.length())
            replaceRange(m_cursorPos, QtTuiUnicode::nextClusterIndex(m_text, m_cursorPos), QString());
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
            replaceRange(m_cursorPos, m_cursorPos, eventText);
        }
        break;
    }

    adjustScrollToCursor();
    markDirty();

    // Typing updates the completions; cursor movement does not
    if (m_completer && m_text != oldText)
        m_completer->updateForText(this, m_text);
}

void QTuiLineEdit::pasteEvent(TuiPasteEvent *event)
{
    insertPastedText(event->text());
}

void QTuiLineEdit::insertPastedText(const QString &pasted)
{
    // One line only: a line break or tab becomes a space, and other
    // control characters are removed. A paste never emits returnPressed().
    QString text;
    for (const QChar ch : pasted) {
        if (ch == QLatin1Char('\n') || ch == QLatin1Char('\t'))
            text += QLatin1Char(' ');
        else if (!ch.isNull() && ch.category() != QChar::Other_Control)
            text += ch;
    }
    if (!replaceRange(m_cursorPos, m_cursorPos, text))
        return;
    adjustScrollToCursor();
    markDirty();
    if (m_completer)
        m_completer->updateForText(this, m_text);
}

void QTuiLineEdit::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    const int clickCol = event->x() - geo.x() + m_scrollOffset;
    const int shownIndex = clickCol <= 0 ? 0 : QtTuiUnicode::indexOfColumn(displayText(), clickCol);
    m_cursorPos = textIndex(shownIndex);
    markDirty();
}

QPoint QTuiLineEdit::terminalCursorPos() const
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return QPoint(-1, -1);

    const int col = QtTuiUnicode::columnOfIndex(displayText(), displayIndex(m_cursorPos))
        - m_scrollOffset;
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
    const QString shown = displayText();
    const int cursor = displayIndex(m_cursorPos);

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

    // An empty line edit shows the placeholder in the muted color, with
    // the cursor on its first cell
    if (m_text.isEmpty() && !m_placeholderText.isEmpty()) {
        // On the focus colors the muted color has too little contrast, so
        // a focused placeholder uses the text color in italics
        Cell hint = base;
        if (focused)
            hint.italic = true;
        else
            hint.foreground = theme.disabledForeground;
        renderTextTruncated(buffer, xStart, y, m_placeholderText, visibleWidth, hint);
        if (focused)
            buffer.backCell(xStart, y).inverse = true;
        return;
    }

    // Draw the visible clusters at their display columns
    int col = -m_scrollOffset;
    int i = 0;
    QtTuiUnicode::Cluster c;
    while (col < visibleWidth && QtTuiUnicode::nextCluster(shown, i, c)) {
        const int nextCol = col + c.width;
        if (nextCol <= 0) {
            col = nextCol;
            continue;
        }
        Cell style = base;
        if (focused && c.index == cursor)
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
    if (focused && cursor == shown.length()) {
        const int cursorCol = QtTuiUnicode::columnOfIndex(shown, cursor)
            - m_scrollOffset;
        if (cursorCol >= 0 && cursorCol < visibleWidth)
            buffer.backCell(xStart + cursorCol, y).inverse = true;
    }
}

} // namespace QtTUI
