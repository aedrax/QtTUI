#include <QtTUI/QTuiCalendarWidget.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/ScreenBuffer.h>
#include <QLocale>

namespace QtTUI {

QTuiCalendarWidget::QTuiCalendarWidget(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_selectedDate(QDate::currentDate())
    , m_cursorDate(QDate::currentDate())
    , m_displayYear(QDate::currentDate().year())
    , m_displayMonth(QDate::currentDate().month())
{
    setGeometry(QRect(0, 0, 22, 9));
}

QDate QTuiCalendarWidget::selectedDate() const
{
    return m_selectedDate;
}

void QTuiCalendarWidget::setSelectedDate(const QDate &date)
{
    const QDate clamped = clampDate(date);
    if (!clamped.isValid() || clamped == m_selectedDate)
        return;

    m_selectedDate = clamped;
    m_cursorDate = clamped;
    m_displayYear = clamped.year();
    m_displayMonth = clamped.month();
    emit selectionChanged();
}

int QTuiCalendarWidget::yearShown() const
{
    return m_displayYear;
}

int QTuiCalendarWidget::monthShown() const
{
    return m_displayMonth;
}

void QTuiCalendarWidget::setMinimumDate(const QDate &date)
{
    m_minimumDate = date;
    if (m_minimumDate.isValid() && m_cursorDate < m_minimumDate)
        m_cursorDate = m_minimumDate;
    if (m_minimumDate.isValid() && m_selectedDate.isValid() && m_selectedDate < m_minimumDate) {
        m_selectedDate = m_minimumDate;
        emit selectionChanged();
    }
}

void QTuiCalendarWidget::setMaximumDate(const QDate &date)
{
    m_maximumDate = date;
    if (m_maximumDate.isValid() && m_cursorDate > m_maximumDate)
        m_cursorDate = m_maximumDate;
    if (m_maximumDate.isValid() && m_selectedDate.isValid() && m_selectedDate > m_maximumDate) {
        m_selectedDate = m_maximumDate;
        emit selectionChanged();
    }
}

QDate QTuiCalendarWidget::minimumDate() const
{
    return m_minimumDate;
}

QDate QTuiCalendarWidget::maximumDate() const
{
    return m_maximumDate;
}

QDate QTuiCalendarWidget::clampDate(const QDate &date) const
{
    if (!date.isValid())
        return date;
    QDate result = date;
    if (m_minimumDate.isValid() && result < m_minimumDate)
        result = m_minimumDate;
    if (m_maximumDate.isValid() && result > m_maximumDate)
        result = m_maximumDate;
    return result;
}

QSize QTuiCalendarWidget::minimumSize() const
{
    // 22 cols wide: "Mo Tu We Th Fr Sa Su" = 20 chars + 1 col padding each side
    // 9 rows: 1 header + 1 weekday header + 6 grid rows + 1 bottom padding
    return QSize(22, 9);
}

void QTuiCalendarWidget::showPreviousMonth()
{
    m_displayMonth--;
    if (m_displayMonth < 1) {
        m_displayMonth = 12;
        m_displayYear--;
    }
    // Move cursor to same day in new month, or last day if month is shorter
    int day = qMin(m_cursorDate.day(), QDate(m_displayYear, m_displayMonth, 1).daysInMonth());
    m_cursorDate = clampDate(QDate(m_displayYear, m_displayMonth, day));
}

void QTuiCalendarWidget::showNextMonth()
{
    m_displayMonth++;
    if (m_displayMonth > 12) {
        m_displayMonth = 1;
        m_displayYear++;
    }
    int day = qMin(m_cursorDate.day(), QDate(m_displayYear, m_displayMonth, 1).daysInMonth());
    m_cursorDate = clampDate(QDate(m_displayYear, m_displayMonth, day));
}

void QTuiCalendarWidget::paintEvent(ScreenBuffer &buffer)
{
    if (!isVisible())
        return;

    const QRect geo = geometry();
    if (geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.foreground;
    const Color bg = theme.background;
    const QDate today = QDate::currentDate();

    auto setCell = [&](int x, int y, QChar ch, Color cellFg, Color cellBg, bool bold = false, bool underline = false) {
        if (x >= 0 && x < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(x, y);
            cell.character = ch;
            cell.foreground = cellFg;
            cell.background = cellBg;
            cell.bold = bold;
            cell.underline = underline;
        }
    };

    auto writeString = [&](int x, int y, const QString &str, Color cellFg, Color cellBg, bool bold = false, bool underline = false) {
        const int len = static_cast<int>(str.length());
        for (int i = 0; i < len; ++i) {
            setCell(x + i, y, str.at(i), cellFg, cellBg, bold, underline);
        }
    };

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();

    // Clear the widget area
    for (int row = 0; row < geo.height(); ++row) {
        for (int col = 0; col < w; ++col) {
            setCell(x0 + col, y0 + row, QLatin1Char(' '), fg, bg);
        }
    }

    // Row 0: Month/Year header centered, e.g. "< January 2025  >"
    const QString monthName = QLocale().monthName(m_displayMonth, QLocale::LongFormat);
    const QString header = QStringLiteral("< %1 %2 >").arg(monthName).arg(m_displayYear);
    const int headerLen = static_cast<int>(header.length());
    const int headerX = x0 + qMax(0, (w - headerLen) / 2);
    writeString(headerX, y0, header, fg, bg, true);

    // Row 1: Weekday header "Mo Tu We Th Fr Sa Su"
    const QString weekdays = QStringLiteral("Mo Tu We Th Fr Sa Su");
    const int weekdaysLen = static_cast<int>(weekdays.length());
    const int weekdayX = x0 + qMax(0, (w - weekdaysLen) / 2);
    writeString(weekdayX, y0 + 1, weekdays, fg, bg, false, true);

    // Rows 2-7: Day grid (6 rows of 7 columns, each cell 3 chars wide)
    const QDate firstOfMonth(m_displayYear, m_displayMonth, 1);
    // dayOfWeek(): Monday=1 ... Sunday=7
    const int startDow = firstOfMonth.dayOfWeek(); // 1-based, Monday=1
    const int daysInMonth = firstOfMonth.daysInMonth();

    const int gridX = weekdayX; // Align grid with weekday header

    for (int gridRow = 0; gridRow < 6; ++gridRow) {
        for (int gridCol = 0; gridCol < 7; ++gridCol) {
            const int cellIndex = gridRow * 7 + gridCol; // 0-based
            const int dayNum = cellIndex - (startDow - 1) + 1;

            if (dayNum < 1 || dayNum > daysInMonth)
                continue; // Leave blank for days outside this month

            const QDate cellDate(m_displayYear, m_displayMonth, dayNum);
            const QString dayStr = QString::number(dayNum).rightJustified(2, QLatin1Char(' '));

            Color dayFg = fg;
            Color dayBg = bg;
            bool dayBold = false;
            bool dayUnderline = false;

            // Check if this date is outside min/max range
            bool outOfRange = false;
            if (m_minimumDate.isValid() && cellDate < m_minimumDate)
                outOfRange = true;
            if (m_maximumDate.isValid() && cellDate > m_maximumDate)
                outOfRange = true;

            if (outOfRange) {
                dayFg = theme.disabledForeground;
                dayBg = theme.disabledBackground;
            } else if (cellDate == m_selectedDate) {
                // Selected date: focus colors
                dayFg = theme.focusForeground;
                dayBg = theme.focusBackground;
                dayBold = true;
            } else if (cellDate == today) {
                // Today: bold + underline to distinguish from selected
                dayBold = true;
                dayUnderline = true;
            }

            // If cursor is on this date and it differs from selected, show it
            if (cellDate == m_cursorDate && cellDate != m_selectedDate && !outOfRange) {
                dayFg = theme.focusForeground;
                dayBg = theme.focusBackground;
            }

            const int cellX = gridX + gridCol * 3;
            const int cellY = y0 + 2 + gridRow;
            writeString(cellX, cellY, dayStr, dayFg, dayBg, dayBold, dayUnderline);
        }
    }
}

void QTuiCalendarWidget::keyPressEvent(TuiKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left: {
        const QDate newDate = m_cursorDate.addDays(-1);
        const QDate clamped = clampDate(newDate);
        if (clamped.isValid() && clamped != m_cursorDate) {
            m_cursorDate = clamped;
            if (m_cursorDate.month() != m_displayMonth || m_cursorDate.year() != m_displayYear) {
                m_displayMonth = m_cursorDate.month();
                m_displayYear = m_cursorDate.year();
            }
        }
        break;
    }
    case Qt::Key_Right: {
        const QDate newDate = m_cursorDate.addDays(1);
        const QDate clamped = clampDate(newDate);
        if (clamped.isValid() && clamped != m_cursorDate) {
            m_cursorDate = clamped;
            if (m_cursorDate.month() != m_displayMonth || m_cursorDate.year() != m_displayYear) {
                m_displayMonth = m_cursorDate.month();
                m_displayYear = m_cursorDate.year();
            }
        }
        break;
    }
    case Qt::Key_Up: {
        const QDate newDate = m_cursorDate.addDays(-7);
        const QDate clamped = clampDate(newDate);
        if (clamped.isValid() && clamped != m_cursorDate) {
            m_cursorDate = clamped;
            if (m_cursorDate.month() != m_displayMonth || m_cursorDate.year() != m_displayYear) {
                m_displayMonth = m_cursorDate.month();
                m_displayYear = m_cursorDate.year();
            }
        }
        break;
    }
    case Qt::Key_Down: {
        const QDate newDate = m_cursorDate.addDays(7);
        const QDate clamped = clampDate(newDate);
        if (clamped.isValid() && clamped != m_cursorDate) {
            m_cursorDate = clamped;
            if (m_cursorDate.month() != m_displayMonth || m_cursorDate.year() != m_displayYear) {
                m_displayMonth = m_cursorDate.month();
                m_displayYear = m_cursorDate.year();
            }
        }
        break;
    }
    case Qt::Key_PageUp:
        showPreviousMonth();
        break;
    case Qt::Key_PageDown:
        showNextMonth();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        // Select the cursor date if it's within range
        bool inRange = true;
        if (m_minimumDate.isValid() && m_cursorDate < m_minimumDate)
            inRange = false;
        if (m_maximumDate.isValid() && m_cursorDate > m_maximumDate)
            inRange = false;

        if (inRange && m_cursorDate != m_selectedDate) {
            m_selectedDate = m_cursorDate;
            emit selectionChanged();
        }
        if (inRange)
            emit activated(m_cursorDate);
        break;
    }
    default:
        QTuiWidget::keyPressEvent(event);
        return;
    }
}

} // namespace QtTUI
