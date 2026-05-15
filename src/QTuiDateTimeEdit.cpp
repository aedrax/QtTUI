#include <QtTUI/QTuiDateTimeEdit.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiDateTimeEdit::QTuiDateTimeEdit(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_dateTime(QDateTime::currentDateTime())
{
    setFocusPolicy(Qt::StrongFocus);
}

QTuiDateTimeEdit::QTuiDateTimeEdit(const QDateTime &dateTime, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_dateTime(dateTime.isValid() ? dateTime : QDateTime::currentDateTime())
{
    setFocusPolicy(Qt::StrongFocus);
}

QDateTime QTuiDateTimeEdit::dateTime() const
{
    return m_dateTime;
}

void QTuiDateTimeEdit::setDateTime(const QDateTime &dateTime)
{
    if (!dateTime.isValid())
        return;

    QDateTime clamped = dateTime;
    if (m_minimum.isValid() && clamped < m_minimum)
        clamped = m_minimum;
    if (m_maximum.isValid() && clamped > m_maximum)
        clamped = m_maximum;

    if (m_dateTime != clamped) {
        m_dateTime = clamped;
        emit dateTimeChanged(m_dateTime);
        markDirty();
    }
}

QDate QTuiDateTimeEdit::date() const
{
    return m_dateTime.date();
}

void QTuiDateTimeEdit::setDate(const QDate &date)
{
    if (date.isValid())
        setDateTime(QDateTime(date, m_dateTime.time()));
}

QTime QTuiDateTimeEdit::time() const
{
    return m_dateTime.time();
}

void QTuiDateTimeEdit::setTime(const QTime &time)
{
    if (time.isValid())
        setDateTime(QDateTime(m_dateTime.date(), time));
}

QString QTuiDateTimeEdit::displayFormat() const
{
    return m_displayFormat;
}

void QTuiDateTimeEdit::setDisplayFormat(const QString &format)
{
    if (!format.isEmpty() && m_displayFormat != format) {
        m_displayFormat = format;
        markDirty();
    }
}

void QTuiDateTimeEdit::setMinimumDateTime(const QDateTime &min)
{
    m_minimum = min;
    if (m_minimum.isValid() && m_maximum.isValid() && m_minimum > m_maximum)
        m_maximum = m_minimum;
    // Re-clamp current value
    if (m_minimum.isValid() && m_dateTime < m_minimum)
        setDateTime(m_minimum);
}

void QTuiDateTimeEdit::setMaximumDateTime(const QDateTime &max)
{
    m_maximum = max;
    if (m_minimum.isValid() && m_maximum.isValid() && m_maximum < m_minimum)
        m_minimum = m_maximum;
    // Re-clamp current value
    if (m_maximum.isValid() && m_dateTime > m_maximum)
        setDateTime(m_maximum);
}

QDateTime QTuiDateTimeEdit::minimumDateTime() const
{
    return m_minimum;
}

QDateTime QTuiDateTimeEdit::maximumDateTime() const
{
    return m_maximum;
}

bool QTuiDateTimeEdit::calendarPopup() const
{
    return m_calendarPopup;
}

void QTuiDateTimeEdit::setCalendarPopup(bool enable)
{
    m_calendarPopup = enable;
}

QList<QTuiDateTimeEdit::Section> QTuiDateTimeEdit::displaySections() const
{
    return { Year, Month, Day, Hour, Minute, Second };
}

QList<QTuiDateTimeEdit::SectionRange> QTuiDateTimeEdit::parseSectionRanges() const
{
    QList<SectionRange> ranges;
    const auto sections = displaySections();
    const QString formatted = m_dateTime.toString(m_displayFormat);

    // Map format tokens to sections and find their positions in the formatted string
    // We scan the format string for known tokens and track their output positions
    int outPos = 0;
    int i = 0;
    while (i < m_displayFormat.length()) {
        // Try to match known format tokens at position i
        struct TokenInfo {
            QString token;
            Section section;
        };
        // Order matters: longer tokens first
        static const TokenInfo tokens[] = {
            { QStringLiteral("yyyy"), Year },
            { QStringLiteral("yy"),   Year },
            { QStringLiteral("MM"),   Month },
            { QStringLiteral("dd"),   Day },
            { QStringLiteral("HH"),   Hour },
            { QStringLiteral("hh"),   Hour },
            { QStringLiteral("mm"),   Minute },
            { QStringLiteral("ss"),   Second },
        };

        bool matched = false;
        for (const auto &tok : tokens) {
            if (m_displayFormat.mid(i, tok.token.length()) == tok.token) {
                // Check if this section is in our displaySections list
                if (sections.contains(tok.section)) {
                    // Determine the output length for this token
                    int outputLen = tok.token.length();
                    // yyyy -> 4, yy -> 2, MM -> 2, dd -> 2, HH -> 2, hh -> 2, mm -> 2, ss -> 2
                    ranges.append({ tok.section, outPos, outputLen });
                }
                outPos += tok.token.length();
                i += tok.token.length();
                matched = true;
                break;
            }
        }

        if (!matched) {
            // Literal character - skip it
            ++outPos;
            ++i;
        }
    }

    return ranges;
}

void QTuiDateTimeEdit::stepSection(int delta)
{
    const auto sections = displaySections();
    if (m_currentSection < 0 || m_currentSection >= sections.size())
        return;

    Section sec = sections.at(m_currentSection);
    QDate d = m_dateTime.date();
    QTime t = m_dateTime.time();

    switch (sec) {
    case Year:
        d = d.addYears(delta);
        break;
    case Month: {
        int newMonth = d.month() + delta;
        int newYear = d.year();
        while (newMonth > 12) { newMonth -= 12; ++newYear; }
        while (newMonth < 1)  { newMonth += 12; --newYear; }
        int maxDay = QDate(newYear, newMonth, 1).daysInMonth();
        d = QDate(newYear, newMonth, qMin(d.day(), maxDay));
        break;
    }
    case Day:
        d = d.addDays(delta);
        break;
    case Hour: {
        int h = t.hour() + delta;
        while (h >= 24) h -= 24;
        while (h < 0)   h += 24;
        t = QTime(h, t.minute(), t.second());
        break;
    }
    case Minute: {
        int m = t.minute() + delta;
        while (m >= 60) m -= 60;
        while (m < 0)   m += 60;
        t = QTime(t.hour(), m, t.second());
        break;
    }
    case Second: {
        int s = t.second() + delta;
        while (s >= 60) s -= 60;
        while (s < 0)   s += 60;
        t = QTime(t.hour(), t.minute(), s);
        break;
    }
    }

    if (d.isValid() && t.isValid())
        setDateTime(QDateTime(d, t));
}

void QTuiDateTimeEdit::nextSection()
{
    const auto sections = displaySections();
    if (m_currentSection < sections.size() - 1) {
        ++m_currentSection;
        markDirty();
    }
}

void QTuiDateTimeEdit::prevSection()
{
    if (m_currentSection > 0) {
        --m_currentSection;
        markDirty();
    }
}

QSize QTuiDateTimeEdit::minimumSize() const
{
    // Format the datetime to determine content width
    QString formatted = m_dateTime.toString(m_displayFormat);
    // Add 2 for left/right border, 2 for padding
    return QSize(formatted.length() + 4, 3);
}

void QTuiDateTimeEdit::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);

    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    Color fg = theme.foreground;
    Color bg = theme.background;

    if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // Clear the entire widget area
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

    // Render top border
    if (geoH >= 1) {
        int py = geoY;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.setChar(border.topLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.topRight);
                else
                    cell.setChar(border.horizontal);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render bottom border
    if (geoH >= 3) {
        int py = geoY + geoH - 1;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.setChar(border.bottomLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.bottomRight);
                else
                    cell.setChar(border.horizontal);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render left and right borders for middle rows
    for (int row = 1; row < geoH - 1; ++row) {
        int py = geoY + row;
        if (py < buffer.height()) {
            int pxLeft = geoX;
            if (pxLeft < buffer.width()) {
                Cell &cell = buffer.backCell(pxLeft, py);
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                Cell &cell = buffer.backCell(pxRight, py);
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render formatted datetime content in the middle row
    if (geoH >= 3 && geoW >= 3) {
        QString formatted = m_dateTime.toString(m_displayFormat);
        int contentRow = geoY + geoH / 2;
        int innerWidth = geoW - 2;
        int textLen = qMin(static_cast<int>(formatted.length()), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center

        // Get section ranges for highlighting
        QList<SectionRange> sectionRanges = parseSectionRanges();
        const auto sections = displaySections();
        SectionRange activeRange = { Year, -1, 0 };
        if (m_currentSection >= 0 && m_currentSection < sections.size()) {
            Section activeSec = sections.at(m_currentSection);
            for (const auto &sr : sectionRanges) {
                if (sr.section == activeSec) {
                    activeRange = sr;
                    break;
                }
            }
        }

        for (int i = 0; i < textLen; ++i) {
            int px = geoX + startCol + i;
            int py = contentRow;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(formatted.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;

                // Highlight the active section with underline
                if (hasFocus() && activeRange.startPos >= 0
                    && i >= activeRange.startPos
                    && i < activeRange.startPos + activeRange.length) {
                    cell.underline = true;
                    cell.bold = true;
                }
            }
        }
    }
}

void QTuiDateTimeEdit::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();

    switch (key) {
    case Qt::Key_Left:
        prevSection();
        break;
    case Qt::Key_Right:
        nextSection();
        break;
    case Qt::Key_Up:
        stepSection(1);
        break;
    case Qt::Key_Down:
        stepSection(-1);
        break;
    default:
        break;
    }
}

// --- QTuiDateEdit ---

QTuiDateEdit::QTuiDateEdit(QTuiWidget *parent)
    : QTuiDateTimeEdit(parent)
{
    setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
}

QTuiDateEdit::QTuiDateEdit(const QDate &date, QTuiWidget *parent)
    : QTuiDateTimeEdit(QDateTime(date, QTime(0, 0, 0)), parent)
{
    setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
}

QList<QTuiDateTimeEdit::Section> QTuiDateEdit::displaySections() const
{
    return { Year, Month, Day };
}

// --- QTuiTimeEdit ---

QTuiTimeEdit::QTuiTimeEdit(QTuiWidget *parent)
    : QTuiDateTimeEdit(parent)
{
    setDisplayFormat(QStringLiteral("HH:mm:ss"));
}

QTuiTimeEdit::QTuiTimeEdit(const QTime &time, QTuiWidget *parent)
    : QTuiDateTimeEdit(QDateTime(QDate::currentDate(), time), parent)
{
    setDisplayFormat(QStringLiteral("HH:mm:ss"));
}

QList<QTuiDateTimeEdit::Section> QTuiTimeEdit::displaySections() const
{
    return { Hour, Minute, Second };
}

} // namespace QtTUI
