#include <QtTUI/QTuiLabel.h>
#include <QHash>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiLabel::QTuiLabel(const QString &text, QTuiWidget *parent)
    : QTuiFrame(parent)
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

    // Each newline starts a new line
    const QStringList textLines = text.split(QLatin1Char('\n'));
    if (!m_wordWrap)
        return textLines;

    // Word-wrap: break each line at word boundaries
    for (const QString &textLine : textLines) {
        const QStringList words = textLine.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (words.isEmpty()) {
            lines.append(QString());
            continue;
        }

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
        lines.append(currentLine);
    }

    return lines;
}

Qt::TextFormat QTuiLabel::textFormat() const
{
    return m_textFormat;
}

void QTuiLabel::setTextFormat(Qt::TextFormat format)
{
    if (m_textFormat == format)
        return;
    m_textFormat = format;
    markDirty();
}

QString QTuiLabel::displayText() const
{
    QString shown;
    parse(&shown, nullptr);
    return shown;
}

// Reads Markdown (a small subset) into text and style bits
static void parseMarkdown(const QString &in, QString *out, QVector<quint8> *styles)
{
    quint8 current = 0;
    auto put = [&](QChar ch) {
        out->append(ch);
        if (styles)
            styles->append(current);
    };
    // A marker counts only if the same marker closes it later in the paragraph
    auto closes = [&](int from, const QString &marker) {
        const int end = in.indexOf(QLatin1Char('\n'), from);
        const int found = in.indexOf(marker, from);
        return found >= 0 && (end < 0 || found < end);
    };
    for (int i = 0; i < in.length(); ++i) {
        const QChar ch = in.at(i);
        if (current & 16) { // inside `code`: no other markup
            if (ch == QLatin1Char('`'))
                current &= ~16;
            else
                put(ch);
            continue;
        }
        if (ch == QLatin1Char('\\') && i + 1 < in.length()) {
            put(in.at(++i));
            continue;
        }
        const QString two = in.mid(i, 2);
        if (two == QLatin1String("**") || two == QLatin1String("__")) {
            if ((current & 1) || closes(i + 2, two)) {
                current ^= 1;
                ++i;
                continue;
            }
        } else if (two == QLatin1String("~~")) {
            if ((current & 8) || closes(i + 2, two)) {
                current ^= 8;
                ++i;
                continue;
            }
        } else if (ch == QLatin1Char('*') || ch == QLatin1Char('_')) {
            if ((current & 2) || closes(i + 1, QString(ch))) {
                current ^= 2;
                continue;
            }
        } else if (ch == QLatin1Char('`')) {
            if (closes(i + 1, QStringLiteral("`"))) {
                current |= 16;
                continue;
            }
        }
        put(ch);
    }
}

// Reads a small HTML subset into text and style bits
static void parseHtml(const QString &in, QString *out, QVector<quint8> *styles)
{
    int count[5] = {0, 0, 0, 0, 0}; // open tags per style bit
    auto bits = [&]() {
        quint8 b = 0;
        for (int k = 0; k < 5; ++k)
            if (count[k] > 0)
                b |= quint8(1 << k);
        return b;
    };
    auto put = [&](QChar ch) {
        out->append(ch);
        if (styles)
            styles->append(bits());
    };
    static const QHash<QString, int> tagBit = {
        {QStringLiteral("b"), 0}, {QStringLiteral("strong"), 0}, {QStringLiteral("i"), 1},
        {QStringLiteral("em"), 1}, {QStringLiteral("u"), 2}, {QStringLiteral("s"), 3},
        {QStringLiteral("del"), 3}, {QStringLiteral("code"), 4}};
    static const QHash<QString, QChar> entities = {
        {QStringLiteral("lt"), QLatin1Char('<')}, {QStringLiteral("gt"), QLatin1Char('>')},
        {QStringLiteral("amp"), QLatin1Char('&')}, {QStringLiteral("quot"), QLatin1Char('"')},
        {QStringLiteral("#39"), QLatin1Char('\'')}, {QStringLiteral("nbsp"), QLatin1Char(' ')}};
    for (int i = 0; i < in.length(); ++i) {
        const QChar ch = in.at(i);
        if (ch == QLatin1Char('<')) {
            const int end = in.indexOf(QLatin1Char('>'), i);
            if (end < 0) {
                put(ch);
                continue;
            }
            QString tag = in.mid(i + 1, end - i - 1).trimmed().toLower();
            i = end;
            const bool closing = tag.startsWith(QLatin1Char('/'));
            if (closing)
                tag = tag.mid(1);
            tag = tag.section(QLatin1Char(' '), 0, 0).remove(QLatin1Char('/'));
            if (tag == QLatin1String("br") || (closing && tag == QLatin1String("p"))) {
                put(QLatin1Char('\n'));
            } else if (tagBit.contains(tag)) {
                int &n = count[tagBit.value(tag)];
                n = qMax(0, n + (closing ? -1 : 1));
            }
            continue;
        }
        if (ch == QLatin1Char('&')) {
            const int end = in.indexOf(QLatin1Char(';'), i);
            if (end > i && entities.contains(in.mid(i + 1, end - i - 1))) {
                put(entities.value(in.mid(i + 1, end - i - 1)));
                i = end;
                continue;
            }
        }
        put(ch);
    }
    // A trailing line break from </p> is not shown
    while (out->endsWith(QLatin1Char('\n'))) {
        out->chop(1);
        if (styles)
            styles->removeLast();
    }
}

void QTuiLabel::parse(QString *shown, QVector<quint8> *styles) const
{
    shown->clear();
    if (styles)
        styles->clear();
    switch (m_textFormat) {
    case Qt::MarkdownText:
        parseMarkdown(m_text, shown, styles);
        break;
    case Qt::RichText:
        parseHtml(m_text, shown, styles);
        break;
    default:
        *shown = m_text;
        if (styles)
            styles->fill(0, m_text.length());
        break;
    }
}

QSize QTuiLabel::minimumSize() const
{
    const QString text = displayText();
    if (text.isEmpty())
        return QSize(0, 1);

    // Each newline starts a new line
    const QStringList textLines = text.split(QLatin1Char('\n'));
    int width = 0;
    for (const QString &line : textLines) {
        if (!m_wordWrap) {
            width = qMax(width, QtTuiUnicode::stringWidth(line));
            continue;
        }
        // With word wrap, minimum width is the longest word
        const QStringList words = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &w : words)
            width = qMax(width, QtTuiUnicode::stringWidth(w));
    }

    return QSize(width, int(textLines.size()));
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
    QString shown;
    QVector<quint8> styles;
    parse(&shown, &styles);
    QStringList lines = wrapText(shown, geoW);
    // Position in `shown` of the next character to match with the lines;
    // wrapping drops or joins spaces, so spaces in `shown` are skipped
    int src = 0;
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
        // The source of the line starts after spaces and line breaks
        while (src < shown.length() && (shown.at(src) == QLatin1Char(' ') || shown.at(src) == QLatin1Char('\n')))
            ++src;
        while (QtTuiUnicode::nextCluster(line, i, c)) {
            // Match the cluster with its source character for the style
            quint8 bits = 0;
            if (c.base == U' ') {
                while (src < shown.length() && shown.at(src) == QLatin1Char(' '))
                    ++src;
            } else if (src < shown.length()) {
                bits = styles.value(src);
                src += i - c.index;
            }
            if (col + c.width > geoW || geoX + col + c.width > buffer.width())
                continue; // keep matching, so the next line starts right
            Cell styled = style;
            styled.bold = style.bold || (bits & Bold);
            styled.italic = style.italic || (bits & Italic);
            styled.underline = style.underline || (bits & Underline);
            styled.strikethrough = bits & Strike;
            styled.inverse = bits & Code;
            buffer.putChar(geoX + col, py, c.base, c.combining, styled);
            col += c.width;
        }
    }
}

} // namespace QtTUI
