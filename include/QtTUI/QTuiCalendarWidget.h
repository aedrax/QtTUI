#ifndef QTTUI_QTUI_CALENDARWIDGET_H
#define QTTUI_QTUI_CALENDARWIDGET_H

#include <QtTUI/QTuiWidget.h>
#include <QDate>
#include <QString>

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
    // Digit keys must not fire shortcuts while the year list is open
    bool acceptsTextInput() const override { return m_yearEditing; }

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
    // Shows a month and moves the cursor to the same day in it
    void showMonth(int year, int month);
    // The year combo box: a click on the year, F4 or Alt+Down opens its
    // list
    void startYearEdit();
    void stepEditYear(int years);
    void finishYearEdit(bool apply);
    // The parts of row 0 and their columns, relative to the widget
    QString monthText() const;
    QString yearText() const;
    int gridColumn() const;
    int yearColumn() const;
    // The box of the open year list, in screen coordinates
    QRect yearListRect() const;
    static constexpr int YearListRows = 6;
    // The weekday names on row 1, for example "Mo Tu We Th Fr Sa Su"
    QString weekdayText() const;

    QDate m_selectedDate;
    QDate m_cursorDate;
    QDate m_minimumDate;
    QDate m_maximumDate;
    int m_displayYear;
    int m_displayMonth;
    bool m_yearEditing = false;
    int m_editYear = 0;
    // The digits the user typed in the year edit
    QString m_yearInput;
};

} // namespace QtTUI

#endif // QTTUI_QTUI_CALENDARWIDGET_H
