#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiClipboard.h>
#include <QtTUI/QTuiCompleter.h>
#include <QtTUI/QTuiValidator.h>

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
    // With a mask, the cursor starts at the first position for a character
    connect(this, &QTuiWidget::focusGained, this, [this]() {
        if (m_mask.isEmpty())
            return;
        m_cursorPos = 0;
        while (m_cursorPos < m_mask.size() && m_mask.at(m_cursorPos).separator)
            ++m_cursorPos;
        m_selAnchor = -1;
        adjustScrollToCursor();
        markDirty();
    });
}

QString QTuiLineEdit::inputMask() const
{
    return m_inputMask;
}

void QTuiLineEdit::setInputMask(const QString &inputMask)
{
    if (m_inputMask == inputMask)
        return;
    const QString oldText = text();
    m_inputMask = inputMask;

    // Read the mask into one cell per position
    m_mask.clear();
    m_blank = QLatin1Char(' ');
    quint8 caseMode = 0;
    for (int i = 0; i < inputMask.length(); ++i) {
        const QChar c = inputMask.at(i);
        if (c == QLatin1Char(';')) {
            if (i + 1 < inputMask.length())
                m_blank = inputMask.at(i + 1);
            break;
        }
        if (c == QLatin1Char('\\')) {
            if (++i < inputMask.length())
                m_mask.append({inputMask.at(i), true, caseMode});
        } else if (c == QLatin1Char('>')) {
            caseMode = 1;
        } else if (c == QLatin1Char('<')) {
            caseMode = 2;
        } else if (c == QLatin1Char('!')) {
            caseMode = 0;
        } else if (!QStringLiteral("[]{}").contains(c)) { // reserved, as in Qt
            m_mask.append({c, !QStringLiteral("AaNnXx90Dd#HhBb").contains(c), caseMode});
        }
    }
    // As in Qt, the mask sets the maximum length
    m_maxLength = m_mask.isEmpty() ? 32767 : int(m_mask.size());

    // Keep the old text, as far as the new mask takes it
    m_text.clear();
    m_undoStack.clear();
    m_redoStack.clear();
    m_typingEnd = -1;
    setText(oldText);
    m_cursorPos = 0;
    while (m_cursorPos < m_mask.size() && m_mask.at(m_cursorPos).separator)
        ++m_cursorPos;
    if (m_mask.isEmpty())
        m_cursorPos = m_text.length();
    m_selAnchor = -1;
    m_scrollOffset = 0;
    adjustScrollToCursor();
    markDirty();
}

bool QTuiLineEdit::maskAccepts(QChar type, QChar ch) const
{
    // A position that can stay empty also takes the blank
    const bool optional = type.isLower() || type == QLatin1Char('0') || type == QLatin1Char('#');
    if (optional && ch == m_blank)
        return true;
    switch (type.toLower().unicode()) {
    case 'a': return ch.isLetter();
    case 'n': return ch.isLetterOrNumber();
    case 'x': return ch.isPrint() && ch != m_blank;
    case '9':
    case '0': return ch.isNumber();
    case 'd': return ch.isNumber() && ch.digitValue() > 0;
    case '#': return ch.isNumber() || ch == QLatin1Char('+') || ch == QLatin1Char('-');
    case 'h': return ch.isDigit() || (ch.toLower() >= QLatin1Char('a') && ch.toLower() <= QLatin1Char('f'));
    case 'b': return ch == QLatin1Char('0') || ch == QLatin1Char('1');
    }
    return false;
}

// This follows QLineEdit: a character goes into the position at the cursor
// if the mask takes it there. If not, a character that is a later separator
// moves the cursor after that separator, and any other character goes into
// the next position that takes it.
int QTuiLineEdit::maskInsert(QString &buffer, int pos, const QString &text) const
{
    const int size = m_mask.size();
    auto put = [&](int i, QChar ch) {
        const MaskCell &cell = m_mask.at(i);
        buffer[i] = cell.caseMode == 1 ? ch.toUpper() : cell.caseMode == 2 ? ch.toLower() : ch;
    };
    auto findFrom = [&](int from, bool separator, QChar ch) {
        for (int i = from; i < size; ++i) {
            const MaskCell &cell = m_mask.at(i);
            if (cell.separator == separator && (separator ? cell.ch == ch : maskAccepts(cell.ch, ch)))
                return i;
        }
        return -1;
    };

    int i = pos;
    int t = 0;
    bool changed = false;
    while (i < size && t < text.length()) {
        const QChar ch = text.at(t);
        const MaskCell &cell = m_mask.at(i);
        if (cell.separator) {
            // Go over the separator; it also takes the same typed character
            if (ch == cell.ch) {
                ++t;
                changed = true;
            }
            ++i;
            continue;
        }
        if (maskAccepts(cell.ch, ch)) {
            put(i++, ch);
            changed = true;
        } else if (int n = findFrom(i, true, ch); n >= 0) {
            // One typed separator directly after the same separator does
            // nothing: the cursor went over it already
            if (text.length() != 1 || i == 0 || !m_mask.at(i - 1).separator || m_mask.at(i - 1).ch != ch) {
                i = n + 1;
                changed = true;
            }
        } else if (n = findFrom(i, false, ch); n >= 0) {
            put(n, ch);
            i = n + 1;
            changed = true;
        }
        ++t;
    }
    if (!changed)
        return -1;
    // The cursor waits at the next position for a character
    while (i < size && m_mask.at(i).separator)
        ++i;
    return i;
}

QString QTuiLineEdit::stripMask(const QString &buffer) const
{
    QString text;
    for (int i = 0; i < buffer.length() && i < m_mask.size(); ++i) {
        if (m_mask.at(i).separator || buffer.at(i) != m_blank)
            text += buffer.at(i);
    }
    return text;
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

int QTuiLineEdit::maxLength() const
{
    return m_maxLength;
}

void QTuiLineEdit::setMaxLength(int length)
{
    if (!m_mask.isEmpty())
        return; // the mask sets the length
    m_maxLength = qMax(0, length);
    if (m_text.length() > m_maxLength)
        setText(m_text.left(m_maxLength));
}

const QTuiValidator *QTuiLineEdit::validator() const
{
    return m_validator;
}

void QTuiLineEdit::setValidator(const QTuiValidator *validator)
{
    m_validator = validator;
}

bool QTuiLineEdit::hasAcceptableInput() const
{
    // Each mask position must hold a character that it takes; only a
    // position that can stay empty takes the blank
    for (int i = 0; i < m_mask.size(); ++i) {
        if (!m_mask.at(i).separator && !maskAccepts(m_mask.at(i).ch, m_text.at(i)))
            return false;
    }
    if (!m_validator)
        return true;
    QString text = this->text();
    int pos = qMin(m_cursorPos, int(text.length()));
    return m_validator->validate(text, pos) == QTuiValidator::Acceptable;
}

bool QTuiLineEdit::hasSelectedText() const
{
    return m_selAnchor >= 0 && m_selAnchor != m_cursorPos;
}

QString QTuiLineEdit::selectedText() const
{
    if (!hasSelectedText())
        return QString();
    const int start = qMin(m_selAnchor, m_cursorPos);
    return m_text.mid(start, qAbs(m_cursorPos - m_selAnchor));
}

int QTuiLineEdit::selectionStart() const
{
    return hasSelectedText() ? qMin(m_selAnchor, m_cursorPos) : -1;
}

void QTuiLineEdit::setSelection(int start, int length)
{
    const int a = qBound(0, start, int(m_text.length()));
    const int b = qBound(0, start + length, int(m_text.length()));
    m_selAnchor = a;
    m_cursorPos = b;
    adjustScrollToCursor();
    markDirty();
}

void QTuiLineEdit::selectAll()
{
    setSelection(0, m_text.length());
}

void QTuiLineEdit::deselect()
{
    if (m_selAnchor < 0)
        return;
    m_selAnchor = -1;
    markDirty();
}

void QTuiLineEdit::copy() const
{
    // A hidden text must not leave the line edit
    if (m_echoMode == Normal && hasSelectedText())
        QTuiApplication::clipboard()->setText(selectedText());
}

void QTuiLineEdit::cut()
{
    if (m_echoMode != Normal || m_readOnly || !hasSelectedText())
        return;
    copy();
    replaceSelection(QString());
    adjustScrollToCursor();
    markDirty();
}

void QTuiLineEdit::paste()
{
    insertPastedText(QTuiApplication::clipboard()->text());
}

// Maximum number of undo steps
static constexpr int kMaxUndo = 100;

void QTuiLineEdit::undo()
{
    if (m_undoStack.isEmpty())
        return;
    m_redoStack.append({m_text, m_cursorPos});
    const UndoState state = m_undoStack.takeLast();
    m_text = state.text;
    m_cursorPos = state.cursor;
    m_selAnchor = -1;
    m_typingEnd = -1;
    emit textChanged(text());
    adjustScrollToCursor();
    markDirty();
}

void QTuiLineEdit::redo()
{
    if (m_redoStack.isEmpty())
        return;
    m_undoStack.append({m_text, m_cursorPos});
    const UndoState state = m_redoStack.takeLast();
    m_text = state.text;
    m_cursorPos = state.cursor;
    m_selAnchor = -1;
    m_typingEnd = -1;
    emit textChanged(text());
    adjustScrollToCursor();
    markDirty();
}

bool QTuiLineEdit::isUndoAvailable() const
{
    return !m_undoStack.isEmpty();
}

bool QTuiLineEdit::isRedoAvailable() const
{
    return !m_redoStack.isEmpty();
}

bool QTuiLineEdit::replaceSelection(const QString &text)
{
    if (hasSelectedText())
        return replaceRange(qMin(m_selAnchor, m_cursorPos), qMax(m_selAnchor, m_cursorPos), text);
    return replaceRange(m_cursorPos, m_cursorPos, text);
}

bool QTuiLineEdit::replaceRange(int start, int end, const QString &text)
{
    if (m_readOnly)
        return false;

    QString inserted = text;
    QString candidate = m_text;
    int pos = start;
    if (!m_mask.isEmpty()) {
        // With a mask the text keeps its length: a removed character
        // becomes a blank, and a new character goes into a mask position
        for (int i = start; i < end; ++i) {
            if (!m_mask.at(i).separator)
                candidate[i] = m_blank;
        }
        if (!inserted.isEmpty())
            pos = maskInsert(candidate, start, inserted);
        if (pos < 0)
            return false;
        QString plain = stripMask(candidate);
        int plainPos = plain.length();
        if (m_validator && m_validator->validate(plain, plainPos) == QTuiValidator::Invalid)
            return false;
        if (candidate == m_text) {
            // Only the cursor moves (over a separator or a blank)
            m_cursorPos = pos;
            m_selAnchor = -1;
            return true;
        }
    } else {
        // Cut the inserted text to the length limit, at a cluster boundary
        const int room = m_maxLength - (m_text.length() - (end - start));
        if (inserted.length() > room) {
            int cut = 0;
            while (cut < inserted.length()) {
                const int next = QtTuiUnicode::nextClusterIndex(inserted, cut);
                if (next > room)
                    break;
                cut = next;
            }
            inserted.truncate(qMax(0, cut));
        }
        if (start == end && inserted.isEmpty())
            return false;

        candidate.replace(start, end - start, inserted);
        pos = start + inserted.length();
        if (m_validator && m_validator->validate(candidate, pos) == QTuiValidator::Invalid)
            return false;
    }

    // One undo step per edit, except that one character typed right after
    // the previous one joins its step
    const bool typing = start == end && QtTuiUnicode::nextClusterIndex(inserted, 0) == inserted.length()
        && inserted != QLatin1String(" ");
    if (!(typing && start == m_typingEnd)) {
        m_undoStack.append({m_text, m_cursorPos});
        if (m_undoStack.size() > kMaxUndo)
            m_undoStack.removeFirst();
    }
    m_redoStack.clear();

    m_text = candidate;
    m_cursorPos = qBound(0, pos, int(m_text.length()));
    m_selAnchor = -1;
    m_typingEnd = typing ? m_cursorPos : -1;
    emit textChanged(this->text());
    return true;
}

QSize QTuiLineEdit::minimumSize() const
{
    return QSize(1, 1);
}

QString QTuiLineEdit::text() const
{
    return m_mask.isEmpty() ? m_text : stripMask(m_text);
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

void QTuiLineEdit::setText(const QString &newText)
{
    QString text = newText.left(m_maxLength);
    if (!m_mask.isEmpty()) {
        // Start from the empty template and put in what the mask takes
        text.clear();
        for (const MaskCell &cell : m_mask)
            text += cell.separator ? cell.ch : m_blank;
        maskInsert(text, 0, newText);
    }
    if (m_text == text)
        return;
    m_text = text;
    m_cursorPos = m_text.length();
    m_selAnchor = -1;
    m_undoStack.clear();
    m_redoStack.clear();
    m_typingEnd = -1;
    adjustScrollToCursor();
    emit textChanged(this->text());
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
    if (clamped == m_cursorPos && m_selAnchor < 0)
        return;
    m_cursorPos = clamped;
    m_selAnchor = -1;
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
    const QString oldText = text();

    // Clipboard and select all
    if (event->modifiers() == Qt::ControlModifier) {
        switch (key) {
        case Qt::Key_V: paste(); return;
        case Qt::Key_C: copy(); return;
        case Qt::Key_X: cut(); return;
        case Qt::Key_A: selectAll(); return;
        case Qt::Key_Z: undo(); return;
        case Qt::Key_Y: redo(); return;
        case Qt::Key_W:
        case Qt::Key_U: {
            // The vim and shell insert keys: Ctrl+U deletes the text before
            // the cursor, Ctrl+W the word before the cursor (up to a space)
            int start = 0;
            if (key == Qt::Key_W) {
                start = m_cursorPos;
                while (start > 0 && m_text.at(start - 1).isSpace())
                    --start;
                while (start > 0 && !m_text.at(start - 1).isSpace())
                    --start;
            }
            replaceRange(start, m_cursorPos, QString());
            adjustScrollToCursor();
            markDirty();
            if (m_completer && this->text() != oldText)
                m_completer->updateForText(this, this->text());
            return;
        }
        default: break;
        }
    }
    if (key == Qt::Key_Z && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        redo();
        return;
    }

    // Shift+movement extends the selection; plain movement ends it
    const bool movement = key == Qt::Key_Left || key == Qt::Key_Right
        || key == Qt::Key_Home || key == Qt::Key_End;
    if (movement) {
        if (event->modifiers() & Qt::ShiftModifier) {
            if (m_selAnchor < 0)
                m_selAnchor = m_cursorPos;
        } else if (hasSelectedText()) {
            // Left/Right with a selection go to its start/end, as in Qt
            const int start = qMin(m_selAnchor, m_cursorPos);
            const int end = qMax(m_selAnchor, m_cursorPos);
            m_selAnchor = -1;
            if (key == Qt::Key_Left || key == Qt::Key_Right) {
                m_cursorPos = key == Qt::Key_Left ? start : end;
                adjustScrollToCursor();
                markDirty();
                return;
            }
        } else {
            m_selAnchor = -1;
        }
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
        // Remove the selection, else the whole preceding cluster
        // (surrogate pair + marks)
        if (hasSelectedText())
            replaceSelection(QString());
        else if (m_cursorPos > 0) {
            int from = QtTuiUnicode::prevClusterIndex(m_text, m_cursorPos);
            // With a mask, go over the separators to the character before them
            while (!m_mask.isEmpty() && from > 0 && m_mask.at(from).separator)
                --from;
            replaceRange(from, m_cursorPos, QString());
        }
        break;
    case Qt::Key_Delete:
        if (hasSelectedText())
            replaceSelection(QString());
        else if (m_cursorPos < m_text.length())
            replaceRange(m_cursorPos, QtTuiUnicode::nextClusterIndex(m_text, m_cursorPos), QString());
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // As in QLineEdit: only acceptable input (after fixup) is returned
        if (m_validator && m_mask.isEmpty() && !hasAcceptableInput()) {
            QString fixed = m_text;
            m_validator->fixup(fixed);
            if (fixed != m_text) {
                m_text = fixed;
                m_cursorPos = qMin(m_cursorPos, int(m_text.length()));
                emit textChanged(text());
            }
        }
        if (hasAcceptableInput())
            emit returnPressed();
        break;
    default:
        // Accept printable characters; a leading high surrogate means the
        // parser delivered a non-BMP character (e.g. emoji) as a full pair.
        if (!eventText.isEmpty()
            && (eventText.at(0).isPrint() || eventText.at(0).isHighSurrogate())) {
            // Insert the whole string so surrogate pairs stay together;
            // typing replaces the selection
            replaceSelection(eventText);
        }
        break;
    }

    adjustScrollToCursor();
    markDirty();

    // Typing updates the completions; cursor movement does not
    if (m_completer && text() != oldText)
        m_completer->updateForText(this, text());
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
    if (!replaceSelection(text))
        return;
    adjustScrollToCursor();
    markDirty();
    if (m_completer)
        m_completer->updateForText(this, this->text());
}

void QTuiLineEdit::mousePressEvent(TuiMouseEvent *event)
{
    // A press places the cursor; a drag with the left button selects from
    // the press position to the mouse
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
        return;

    const QRect geo = geometry();
    const int clickCol = event->x() - geo.x() + m_scrollOffset;
    const int shownIndex = clickCol <= 0 ? 0 : QtTuiUnicode::indexOfColumn(displayText(), clickCol);
    if (drag && m_selAnchor < 0)
        m_selAnchor = m_cursorPos;
    m_cursorPos = textIndex(shownIndex);
    if (!drag)
        m_selAnchor = -1;
    adjustScrollToCursor();
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
    const int selStart = hasSelectedText() ? displayIndex(qMin(m_selAnchor, m_cursorPos)) : -1;
    const int selEnd = hasSelectedText() ? displayIndex(qMax(m_selAnchor, m_cursorPos)) : -1;

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
        // Selected text shows in the inverted colors
        if (c.index >= selStart && c.index < selEnd)
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
