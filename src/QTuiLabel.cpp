#include <QtTUI/QTuiLabel.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QHash>
#include <QRegularExpression>
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
        updateLinks();
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
    if (m_buddy == buddy)
        return;
    disconnect(m_buddyDestroyed);
    m_buddy = buddy;
    // The & shows again when the buddy goes away
    if (buddy)
        m_buddyDestroyed = connect(buddy, &QObject::destroyed, this, [this]() { markDirty(); });
    markDirty();
}

QChar QTuiLabel::mnemonic() const
{
    QString shown;
    return parse(&shown, nullptr);
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
    updateLinks();
    markDirty();
}

QString QTuiLabel::displayText() const
{
    QString shown;
    parse(&shown, nullptr);
    return shown;
}

// Reads Markdown (a small subset) into text, styles and link targets
static void parseMarkdown(const QString &in, QString *out, QVector<quint16> *styles,
                          QStringList *links)
{
    quint8 current = 0;
    int linkCount = 0;
    int link = 0;     // number of the open link (from 1), or 0
    int linkEnd = -1; // index of the "]" that closes its text
    auto put = [&](QChar ch) {
        out->append(ch);
        if (styles)
            styles->append(quint16(current | (link << 8)));
    };
    // A marker counts only if the same marker closes it later in the paragraph
    auto closes = [&](int from, const QString &marker) {
        const int end = in.indexOf(QLatin1Char('\n'), from);
        const int found = in.indexOf(marker, from);
        return found >= 0 && (end < 0 || found < end);
    };
    for (int i = 0; i < in.length(); ++i) {
        const QChar ch = in.at(i);
        if (i == linkEnd) {
            // The end of the link text: go over "](url)". The url is not
            // text, so its * and _ are not markup.
            i = in.indexOf(QLatin1Char(')'), i);
            link = 0;
            linkEnd = -1;
            continue;
        }
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
        if (ch == QLatin1Char('[') && link == 0) {
            // [text](url) on one line: the text is not empty and has no
            // brackets, and the url has no space
            const int eol = in.indexOf(QLatin1Char('\n'), i);
            const int close = in.indexOf(QLatin1Char(']'), i + 1);
            const bool text = close >= i + 2 && in.mid(close, 2) == QLatin1String("](")
                && !in.mid(i + 1, close - i - 1).contains(QLatin1Char('['));
            const int paren = text ? in.indexOf(QLatin1Char(')'), close + 2) : -1;
            const QString url = in.mid(close + 2, paren - close - 2);
            if (paren >= 0 && (eol < 0 || paren < eol) && !url.contains(QLatin1Char(' '))) {
                if (links)
                    links->append(url);
                // ponytail: the style has one byte for the link number, so
                // links after number 255 act as link 255. Use a wider style
                // if a label needs more.
                link = qMin(++linkCount, 255);
                linkEnd = close;
                continue;
            }
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

// Reads a small HTML subset into text, styles and link targets
static void parseHtml(const QString &in, QString *out, QVector<quint16> *styles,
                      QStringList *links)
{
    int count[5] = {0, 0, 0, 0, 0}; // open tags per style bit
    int linkCount = 0;
    int link = 0; // number of the open link (from 1), or 0
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
            styles->append(quint16(bits() | (link << 8)));
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
            // The tag in its original case: a link target keeps its case
            const QString raw = in.mid(i + 1, end - i - 1);
            QString tag = raw.trimmed().toLower();
            i = end;
            const bool closing = tag.startsWith(QLatin1Char('/'));
            if (closing)
                tag = tag.mid(1);
            tag = tag.section(QLatin1Char(' '), 0, 0).remove(QLatin1Char('/'));
            if (tag == QLatin1String("br") || (closing && tag == QLatin1String("p"))) {
                put(QLatin1Char('\n'));
            } else if (tag == QLatin1String("a")) {
                // <a href="url">: a link. An <a> without href is not a link.
                static const QRegularExpression href(
                    QStringLiteral("\\bhref\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)')"),
                    QRegularExpression::CaseInsensitiveOption);
                const QRegularExpressionMatch m = closing ? QRegularExpressionMatch() : href.match(raw);
                link = 0;
                if (m.hasMatch()) {
                    if (links)
                        links->append(m.captured(1) + m.captured(2));
                    link = qMin(++linkCount, 255);
                }
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

QChar QTuiLabel::parse(QString *shown, QVector<quint16> *styles, QStringList *links) const
{
    shown->clear();
    if (styles)
        styles->clear();
    switch (m_textFormat) {
    case Qt::MarkdownText:
        parseMarkdown(m_text, shown, styles, links);
        break;
    case Qt::RichText:
        parseHtml(m_text, shown, styles, links);
        break;
    default:
        *shown = m_text;
        if (styles)
            styles->fill(0, m_text.length());
        break;
    }

    // With a buddy, &x marks the mnemonic x, and && is one &
    QChar mnemonic;
    if (!m_buddy || m_textFormat == Qt::RichText)
        return mnemonic;
    for (int i = 0; i + 1 < shown->length(); ++i) {
        if (shown->at(i) != QLatin1Char('&'))
            continue;
        // Remove the &; the loop then steps over the character after it
        shown->remove(i, 1);
        if (styles)
            styles->remove(i);
        if (shown->at(i) != QLatin1Char('&') && mnemonic.isNull()) {
            mnemonic = shown->at(i);
            if (styles)
                (*styles)[i] |= Underline;
        }
    }
    return mnemonic;
}

QSize QTuiLabel::minimumSize() const
{
    const QString text = displayText();
    const int frame = 2 * frameWidth();
    if (text.isEmpty())
        return QSize(frame, 1 + frame);

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

    return QSize(width + frame, int(textLines.size()) + frame);
}

void QTuiLabel::updateLinks()
{
    const bool hadLinks = !m_links.isEmpty();
    QString shown;
    m_links.clear();
    parse(&shown, nullptr, &m_links);
    m_currentLink = 0;
    // A label with links takes the focus, so that the keys reach the links
    if (hadLinks != !m_links.isEmpty())
        setFocusPolicy(m_links.isEmpty() ? Qt::NoFocus : Qt::StrongFocus);
}

void QTuiLabel::keyPressEvent(TuiKeyEvent *event)
{
    if (m_links.isEmpty())
        return;
    int link = m_currentLink;
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Down:
    case Qt::Key_Tab:
        link += (event->modifiers() & Qt::ShiftModifier) ? -1 : 1;
        break;
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Backtab:
        --link;
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit linkActivated(m_links.at(m_currentLink));
        return;
    default:
        return;
    }
    link = qBound(0, link, int(m_links.size()) - 1);
    if (link == m_currentLink)
        return;
    m_currentLink = link;
    markDirty();
}

void QTuiLabel::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press || event->button() != Qt::LeftButton)
        return;
    // Find the link in the cell under the mouse
    int link = 0;
    layoutText([&](int x, int y, const QtTuiUnicode::Cluster &c, quint16 style) {
        if (y == event->y() && event->x() >= x && event->x() < x + c.width)
            link = style >> 8;
    });
    if (link == 0)
        return;
    if (m_currentLink != link - 1) {
        m_currentLink = link - 1;
        markDirty();
    }
    emit linkActivated(m_links.at(m_currentLink));
}

void QTuiLabel::layoutText(const std::function<void(int x, int y, const QtTuiUnicode::Cluster &c,
                                                    quint16 style)> &visit) const
{
    const QRect area = contentsRect();
    if (area.isEmpty())
        return;

    // Get wrapped lines
    QString shown;
    QVector<quint16> styles;
    parse(&shown, &styles);
    QStringList lines = wrapText(shown, area.width());
    // Position in `shown` of the next character to match with the lines;
    // wrapping drops or joins spaces, so spaces in `shown` are skipped
    int src = 0;
    if (lines.isEmpty())
        return;

    // Vertical alignment: determine starting row
    int totalLines = lines.size();
    int startRow = 0;
    if (m_alignment & Qt::AlignVCenter) {
        startRow = (area.height() - totalLines) / 2;
    } else if (m_alignment & Qt::AlignBottom) {
        startRow = area.height() - totalLines;
    }
    // AlignTop: startRow stays 0
    startRow = qMax(0, startRow);

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        int row = startRow + lineIdx;
        if (row >= area.height())
            break;

        const QString &line = lines.at(lineIdx);
        const int textCols = qMin(QtTuiUnicode::stringWidth(line), area.width());

        // Horizontal alignment: determine starting column (display columns)
        int startCol = 0;
        if (m_alignment & Qt::AlignHCenter) {
            startCol = (area.width() - textCols) / 2;
        } else if (m_alignment & Qt::AlignRight) {
            startCol = area.width() - textCols;
        }
        // AlignLeft: startCol stays 0
        startCol = qMax(0, startCol);

        int col = startCol;
        int i = 0;
        QtTuiUnicode::Cluster c;
        // The source of the line starts after spaces and line breaks
        while (src < shown.length() && (shown.at(src) == QLatin1Char(' ') || shown.at(src) == QLatin1Char('\n')))
            ++src;
        bool full = false; // a cluster did not fit: the rest of the line is cut
        while (QtTuiUnicode::nextCluster(line, i, c)) {
            // Match the cluster with its source character for the style
            quint16 bits = 0;
            if (c.base == U' ') {
                // A space has the style of the first source space
                if (src < shown.length() && shown.at(src) == QLatin1Char(' '))
                    bits = styles.value(src);
                while (src < shown.length() && shown.at(src) == QLatin1Char(' '))
                    ++src;
            } else if (src < shown.length()) {
                bits = styles.value(src);
                src += i - c.index;
            }
            // A wide character that goes past the right edge does not show
            if (full || col + c.width > area.width()) {
                full = true; // keep matching, so the next line starts right
                continue;
            }
            visit(area.x() + col, area.y() + row, c, bits);
            col += c.width;
        }
    }
}

void QTuiLabel::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    // Determine colors based on focus state. A label with links keeps its
    // colors; only the current link shows the focus.
    const bool focusColors = hasFocus() && m_links.isEmpty();
    const Color fg = focusColors ? theme.focusForeground : theme.foreground;
    const Color bg = focusColors ? theme.focusBackground : theme.background;

    // Clear the label area with background
    fillRect(buffer, geo, cellStyle(theme, fg, bg));
    if (frameShape() == Box)
        drawBorder(buffer, geo, theme.borderStyle, cellStyle(theme, fg, bg));

    const Cell style = cellStyle(theme, fg, bg);
    const bool focused = hasFocus();
    layoutText([&](int x, int y, const QtTuiUnicode::Cluster &c, quint16 bits) {
        // A character past the buffer edge does not show
        if (y >= buffer.height() || x + c.width > buffer.width())
            return;
        Cell styled = style;
        styled.bold = style.bold || (bits & Bold);
        styled.italic = style.italic || (bits & Italic);
        styled.underline = style.underline || (bits & Underline);
        styled.strikethrough = bits & Strike;
        styled.inverse = bits & Code;
        if (const int link = bits >> 8) {
            // A link: underlined in the accent color. The current link of
            // a label with focus shows in the focus colors.
            styled.underline = true;
            styled.foreground = theme.focusBackground;
            if (focused && link - 1 == m_currentLink) {
                styled.foreground = theme.focusForeground;
                styled.background = theme.focusBackground;
            }
        }
        buffer.putChar(x, y, c.base, c.combining, styled);
    });
}

} // namespace QtTUI
