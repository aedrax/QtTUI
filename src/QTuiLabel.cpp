#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiLabel::QTuiLabel(const QString &text, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_text(text)
{
}

QString QTuiLabel::text() const
{
    return m_text;
}

void QTuiLabel::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged(m_text);
        markDirty();
    }
}

Qt::Alignment QTuiLabel::alignment() const
{
    return m_alignment;
}

void QTuiLabel::setAlignment(Qt::Alignment alignment)
{
    if (m_alignment == alignment)
        return;
    m_alignment = alignment;
    markDirty();
}

bool QTuiLabel::wordWrap() const
{
    return m_wordWrap;
}

void QTuiLabel::setWordWrap(bool on)
{
    if (m_wordWrap == on)
        return;
    m_wordWrap = on;
    markDirty();
}

QTuiWidget *QTuiLabel::buddy() const
{
    return m_buddy;
}

void QTuiLabel::setBuddy(QTuiWidget *buddy)
{
    m_buddy = buddy;
}

QStringList QTuiLabel::wrapText(const QString &text, int width) const
{
    QStringList lines;
    if (width <= 0 || text.isEmpty())
        return lines;

    if (!m_wordWrap) {
        lines.append(text);
        return lines;
    }

    // Word-wrap: break at word boundaries
    const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QString currentLine;

    for (const QString &word : words) {
        if (currentLine.isEmpty()) {
            // If a single word exceeds width, force it onto its own line
            currentLine = word;
        } else if (QtTuiUnicode::stringWidth(currentLine) + 1
                       + QtTuiUnicode::stringWidth(word) <= width) {
            currentLine += QLatin1Char(' ') + word;
        } else {
            lines.append(currentLine);
            currentLine = word;
        }
    }
    if (!currentLine.isEmpty())
        lines.append(currentLine);

    return lines;
}

QSize QTuiLabel::minimumSize() const
{
    if (m_text.isEmpty())
        return QSize(0, 1);

    if (!m_wordWrap)
        return QSize(QtTuiUnicode::stringWidth(m_text), 1);

    // With word wrap, minimum width is the longest word, height depends on wrapping
    const QStringList words = m_text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    int maxWordLen = 0;
    for (const QString &w : words)
        maxWordLen = qMax(maxWordLen, QtTuiUnicode::stringWidth(w));

    return QSize(maxWordLen, 1);
}

void QTuiLabel::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    // Determine colors based on focus state
    const Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    const Color bg = hasFocus() ? theme.focusBackground : theme.background;

    // Clear the label area with background
    for (int row = 0; row < geoH; ++row) {
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            int py = geoY + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }

    // Get wrapped lines
    QStringList lines = wrapText(m_text, geoW);
    if (lines.isEmpty())
        return;

    // Vertical alignment: determine starting row
    int totalLines = lines.size();
    int startRow = 0;
    if (m_alignment & Qt::AlignVCenter) {
        startRow = (geoH - totalLines) / 2;
    } else if (m_alignment & Qt::AlignBottom) {
        startRow = geoH - totalLines;
    }
    // AlignTop: startRow stays 0
    startRow = qMax(0, startRow);

    // Render each line
    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        int row = startRow + lineIdx;
        if (row >= geoH)
            break;

        const QString &line = lines.at(lineIdx);
        const int textCols = qMin(QtTuiUnicode::stringWidth(line), geoW);

        // Horizontal alignment: determine starting column (display columns)
        int startCol = 0;
        if (m_alignment & Qt::AlignHCenter) {
            startCol = (geoW - textCols) / 2;
        } else if (m_alignment & Qt::AlignRight) {
            startCol = geoW - textCols;
        }
        // AlignLeft: startCol stays 0
        startCol = qMax(0, startCol);

        // Write clusters until the widget (or buffer) edge; a wide character
        // straddling the right edge is left as the background space.
        Cell style;
        style.foreground = fg;
        style.background = bg;
        style.bold = theme.bold;
        style.italic = theme.italic;
        style.underline = theme.underline;

        const int py = geoY + row;
        if (py >= buffer.height())
            continue;
        int col = startCol;
        int i = 0;
        QtTuiUnicode::Cluster c;
        while (QtTuiUnicode::nextCluster(line, i, c)) {
            if (col + c.width > geoW || geoX + col + c.width > buffer.width())
                break;
            buffer.putChar(geoX + col, py, c.base, c.combining, style);
            col += c.width;
        }
    }
}

} // namespace QtTUI
