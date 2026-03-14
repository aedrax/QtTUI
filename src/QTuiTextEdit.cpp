#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiTextEdit::QTuiTextEdit(QTuiWidget *parent)
    : QTuiWidget(parent)
{
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
    clearSelection();
    emit textChanged();
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

void QTuiTextEdit::insertChar(QChar ch)
{
    if (m_readOnly)
        return;
    m_lines[m_cursorRow].insert(m_cursorCol, ch);
    ++m_cursorCol;
    emit textChanged();
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
        m_lines[m_cursorRow].remove(m_cursorCol - 1, 1);
        --m_cursorCol;
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

void QTuiTextEdit::moveCursor(int dRow, int dCol)
{
    int newRow = m_cursorRow + dRow;
    int newCol = m_cursorCol + dCol;

    // Clamp row
    newRow = qBound(0, newRow, m_lines.size() - 1);

    // Handle column wrapping at line boundaries
    if (dCol != 0 && dRow == 0) {
        if (newCol < 0) {
            // Wrap to end of previous line
            if (m_cursorRow > 0) {
                --newRow;
                newCol = m_lines.at(newRow).length();
            } else {
                newCol = 0;
            }
        } else if (newCol > m_lines.at(m_cursorRow).length()) {
            // Wrap to start of next line
            if (m_cursorRow < m_lines.size() - 1) {
                ++newRow;
                newCol = 0;
            } else {
                newCol = m_lines.at(m_cursorRow).length();
            }
        }
    }

    // Clamp column to line length
    newCol = qBound(0, newCol, m_lines.at(newRow).length());

    m_cursorRow = newRow;
    m_cursorCol = newCol;
}

void QTuiTextEdit::scrollToVisible()
{
    int visibleRows = geometry().height();
    if (visibleRows <= 0)
        return;

    if (m_cursorRow < m_scrollRow)
        m_scrollRow = m_cursorRow;
    else if (m_cursorRow >= m_scrollRow + visibleRows)
        m_scrollRow = m_cursorRow - visibleRows + 1;
}

void QTuiTextEdit::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();
    const bool shift = mods & Qt::ShiftModifier;

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
        clearSelection();
        deleteBackward();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        clearSelection();
        insertNewLine();
        break;
    default:
        if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
            clearSelection();
            insertChar(event->text().at(0));
        }
        break;
    }

    scrollToVisible();
}

QSize QTuiTextEdit::minimumSize() const
{
    return QSize(1, 1);
}

void QTuiTextEdit::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    const auto theme = themeProperties();
    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    // Determine selection range (normalized)
    int selSR = -1, selSC = -1, selER = -1, selEC = -1;
    bool hasSel = hasSelection();
    if (hasSel)
        normalizeSelection(selSR, selSC, selER, selEC);

    for (int row = 0; row < geoH; ++row) {
        int lineIdx = m_scrollRow + row;
        int py = geoY + row;
        if (py >= buffer.height())
            break;

        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px >= buffer.width())
                break;

            Cell &cell = buffer.backCell(px, py);

            // Determine character
            if (lineIdx < m_lines.size() && col < m_lines.at(lineIdx).length()) {
                cell.character = m_lines.at(lineIdx).at(col);
            } else {
                cell.character = QLatin1Char(' ');
            }

            cell.bold = theme.bold;
            cell.italic = theme.italic;
            cell.underline = theme.underline;

            // Check if this cell is in the selection
            bool inSelection = false;
            if (hasSel && lineIdx >= selSR && lineIdx <= selER) {
                if (selSR == selER) {
                    inSelection = (col >= selSC && col < selEC);
                } else if (lineIdx == selSR) {
                    inSelection = (col >= selSC);
                } else if (lineIdx == selER) {
                    inSelection = (col < selEC);
                } else {
                    inSelection = true;
                }
            }

            // Check if this is the cursor position
            bool isCursor = hasFocus() && lineIdx == m_cursorRow && col == m_cursorCol;

            if (isCursor) {
                cell.foreground = theme.focusForeground;
                cell.background = theme.focusBackground;
                cell.inverse = true;
            } else if (inSelection) {
                cell.foreground = theme.focusBackground;
                cell.background = theme.focusForeground;
                cell.inverse = false;
            } else {
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.inverse = false;
            }
        }
    }
}

} // namespace QtTUI
