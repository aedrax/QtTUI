#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiClipboard.h>

namespace QtTUI {

// Cursor/selection positions (m_cursorCol, m_sel*Col) are UTF-16 code-unit
// indices into their line, kept on cluster boundaries. The horizontal scroll
// offset (m_scrollCol), the sticky column (m_desiredCol) and all
// terminal-facing coordinates are display COLUMNS (wide characters occupy
// two columns; combining marks occupy none). QtTuiUnicode maps between the
// two; for narrow-only text index and column coincide.

QTuiTextEdit::QTuiTextEdit(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    m_lines.append(QString());
}

QString QTuiTextEdit::toPlainText() const
{
    return m_lines.join(QLatin1Char('\n'));
}

void QTuiTextEdit::setPlainText(const QString &text)
{
    m_lines = text.split(QLatin1Char('\n'));
    if (m_lines.isEmpty())
        m_lines.append(QString());
    m_cursorRow = 0;
    m_cursorCol = 0;
    m_scrollRow = 0;
    m_scrollCol = 0;
    m_desiredCol = 0;
    clearSelection();
    m_undoStack.clear();
    m_redoStack.clear();
    m_typingRow = m_typingCol = -1;
    emit textChanged();
    markDirty();
}

bool QTuiTextEdit::isReadOnly() const
{
    return m_readOnly;
}

void QTuiTextEdit::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

int QTuiTextEdit::cursorRow() const
{
    return m_cursorRow;
}

int QTuiTextEdit::cursorColumn() const
{
    return m_cursorCol;
}

bool QTuiTextEdit::hasSelection() const
{
    return m_selStartRow >= 0 && m_selStartCol >= 0
        && m_selEndRow >= 0 && m_selEndCol >= 0
        && (m_selStartRow != m_selEndRow || m_selStartCol != m_selEndCol);
}

QString QTuiTextEdit::selectedText() const
{
    if (!hasSelection())
        return QString();

    int sr, sc, er, ec;
    normalizeSelection(sr, sc, er, ec);

    if (sr == er) {
        return m_lines.at(sr).mid(sc, ec - sc);
    }

    QString result;
    result += m_lines.at(sr).mid(sc);
    result += QLatin1Char('\n');
    for (int r = sr + 1; r < er; ++r) {
        result += m_lines.at(r);
        result += QLatin1Char('\n');
    }
    result += m_lines.at(er).left(ec);
    return result;
}

void QTuiTextEdit::clearSelection()
{
    m_selStartRow = -1;
    m_selStartCol = -1;
    m_selEndRow = -1;
    m_selEndCol = -1;
    m_selecting = false;
}

void QTuiTextEdit::selectAll()
{
    m_selStartRow = 0;
    m_selStartCol = 0;
    m_cursorRow = m_lines.size() - 1;
    m_cursorCol = m_lines.last().length();
    m_selEndRow = m_cursorRow;
    m_selEndCol = m_cursorCol;
    m_selecting = true;
    m_desiredCol = QtTuiUnicode::columnOfIndex(m_lines.at(m_cursorRow), m_cursorCol);
    scrollToVisible();
    markDirty();
}

void QTuiTextEdit::startOrExtendSelection()
{
    if (!m_selecting) {
        m_selStartRow = m_cursorRow;
        m_selStartCol = m_cursorCol;
        m_selecting = true;
    }
    m_selEndRow = m_cursorRow;
    m_selEndCol = m_cursorCol;
}

void QTuiTextEdit::normalizeSelection(int &startRow, int &startCol, int &endRow, int &endCol) const
{
    startRow = m_selStartRow;
    startCol = m_selStartCol;
    endRow = m_selEndRow;
    endCol = m_selEndCol;

    if (startRow > endRow || (startRow == endRow && startCol > endCol)) {
        std::swap(startRow, endRow);
        std::swap(startCol, endCol);
    }
}

void QTuiTextEdit::insertText(const QString &text)
{
    if (m_readOnly || text.isEmpty())
        return;
    // Insert the whole string: for non-BMP characters (e.g. emoji) the
    // parser delivers a full surrogate pair, which must stay together.
    m_lines[m_cursorRow].insert(m_cursorCol, text);
    m_cursorCol += text.length();
    emit textChanged();
}

void QTuiTextEdit::pasteEvent(TuiPasteEvent *event)
{
    const QStringList before = m_lines;
    const int row = m_cursorRow;
    const int col = m_cursorCol;
    insertPastedText(event->text());
    recordEdit(before, row, col, false);
}

void QTuiTextEdit::insertPastedText(const QString &pasted)
{
    if (m_readOnly)
        return;

    // Keep line breaks and tabs; remove other control characters
    QString text;
    for (const QChar ch : pasted) {
        if (ch == QLatin1Char('\n') || ch == QLatin1Char('\t')
            || (!ch.isNull() && ch.category() != QChar::Other_Control))
            text += ch;
    }
    if (text.isEmpty())
        return;

    if (hasSelection())
        deleteSelection(); // a paste replaces the selection
    else
        clearSelection();

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        if (i > 0)
            insertNewLine();
        insertText(lines.at(i));
    }
    m_desiredCol = QtTuiUnicode::columnOfIndex(m_lines.at(m_cursorRow), m_cursorCol);
    scrollToVisible();
    markDirty();
}

void QTuiTextEdit::appendLines(const QString &text, int maxLines)
{
    const bool follow = m_cursorRow == m_lines.size() - 1;

    if (!text.isNull()) {
        const QStringList newLines = text.split(QLatin1Char('\n'));
        // The first append into an empty document replaces its empty line
        if (m_lines.size() == 1 && m_lines.first().isEmpty())
            m_lines = newLines;
        else
            m_lines += newLines;
    }

    // Remove lines from the top and keep the cursor and view on the same text
    const int excess = maxLines > 0 ? int(m_lines.size()) - maxLines : 0;
    if (excess > 0) {
        m_lines.erase(m_lines.begin(), m_lines.begin() + excess);
        m_cursorRow = qMax(0, m_cursorRow - excess);
        m_scrollRow = qMax(0, m_scrollRow - excess);
        clearSelection();
    }

    if (follow) {
        m_cursorRow = m_lines.size() - 1;
        m_cursorCol = m_lines.last().length();
        scrollToVisible();
    } else {
        m_cursorCol = qMin(m_cursorCol, int(m_lines.at(m_cursorRow).length()));
    }

    emit textChanged();
    markDirty();
}

int QTuiTextEdit::lineCount() const
{
    return m_lines.size();
}

void QTuiTextEdit::insertNewLine()
{
    if (m_readOnly)
        return;
    const QString &currentLine = m_lines.at(m_cursorRow);
    QString remainder = currentLine.mid(m_cursorCol);
    m_lines[m_cursorRow] = currentLine.left(m_cursorCol);
    m_lines.insert(m_cursorRow + 1, remainder);
    ++m_cursorRow;
    m_cursorCol = 0;
    emit textChanged();
}

void QTuiTextEdit::deleteBackward()
{
    if (m_readOnly)
        return;
    if (m_cursorCol > 0) {
        // Remove the whole preceding cluster (surrogate pair + marks)
        QString &line = m_lines[m_cursorRow];
        const int prev = QtTuiUnicode::prevClusterIndex(line, m_cursorCol);
        line.remove(prev, m_cursorCol - prev);
        m_cursorCol = prev;
        emit textChanged();
    } else if (m_cursorRow > 0) {
        // Merge current line with previous
        int prevLen = m_lines.at(m_cursorRow - 1).length();
        m_lines[m_cursorRow - 1] += m_lines.at(m_cursorRow);
        m_lines.removeAt(m_cursorRow);
        --m_cursorRow;
        m_cursorCol = prevLen;
        emit textChanged();
    }
}

void QTuiTextEdit::deleteForward()
{
    if (m_readOnly)
        return;
    if (m_cursorCol < m_lines.at(m_cursorRow).length()) {
        // Remove the whole following cluster (surrogate pair + marks)
        QString &line = m_lines[m_cursorRow];
        const int next = QtTuiUnicode::nextClusterIndex(line, m_cursorCol);
        line.remove(m_cursorCol, next - m_cursorCol);
        emit textChanged();
    } else if (m_cursorRow < m_lines.size() - 1) {
        // At line end: merge next line into current (mirror of Backspace merge)
        m_lines[m_cursorRow] += m_lines.at(m_cursorRow + 1);
        m_lines.removeAt(m_cursorRow + 1);
        emit textChanged();
    }
}

void QTuiTextEdit::deleteSelection()
{
    if (m_readOnly || !hasSelection())
        return;

    int sr, sc, er, ec;
    normalizeSelection(sr, sc, er, ec);

    if (sr == er) {
        m_lines[sr].remove(sc, ec - sc);
    } else {
        // Keep head of first line + tail of last line, drop the lines between
        m_lines[sr] = m_lines.at(sr).left(sc) + m_lines.at(er).mid(ec);
        for (int r = er; r > sr; --r)
            m_lines.removeAt(r);
    }

    m_cursorRow = sr;
    m_cursorCol = sc;
    clearSelection();
    emit textChanged();
}

void QTuiTextEdit::moveCursor(int dRow, int dCol)
{
    int newRow = qBound(0, m_cursorRow + dRow, m_lines.size() - 1);
    int newCol = m_cursorCol;

    if (dRow != 0) {
        // Vertical movement targets the desired ("sticky") display column so
        // that passing over a short line does not lose the position.
        newCol = QtTuiUnicode::indexOfColumn(m_lines.at(newRow), m_desiredCol);
    } else if (dCol < 0) {
        // Step left by one whole cluster, wrapping to the previous line end
        if (m_cursorCol > 0) {
            newCol = QtTuiUnicode::prevClusterIndex(m_lines.at(m_cursorRow), m_cursorCol);
        } else if (m_cursorRow > 0) {
            --newRow;
            newCol = m_lines.at(newRow).length();
        }
    } else if (dCol > 0) {
        // Step right by one whole cluster, wrapping to the next line start
        const QString &line = m_lines.at(m_cursorRow);
        if (m_cursorCol < line.length()) {
            newCol = QtTuiUnicode::nextClusterIndex(line, m_cursorCol);
        } else if (m_cursorRow < m_lines.size() - 1) {
            ++newRow;
            newCol = 0;
        }
    }

    // Clamp column to line length
    newCol = qBound(0, newCol, static_cast<int>(m_lines.at(newRow).length()));

    m_cursorRow = newRow;
    m_cursorCol = newCol;
}

void QTuiTextEdit::scrollToVisible()
{
    int visibleRows = geometry().height();
    if (visibleRows > 0) {
        if (m_cursorRow < m_scrollRow)
            m_scrollRow = m_cursorRow;
        else if (m_cursorRow >= m_scrollRow + visibleRows)
            m_scrollRow = m_cursorRow - visibleRows + 1;
    }

    int visibleCols = geometry().width();
    if (visibleCols > 0) {
        const QString &line = m_lines.at(m_cursorRow);
        const int cursorCol = QtTuiUnicode::columnOfIndex(line, m_cursorCol);
        int cursorWidth = 1;
        if (m_cursorCol < line.length()) {
            int i = m_cursorCol;
            QtTuiUnicode::Cluster c;
            if (QtTuiUnicode::nextCluster(line, i, c))
                cursorWidth = c.width;
        }
        if (cursorCol < m_scrollCol)
            m_scrollCol = cursorCol;
        else if (cursorCol + cursorWidth > m_scrollCol + visibleCols)
            m_scrollCol = cursorCol + cursorWidth - visibleCols;
    }
}

// Maximum number of undo steps
static constexpr int kMaxUndo = 100;

void QTuiTextEdit::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();
    if (key == Qt::Key_Z && mods == Qt::ControlModifier) {
        undo();
        return;
    }
    if (key == Qt::Key_A && mods == Qt::ControlModifier) {
        selectAll();
        return;
    }
    if ((key == Qt::Key_Y && mods == Qt::ControlModifier)
        || (key == Qt::Key_Z && mods == (Qt::ControlModifier | Qt::ShiftModifier))) {
        redo();
        return;
    }

    const QStringList before = m_lines;
    const int row = m_cursorRow;
    const int col = m_cursorCol;
    editKey(event);

    // A typed character: printable text, no Ctrl, nothing selected before
    const QString text = event->text();
    const bool typing = !text.isEmpty() && !(mods & Qt::ControlModifier)
        && (text.at(0).isPrint() || text.at(0).isHighSurrogate()) && text != QLatin1String(" ");
    recordEdit(before, row, col, typing);
}

void QTuiTextEdit::recordEdit(const QStringList &before, int row, int col, bool typing)
{
    if (before == m_lines)
        return;
    // A typed character right after the previous one joins its step
    if (!(typing && row == m_typingRow && col == m_typingCol)) {
        m_undoStack.append({before, row, col});
        if (m_undoStack.size() > kMaxUndo)
            m_undoStack.removeFirst();
    }
    m_redoStack.clear();
    m_typingRow = typing ? m_cursorRow : -1;
    m_typingCol = typing ? m_cursorCol : -1;
}

void QTuiTextEdit::restoreState(const QStringList &lines, int row, int col)
{
    m_lines = lines;
    m_cursorRow = qBound(0, row, int(m_lines.size()) - 1);
    m_cursorCol = qBound(0, col, int(m_lines.at(m_cursorRow).length()));
    m_typingRow = m_typingCol = -1;
    clearSelection();
    m_desiredCol = QtTuiUnicode::columnOfIndex(m_lines.at(m_cursorRow), m_cursorCol);
    scrollToVisible();
    emit textChanged();
    markDirty();
}

void QTuiTextEdit::undo()
{
    if (m_undoStack.isEmpty())
        return;
    m_redoStack.append({m_lines, m_cursorRow, m_cursorCol});
    const UndoState state = m_undoStack.takeLast();
    restoreState(state.lines, state.row, state.col);
}

void QTuiTextEdit::redo()
{
    if (m_redoStack.isEmpty())
        return;
    m_undoStack.append({m_lines, m_cursorRow, m_cursorCol});
    const UndoState state = m_redoStack.takeLast();
    restoreState(state.lines, state.row, state.col);
}

bool QTuiTextEdit::isUndoAvailable() const
{
    return !m_undoStack.isEmpty();
}

bool QTuiTextEdit::isRedoAvailable() const
{
    return !m_redoStack.isEmpty();
}

void QTuiTextEdit::editKey(TuiKeyEvent *event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();
    const bool shift = mods & Qt::ShiftModifier;

    // Clipboard: Ctrl+C copies the selection, Ctrl+X cuts it, Ctrl+V pastes
    if (mods == Qt::ControlModifier
        && (key == Qt::Key_C || key == Qt::Key_X || key == Qt::Key_V)) {
        if (key == Qt::Key_V) {
            insertPastedText(QTuiApplication::clipboard()->text());
            return; // insertPastedText() repaints
        }
        if (!hasSelection())
            return;
        QTuiApplication::clipboard()->setText(selectedText());
        if (key == Qt::Key_X && !m_readOnly) {
            deleteSelection();
            scrollToVisible();
            markDirty();
        }
        return;
    }

    switch (key) {
    case Qt::Key_Left:
        if (shift) startOrExtendSelection();
        else clearSelection();
        moveCursor(0, -1);
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    case Qt::Key_Right:
        if (shift) startOrExtendSelection();
        else clearSelection();
        moveCursor(0, 1);
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    case Qt::Key_Up:
        if (shift) startOrExtendSelection();
        else clearSelection();
        moveCursor(-1, 0);
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    case Qt::Key_Down:
        if (shift) startOrExtendSelection();
        else clearSelection();
        moveCursor(1, 0);
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    case Qt::Key_Home:
        if (shift) startOrExtendSelection();
        else clearSelection();
        m_cursorCol = 0;
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    case Qt::Key_End:
        if (shift) startOrExtendSelection();
        else clearSelection();
        m_cursorCol = m_lines.at(m_cursorRow).length();
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    case Qt::Key_PageUp: {
        if (shift) startOrExtendSelection();
        else clearSelection();
        int pageSize = qMax(1, geometry().height());
        moveCursor(-pageSize, 0);
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    }
    case Qt::Key_PageDown: {
        if (shift) startOrExtendSelection();
        else clearSelection();
        int pageSize = qMax(1, geometry().height());
        moveCursor(pageSize, 0);
        if (shift) { m_selEndRow = m_cursorRow; m_selEndCol = m_cursorCol; }
        break;
    }
    case Qt::Key_Backspace:
        if (hasSelection()) {
            deleteSelection(); // no-op when read-only
        } else {
            clearSelection();
            deleteBackward();
        }
        break;
    case Qt::Key_Delete:
        if (hasSelection()) {
            deleteSelection(); // no-op when read-only
        } else {
            clearSelection();
            deleteForward();
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        clearSelection();
        insertNewLine();
        break;
    default: {
        const QString text = event->text();
        // Accept printable characters; a leading high surrogate means the
        // parser delivered a non-BMP character (e.g. emoji) as a full pair.
        if (text.isEmpty() || !(text.at(0).isPrint() || text.at(0).isHighSurrogate()))
            return; // Unhandled key: no visible state change
        if (hasSelection())
            deleteSelection(); // typing replaces the selection (no-op when read-only)
        else
            clearSelection();
        insertText(text);
        break;
    }
    }

    // Vertical movement keeps the desired column; everything else resets it.
    const bool verticalMove = key == Qt::Key_Up || key == Qt::Key_Down
        || key == Qt::Key_PageUp || key == Qt::Key_PageDown;
    if (!verticalMove)
        m_desiredCol = QtTuiUnicode::columnOfIndex(m_lines.at(m_cursorRow), m_cursorCol);

    scrollToVisible();
    markDirty();
}

void QTuiTextEdit::mousePressEvent(TuiMouseEvent *event)
{
    // A press places the cursor; a drag with the left button selects from
    // there to the mouse
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
        return;

    const QRect geo = geometry();
    // Absolute screen coordinates -> widget-relative -> document position
    int row = event->y() - geo.y() + m_scrollRow;
    row = qBound(0, row, m_lines.size() - 1);
    const QString &line = m_lines.at(row);
    const int clickCol = event->x() - geo.x() + m_scrollCol; // display column
    const int col = (clickCol <= 0) ? 0 : QtTuiUnicode::indexOfColumn(line, clickCol);

    if (drag)
        startOrExtendSelection(); // the anchor is the cursor before the move
    m_cursorRow = row;
    m_cursorCol = col;
    m_desiredCol = QtTuiUnicode::columnOfIndex(line, col);
    if (drag) {
        m_selEndRow = m_cursorRow;
        m_selEndCol = m_cursorCol;
    } else {
        clearSelection();
    }
    scrollToVisible();
    markDirty();
}

QSize QTuiTextEdit::minimumSize() const
{
    return QSize(1, 1);
}

QPoint QTuiTextEdit::terminalCursorPos() const
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return QPoint(-1, -1);

    const int row = m_cursorRow - m_scrollRow;
    const int col = QtTuiUnicode::columnOfIndex(m_lines.at(m_cursorRow), m_cursorCol)
        - m_scrollCol;
    if (row < 0 || row >= geo.height() || col < 0 || col >= geo.width())
        return QPoint(-1, -1);
    return QPoint(geo.x() + col, geo.y() + row);
}

void QTuiTextEdit::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    const auto theme = themeProperties();
    // Determine selection range (normalized)
    int selSR = -1, selSC = -1, selER = -1, selEC = -1;
    bool hasSel = hasSelection();
    if (hasSel)
        normalizeSelection(selSR, selSC, selER, selEC);

    for (int row = 0; row < geo.height(); ++row) {
        const int lineIdx = m_scrollRow + row;
        const int py = geo.y() + row;
        if (py >= buffer.height())
            break;

        const QString line = (lineIdx < m_lines.size()) ? m_lines.at(lineIdx)
                                                        : QString();
        const int lineLen = line.length();
        const int lineCols = QtTuiUnicode::stringWidth(line);
        const bool focused = hasFocus();

        // Style for the cell holding code-unit index `textIdx` of this line
        // (indices past lineLen address the virtual cells after the text,
        // mirroring the pre-Unicode per-cell behavior).
        auto styleFor = [&](int textIdx) {
            bool inSelection = false;
            if (hasSel && lineIdx >= selSR && lineIdx <= selER) {
                if (selSR == selER) {
                    inSelection = (textIdx >= selSC && textIdx < selEC);
                } else if (lineIdx == selSR) {
                    inSelection = (textIdx >= selSC);
                } else if (lineIdx == selER) {
                    inSelection = (textIdx < selEC);
                } else {
                    inSelection = true;
                }
            }
            const bool isCursor = focused && lineIdx == m_cursorRow
                && textIdx == m_cursorCol;

            Cell style;
            style.bold = theme.bold;
            style.italic = theme.italic;
            style.underline = theme.underline;
            if (isCursor) {
                style.foreground = theme.focusForeground;
                style.background = theme.focusBackground;
                style.inverse = true;
            } else if (inSelection) {
                style.foreground = theme.focusBackground;
                style.background = theme.focusForeground;
                style.inverse = false;
            } else {
                style.foreground = theme.foreground;
                style.background = theme.background;
                style.inverse = false;
            }
            return style;
        };

        // Fill the row with spaces first. Cells within the text's column
        // span are overwritten by the cluster pass below; cells after the
        // text use a virtual index so the end-of-line cursor and selection
        // highlighting keep their pre-Unicode behavior.
        for (int col = 0; col < geo.width(); ++col) {
            const int px = geo.x() + col;
            if (px >= buffer.width())
                break;
            const int displayCol = m_scrollCol + col;
            const int textIdx = lineLen + (displayCol - lineCols);
            buffer.backCell(px, py) = styleFor(textIdx);
        }

        // Draw the visible clusters at their display columns
        int col = -m_scrollCol;
        int i = 0;
        QtTuiUnicode::Cluster c;
        while (col < geo.width() && QtTuiUnicode::nextCluster(line, i, c)) {
            const int nextCol = col + c.width;
            if (nextCol <= 0) {
                col = nextCol;
                continue;
            }
            const Cell style = styleFor(c.index);
            if (col >= 0 && nextCol <= geo.width() && geo.x() + nextCol <= buffer.width()) {
                buffer.putChar(geo.x() + col, py, c.base, c.combining, style);
            } else {
                // A wide cluster straddling the viewport edge: show its
                // visible half as a styled space
                for (int x = qMax(0, col); x < qMin(geo.width(), nextCol); ++x) {
                    if (geo.x() + x < buffer.width())
                        buffer.putChar(geo.x() + x, py, U' ', style);
                }
            }
            col = nextCol;
        }
    }
}

} // namespace QtTUI
