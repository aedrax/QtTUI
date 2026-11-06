#include <QtTUI/QTuiDateTimeEdit.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiCalendarWidget.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

// The calendar of the calendarPopup property: a popup child of the edit, so
// it paints above the other widgets and gets the keys while it is open
class QTuiDateTimeEditPopup : public QTuiCalendarWidget {
public:
    explicit QTuiDateTimeEditPopup(QTuiWidget *edit)
        : QTuiCalendarWidget(edit)
    {
        setPopup(true);
        setVisible(false);
    }

protected:
    void keyPressEvent(TuiKeyEvent *event) override
    {
        // Escape closes the popup. While the year list of the calendar is
        // open, Escape closes only that list.
        if (event->key() == Qt::Key_Escape && !acceptsTextInput()) {
            setVisible(false);
            return;
        }
        QTuiCalendarWidget::keyPressEvent(event);
    }
};

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
        m_typed.clear();
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
    if (m_calendarPopup == enable)
        return;
    m_calendarPopup = enable;
    if (!enable && m_popup)
        m_popup->setVisible(false);
    markDirty(); // the "▼" cell shows or hides
}

bool QTuiDateTimeEdit::hasCalendarPopup() const
{
    if (!m_calendarPopup)
        return false;
    const auto ranges = parseSectionRanges();
    for (const auto &range : ranges) {
        if (range.section == Year || range.section == Month || range.section == Day)
            return true;
    }
    return false;
}

void QTuiDateTimeEdit::showCalendarPopup()
{
    if (!m_popup) {
        m_popup = new QTuiDateTimeEditPopup(this);
        connect(m_popup, &QTuiCalendarWidget::activated, this, [this](const QDate &date) {
            setDate(date);
            m_popup->setVisible(false); // the focus goes back to the edit
        });
        // A hidden edit must not leave its calendar on the screen
        connect(this, &QTuiWidget::visibilityChanged, m_popup, [this](bool visible) {
            if (!visible)
                m_popup->setVisible(false);
        });
    }
    m_popup->setMinimumDate(m_minimum.date());
    m_popup->setMaximumDate(m_maximum.date());
    m_popup->setSelectedDate(date());

    // Below the edit, moved as necessary to stay inside the main window
    QRect geo(geometry().bottomLeft() + QPoint(0, 1), m_popup->minimumSize());
    if (auto *app = QTuiApplication::instance(); app && app->mainWindow()) {
        const QRect screen = app->mainWindow()->geometry();
        geo.moveLeft(qBound(screen.left(), geo.left(), screen.right() + 1 - geo.width()));
        geo.moveTop(qBound(screen.top(), geo.top(), screen.bottom() + 1 - geo.height()));
    }
    m_popup->setGeometry(geo);
    m_popup->setVisible(true);
    m_popup->setFocus();
}

void QTuiDateTimeEdit::typeDigit(QChar digit)
{
    const auto ranges = parseSectionRanges();
    if (m_currentSection < 0 || m_currentSection >= ranges.size())
        return;
    const Section sec = ranges.at(m_currentSection).section;
    const int maxDigits = sec == Year ? 4 : 2;

    // Sets the section to value. Returns false if the section cannot have
    // this value.
    auto apply = [this, sec](int value) {
        QDate d = m_dateTime.date();
        QTime t = m_dateTime.time();
        switch (sec) {
        case Year:
            if (value < 1)
                return false;
            d = QDate(value, d.month(), qMin(d.day(), QDate(value, d.month(), 1).daysInMonth()));
            break;
        case Month:
            if (value < 1 || value > 12)
                return false;
            d = QDate(d.year(), value, qMin(d.day(), QDate(d.year(), value, 1).daysInMonth()));
            break;
        case Day:
            if (value < 1 || value > d.daysInMonth())
                return false;
            d = QDate(d.year(), d.month(), value);
            break;
        case Hour:
            if (value > 23)
                return false;
            t = QTime(value, t.minute(), t.second());
            break;
        case Minute:
            if (value > 59)
                return false;
            t = QTime(t.hour(), value, t.second());
            break;
        case Second:
            if (value > 59)
                return false;
            t = QTime(t.hour(), t.minute(), value);
            break;
        }
        if (!d.isValid() || !t.isValid())
            return false;
        setDateTime(QDateTime(d, t));
        return true;
    };

    m_typed += digit;
    const bool full = m_typed.size() >= maxDigits;
    // A year needs all 4 digits. The other sections show each digit at
    // once; a value that is not valid yet, such as "0" for a day, waits for
    // the next digit.
    const bool ok = (sec != Year || full) && apply(m_typed.toInt());
    if (full && ok) {
        m_typed.clear();
        nextSection();
    } else if (full) {
        // For example "45" in the day section: start again with "5"
        m_typed = QString(digit);
        if (sec != Year)
            apply(m_typed.toInt());
    }
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
    m_typed.clear();
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
    m_typed.clear();
    if (m_currentSection < parseSectionRanges().size() - 1) {
        ++m_currentSection;
        markDirty();
    }
}

void QTuiDateTimeEdit::prevSection()
{
    m_typed.clear();
    if (m_currentSection > 0) {
        --m_currentSection;
        markDirty();
    }
}

QSize QTuiDateTimeEdit::minimumSize() const
{
    // Format the datetime to determine content width
    QString formatted = m_dateTime.toString(m_displayFormat);
    // Add 2 for left/right border, 2 for padding, 2 for " ▼"
    return QSize(formatted.length() + 4 + (hasCalendarPopup() ? 2 : 0), 3);
}

QRect QTuiDateTimeEdit::textRect() const
{
    // Centered between the borders, without the 2 columns of " ▼"
    const QRect geo = geometry();
    const int innerWidth = qMax(0, geo.width() - 2 - (hasCalendarPopup() ? 2 : 0));
    const int textLen = qMin(static_cast<int>(m_dateTime.toString(m_displayFormat).length()), innerWidth);
    return QRect(geo.x() + 1 + (innerWidth - textLen) / 2, geo.y() + geo.height() / 2, textLen, 1);
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
        const QRect text = textRect();
        int contentRow = text.y();
        int textLen = text.width();
        int startCol = text.x() - geo.x();

        // The cell that opens the calendar, before the right border
        if (hasCalendarPopup() && geo.width() >= 5)
            buffer.putChar(geo.right() - 1, contentRow, U'\u25BC', cellStyle(theme, fg, bg)); // ▼

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

    // F4 and Alt+Down open the calendar, as in Qt
    if (hasCalendarPopup() && (key == Qt::Key_F4
                               || (key == Qt::Key_Down && (event->modifiers() & Qt::AltModifier)))) {
        showCalendarPopup();
        return;
    }

    const QString text = event->text();
    if (text.size() == 1 && text.at(0) >= QLatin1Char('0') && text.at(0) <= QLatin1Char('9')
        && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
        typeDigit(text.at(0));
        return;
    }

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

void QTuiDateTimeEdit::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press || event->button() != Qt::LeftButton)
        return;

    const QRect geo = geometry();
    const QRect text = textRect();
    if (event->y() != text.y())
        return;

    // "▼": open the calendar, or close it if it is open
    if (hasCalendarPopup() && event->x() == geo.right() - 1) {
        if (m_popup && m_popup->isVisible())
            m_popup->setVisible(false);
        else
            showCalendarPopup();
        return;
    }

    // The section under the click becomes the current section
    const int pos = event->x() - text.x();
    const auto ranges = parseSectionRanges();
    for (int i = 0; i < ranges.size(); ++i) {
        if (pos >= ranges.at(i).startPos && pos < ranges.at(i).startPos + ranges.at(i).length
            && pos < text.width()) {
            if (m_currentSection != i) {
                m_currentSection = i;
                m_typed.clear();
                markDirty();
            }
            return;
        }
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
