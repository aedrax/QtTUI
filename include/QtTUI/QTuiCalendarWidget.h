#ifndef QTTUI_QTUI_CALENDARWIDGET_H
#define QTTUI_QTUI_CALENDARWIDGET_H

#include <QtTUI/QTuiWidget.h>
#include <QDate>

namespace QtTUI {

class QTuiCalendarWidget : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY selectionChanged)
public:
    explicit QTuiCalendarWidget(QTuiWidget *parent = nullptr);

    QDate selectedDate() const;
    void setSelectedDate(const QDate &date);

    int yearShown() const;
    int monthShown() const;

    void setMinimumDate(const QDate &date);
    void setMaximumDate(const QDate &date);
    QDate minimumDate() const;
    QDate maximumDate() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void activated(const QDate &date);
    void selectionChanged();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    void showPreviousMonth();
    void showNextMonth();
    QDate clampDate(const QDate &date) const;
    // Moves the cursor by a number of days and shows its month
    void moveCursor(int days);
    // Selects the cursor date if it is in the range, and emits activated()
    void selectCursorDate();
    // "< <month> <year> >" on row 0
    QString headerText() const;
    // The weekday names on row 1, for example "Mo Tu We Th Fr Sa Su"
    QString weekdayText() const;

    QDate m_selectedDate;
    QDate m_cursorDate;
    QDate m_minimumDate;
    QDate m_maximumDate;
    int m_displayYear;
    int m_displayMonth;
};

} // namespace QtTUI

#endif // QTTUI_QTUI_CALENDARWIDGET_H
