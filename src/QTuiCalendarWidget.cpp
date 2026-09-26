#include <QtTUI/QTuiCalendarWidget.h>
#include <QtTUI/RenderUtils.h>
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
    setFocusPolicy(Qt::StrongFocus);
    setGeometry(QRect(0, 0, 22, 9));
    connect(this, &QTuiWidget::focusLost, this, [this]() { finishYearEdit(false); });
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
    markDirty();
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
    if (m_minimumDate == date)
        return;
    m_minimumDate = date;
    if (m_minimumDate.isValid() && m_cursorDate < m_minimumDate)
        m_cursorDate = m_minimumDate;
    if (m_minimumDate.isValid() && m_selectedDate.isValid() && m_selectedDate < m_minimumDate) {
        m_selectedDate = m_minimumDate;
        emit selectionChanged();
    }
    markDirty();
}

void QTuiCalendarWidget::setMaximumDate(const QDate &date)
{
    if (m_maximumDate == date)
        return;
    m_maximumDate = date;
    if (m_maximumDate.isValid() && m_cursorDate > m_maximumDate)
        m_cursorDate = m_maximumDate;
    if (m_maximumDate.isValid() && m_selectedDate.isValid() && m_selectedDate > m_maximumDate) {
        m_selectedDate = m_maximumDate;
        emit selectionChanged();
    }
    markDirty();
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

void QTuiCalendarWidget::moveCursor(int days)
{
    const QDate clamped = clampDate(m_cursorDate.addDays(days));
    if (!clamped.isValid() || clamped == m_cursorDate)
        return;
    m_cursorDate = clamped;
    m_displayMonth = m_cursorDate.month();
    m_displayYear = m_cursorDate.year();
    markDirty();
}

void QTuiCalendarWidget::selectCursorDate()
{
    if (clampDate(m_cursorDate) != m_cursorDate)
        return;
    if (m_cursorDate != m_selectedDate) {
        m_selectedDate = m_cursorDate;
        emit selectionChanged();
        markDirty();
    }
    emit activated(m_cursorDate);
}

QSize QTuiCalendarWidget::minimumSize() const
{
    // 22 cols wide: "Mo Tu We Th Fr Sa Su" = 20 chars + 1 col padding each side
    // 9 rows: 1 header + 1 weekday header + 6 grid rows + 1 bottom padding
    return QSize(22, 9);
}

void QTuiCalendarWidget::showMonth(int year, int month)
{
    m_displayYear = year;
    m_displayMonth = month;
    // Move cursor to same day in new month, or last day if month is shorter
    const int day = qMin(m_cursorDate.day(), QDate(year, month, 1).daysInMonth());
    m_cursorDate = clampDate(QDate(year, month, day));
    markDirty();
}

void QTuiCalendarWidget::startYearEdit()
{
    m_yearEditing = true;
    m_editYear = m_displayYear;
    m_yearInput.clear();
    markDirty();
}

void QTuiCalendarWidget::stepEditYear(int years)
{
    m_editYear = qBound(1, m_editYear + years, 9999);
    m_yearInput.clear();
    markDirty();
}

void QTuiCalendarWidget::finishYearEdit(bool apply)
{
    if (!m_yearEditing)
        return;
    m_yearEditing = false;
    const int year = m_yearInput.isEmpty() ? m_editYear : m_yearInput.toInt();
    m_yearInput.clear();
    if (apply && year >= 1)
        showMonth(year, m_displayMonth);
    else
        markDirty();
}

void QTuiCalendarWidget::showPreviousMonth()
{
    if (m_displayMonth == 1)
        showMonth(m_displayYear - 1, 12);
    else
        showMonth(m_displayYear, m_displayMonth - 1);
}

void QTuiCalendarWidget::showNextMonth()
{
    if (m_displayMonth == 12)
        showMonth(m_displayYear + 1, 1);
    else
        showMonth(m_displayYear, m_displayMonth + 1);
}

QString QTuiCalendarWidget::monthText() const
{
    return QLocale().monthName(m_displayMonth, QLocale::LongFormat);
}

QString QTuiCalendarWidget::yearText() const
{
    if (!m_yearEditing)
        return QString::number(m_displayYear) + QStringLiteral("\u25BE"); // ▾
    // While the user types, show the typed digits in the 4 year columns
    return (m_yearInput.isEmpty() ? QString::number(m_editYear) : m_yearInput)
        .leftJustified(4, QLatin1Char(' ')) + QStringLiteral("\u25B4"); // ▴
}

int QTuiCalendarWidget::gridColumn() const
{
    return qMax(0, (geometry().width() - static_cast<int>(weekdayText().length())) / 2);
}

QRect QTuiCalendarWidget::yearListRect() const
{
    const QRect geo = geometry();
    return QRect(geo.x() + yearColumn() - 1, geo.y() + 1, 6, YearListRows + 2);
}

int QTuiCalendarWidget::yearColumn() const
{
    // The month and the year field ("2026▾") are centered between the two
    // arrows. The field always has 5 columns, so the text does not move
    // while the user types.
    const int monthLen = static_cast<int>(monthText().length());
    const int space = static_cast<int>(weekdayText().length()) - 2;
    return gridColumn() + 1 + qMax(0, (space - monthLen - 6) / 2) + monthLen + 1;
}

QString QTuiCalendarWidget::weekdayText() const
{
    QStringList days;
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day)
        days.append(QLocale().dayName(day, QLocale::ShortFormat).left(2));
    return days.join(QLatin1Char(' '));
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

    auto setCell = [&](int x, int y, char32_t ch, Color cellFg, Color cellBg, bool bold = false, bool underline = false) {
        if (x >= 0 && x < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(x, y);
            cell.setChar(ch);
            cell.foreground = cellFg;
            cell.background = cellBg;
            cell.bold = bold;
            cell.underline = underline;
        }
    };

    auto writeString = [&](int x, int y, const QString &str, Color cellFg, Color cellBg, bool bold = false, bool underline = false) {
        const int len = static_cast<int>(str.length());
        for (int i = 0; i < len; ++i) {
            setCell(x + i, y, str.at(i).unicode(), cellFg, cellBg, bold, underline);
        }
    };

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();

    // Clear the widget area
    for (int row = 0; row < geo.height(); ++row) {
        for (int col = 0; col < w; ++col) {
            setCell(x0 + col, y0 + row, U' ', fg, bg);
        }
    }

    // Row 0: "<" above Monday, ">" above Sunday, the month and the year
    // centered between them. These columns do not depend on the month.
    const int gridX = x0 + gridColumn();
    const int gridW = static_cast<int>(weekdayText().length());
    writeString(gridX, y0, QStringLiteral("<"), fg, bg, true);
    writeString(gridX + gridW - 1, y0, QStringLiteral(">"), fg, bg, true);
    const QString month = monthText();
    const int yearX = x0 + yearColumn();
    writeString(yearX - static_cast<int>(month.length()) - 1, y0, month, fg, bg, true);
    // The year is a combo box: it shows in the focus colors while its list
    // is open
    writeString(yearX, y0, yearText(), m_yearEditing ? theme.focusForeground : fg,
                m_yearEditing ? theme.focusBackground : bg, true, m_yearEditing);

    // Row 1: Weekday header, 2 characters per day from the locale
    // ("Mo Tu We Th Fr Sa Su" in English), like the month name above
    writeString(gridX, y0 + 1, weekdayText(), fg, bg, false, true);

    // Rows 2-7: Day grid (6 rows of 7 columns, each cell 3 chars wide)
    const QDate firstOfMonth(m_displayYear, m_displayMonth, 1);
    // dayOfWeek(): Monday=1 ... Sunday=7
    const int startDow = firstOfMonth.dayOfWeek(); // 1-based, Monday=1
    const int daysInMonth = firstOfMonth.daysInMonth();

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

    // The open year list, over the day grid: a box with the years around
    // the current one. Earlier years are at the top.
    if (m_yearEditing) {
        const QRect box = yearListRect();
        const Cell boxStyle = cellStyle(theme, fg, bg);
        fillRect(buffer, box, boxStyle);
        drawBorder(buffer, box, theme.borderStyle, boxStyle);
        for (int i = 0; i < YearListRows; ++i) {
            const int year = m_editYear - YearListRows / 2 + i;
            if (year < 1 || year > 9999)
                continue;
            const bool current = year == m_editYear;
            writeString(box.x() + 1, box.y() + 1 + i, QString::number(year).rightJustified(4),
                        current ? theme.focusForeground : fg, current ? theme.focusBackground : bg,
                        current);
        }
    }
}

void QTuiCalendarWidget::keyPressEvent(TuiKeyEvent *event)
{
    // F4 and Alt+Down open the year list, as in a combo box
    if (!m_yearEditing && (event->key() == Qt::Key_F4
                           || (event->key() == Qt::Key_Down && (event->modifiers() & Qt::AltModifier)))) {
        startYearEdit();
        return;
    }

    // While the year list is open: Up and Down move in the list, PageUp and
    // PageDown move 10 years, digits type a year, Enter applies the year and
    // Escape closes the list. Other keys close the list first.
    if (m_yearEditing) {
        const QString text = event->text();
        switch (event->key()) {
        case Qt::Key_Up:
            stepEditYear(-1);
            return;
        case Qt::Key_Down:
            stepEditYear(1);
            return;
        case Qt::Key_PageUp:
            stepEditYear(-10);
            return;
        case Qt::Key_PageDown:
            stepEditYear(10);
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            finishYearEdit(true);
            return;
        case Qt::Key_Escape:
            finishYearEdit(false);
            return;
        case Qt::Key_Backspace:
            m_yearInput.chop(1);
            markDirty();
            return;
        default:
            if (text.size() == 1 && text.at(0).isDigit()) {
                if (m_yearInput.size() < 4)
                    m_yearInput += text;
                // A full year also moves the list to it
                if (m_yearInput.size() == 4 && m_yearInput.toInt() >= 1)
                    m_editYear = m_yearInput.toInt();
                markDirty();
                return;
            }
            finishYearEdit(false);
        }
    }

    switch (event->key()) {
    case Qt::Key_Left:
        moveCursor(-1);
        break;
    case Qt::Key_Right:
        moveCursor(1);
        break;
    case Qt::Key_Up:
        moveCursor(-7);
        break;
    case Qt::Key_Down:
        moveCursor(7);
        break;
    case Qt::Key_PageUp:
        showPreviousMonth();
        break;
    case Qt::Key_PageDown:
        showNextMonth();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        selectCursorDate();
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        return;
    }
}

void QTuiCalendarWidget::mousePressEvent(TuiMouseEvent *event)
{
    // The wheel scrolls the open year list
    if (event->mouseType() == TuiMouseEvent::Wheel && m_yearEditing) {
        stepEditYear(event->wheelDelta() > 0 ? -1 : 1);
        return;
    }
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    // Use the same positions as paintEvent()
    const QRect geo = geometry();
    const int col = event->x() - geo.x();
    const int row = event->y() - geo.y();
    const int gridCol = col - gridColumn();
    const int yearCol = yearColumn() + geo.x();
    const bool onYear = row == 0 && event->x() >= yearCol && event->x() < yearCol + 5;

    // While the year list is open, a click on a year applies it. A click on
    // the field or outside the list closes the list.
    if (m_yearEditing) {
        const QRect box = yearListRect();
        const int i = event->y() - box.y() - 1;
        if (!onYear && box.contains(event->x(), event->y()) && i >= 0 && i < YearListRows) {
            const int year = m_editYear - YearListRows / 2 + i;
            if (year >= 1 && year <= 9999) {
                m_editYear = year;
                m_yearInput.clear();
                finishYearEdit(true);
            }
            return;
        }
        finishYearEdit(false);
        return;
    }

    // Row 0: "<" shows the previous month, ">" shows the next month, and a
    // click on the year opens the year list
    if (row == 0) {
        if (onYear)
            startYearEdit();
        else if (gridCol == 0)
            showPreviousMonth();
        else if (gridCol == static_cast<int>(weekdayText().length()) - 1)
            showNextMonth();
        return;
    }

    // Rows 2-7: each day has 2 columns and a 1-column gap
    const int gridRow = row - 2;
    if (gridCol < 0 || gridCol % 3 == 2 || gridCol / 3 > 6 || gridRow < 0 || gridRow > 5)
        return;
    const QDate firstOfMonth(m_displayYear, m_displayMonth, 1);
    const int dayNum = gridRow * 7 + gridCol / 3 - firstOfMonth.dayOfWeek() + 2;
    if (dayNum < 1 || dayNum > firstOfMonth.daysInMonth())
        return;
    const QDate date(m_displayYear, m_displayMonth, dayNum);
    if (clampDate(date) != date)
        return;
    m_cursorDate = date;
    markDirty();
    selectCursorDate();
}

} // namespace QtTUI
