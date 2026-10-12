#include <QtTUI/QTuiDateTimeEdit.h>
#include <QtTUI/RenderUtils.h>
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
        m_currentSection = 0;
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
    // The sections in the order of the format, at their positions in the text
    struct TokenInfo {
        QString token;
        Section section;
    };
    // Longer tokens first
    static const TokenInfo tokens[] = {
        { QStringLiteral("yyyy"), Year },   { QStringLiteral("yy"), Year },
        { QStringLiteral("MMMM"), Month },  { QStringLiteral("MMM"), Month },
        { QStringLiteral("MM"), Month },    { QStringLiteral("M"), Month },
        { QStringLiteral("dddd"), Day },    { QStringLiteral("ddd"), Day },
        { QStringLiteral("dd"), Day },      { QStringLiteral("d"), Day },
        { QStringLiteral("HH"), Hour },     { QStringLiteral("H"), Hour },
        { QStringLiteral("hh"), Hour },     { QStringLiteral("h"), Hour },
        { QStringLiteral("mm"), Minute },   { QStringLiteral("m"), Minute },
        { QStringLiteral("ss"), Second },   { QStringLiteral("s"), Second },
    };

    QList<SectionRange> ranges;
    const auto sections = displaySections();
    int outPos = 0;
    int i = 0;
    while (i < m_displayFormat.length()) {
        const TokenInfo *match = nullptr;
        for (const auto &tok : tokens) {
            if (m_displayFormat.mid(i, tok.token.length()) == tok.token) {
                match = &tok;
                break;
            }
        }
        if (!match) {
            // A literal character
            ++outPos;
            ++i;
            continue;
        }
        // The text length of a token, for example "MMM" gives "Oct"
        const int outputLen = m_dateTime.toString(match->token).length();
        if (sections.contains(match->section))
            ranges.append({ match->section, outPos, outputLen });
        outPos += outputLen;
        i += match->token.length();
    }
    return ranges;
}

void QTuiDateTimeEdit::stepSection(int delta)
{
    const auto ranges = parseSectionRanges();
    if (m_currentSection < 0 || m_currentSection >= ranges.size())
        return;

    Section sec = ranges.at(m_currentSection).section;
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
    if (m_currentSection < parseSectionRanges().size() - 1) {
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

    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    fillRect(buffer, geo, cellStyle(theme, fg, bg));
    drawBorder(buffer, geo, theme.borderStyle, cellStyle(theme, fg, bg));

    // Render formatted datetime content in the middle row
    if (geo.height() >= 3 && geo.width() >= 3) {
        QString formatted = m_dateTime.toString(m_displayFormat);
        int contentRow = geo.y() + geo.height() / 2;
        int innerWidth = geo.width() - 2;
        int textLen = qMin(static_cast<int>(formatted.length()), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center

        // Get section ranges for highlighting
        const QList<SectionRange> sectionRanges = parseSectionRanges();
        SectionRange activeRange = { Year, -1, 0 };
        if (m_currentSection >= 0 && m_currentSection < sectionRanges.size())
            activeRange = sectionRanges.at(m_currentSection);

        const Cell textStyle = cellStyle(theme, fg, bg);
        renderText(buffer, geo.x() + startCol, contentRow, formatted, textLen, textStyle);

        // Highlight the active section with underline
        if (hasFocus() && activeRange.startPos >= 0 && activeRange.startPos < textLen) {
            Cell sectionStyle = textStyle;
            sectionStyle.underline = true;
            sectionStyle.bold = true;
            renderText(buffer, geo.x() + startCol + activeRange.startPos, contentRow,
                       formatted.mid(activeRange.startPos, activeRange.length),
                       textLen - activeRange.startPos, sectionStyle);
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
