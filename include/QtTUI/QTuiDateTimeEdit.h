#ifndef QTTUI_QTUIDATETIMEEDIT_H
#define QTTUI_QTUIDATETIMEEDIT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QDateTime>
#include <QString>
#include <QList>

namespace QtTUI {

class QTuiCalendarWidget;

class QTuiDateTimeEdit : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(QString displayFormat READ displayFormat WRITE setDisplayFormat)
    Q_PROPERTY(bool calendarPopup READ calendarPopup WRITE setCalendarPopup)
public:
    explicit QTuiDateTimeEdit(QTuiWidget *parent = nullptr);
    explicit QTuiDateTimeEdit(const QDateTime &dateTime, QTuiWidget *parent = nullptr);

    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dateTime);

    QDate date() const;
    void setDate(const QDate &date);

    QTime time() const;
    void setTime(const QTime &time);

    QString displayFormat() const;
    void setDisplayFormat(const QString &format);

    void setMinimumDateTime(const QDateTime &min);
    void setMaximumDateTime(const QDateTime &max);
    QDateTime minimumDateTime() const;
    QDateTime maximumDateTime() const;

    // If true and the format has a date section, the edit shows a "▼"
    // cell. F4, Alt+Down or a click on the cell opens a calendar below the
    // edit. A date that the user chooses there (Enter or a click) sets the
    // date and closes the calendar; Escape closes it without a change.
    bool calendarPopup() const;
    void setCalendarPopup(bool enable);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;
    // Digit keys go into the current section, not to shortcuts
    bool acceptsTextInput() const override { return true; }

signals:
    void dateTimeChanged(const QDateTime &dateTime);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    // A click selects the section under it; a click on "▼" opens or closes
    // the calendar
    void mousePressEvent(TuiMouseEvent *event) override;

    enum Section { Year, Month, Day, Hour, Minute, Second };
    virtual QList<Section> displaySections() const;

private:
    void stepSection(int delta);
    void nextSection();
    void prevSection();
    // Puts a typed digit into the current section, as in Qt: "2" then "5"
    // in the day section gives day 25. A full section (4 digits for the
    // year, else 2) moves to the next section.
    void typeDigit(QChar digit);
    // True if calendarPopup is set and the format has a date section
    bool hasCalendarPopup() const;
    void showCalendarPopup();
    // The cells of the text, in screen coordinates
    QRect textRect() const;

    // Helpers for format parsing and section positioning
    struct SectionRange {
        Section section;
        int startPos;
        int length;
    };
    QList<SectionRange> parseSectionRanges() const;

    QDateTime m_dateTime;
    QDateTime m_minimum;
    QDateTime m_maximum;
    QString m_displayFormat = QStringLiteral("yyyy-MM-dd HH:mm:ss");
    int m_currentSection = 0;
    bool m_calendarPopup = false;
    // The digits typed into the current section
    QString m_typed;
    QTuiCalendarWidget *m_popup = nullptr;
};

class QTuiDateEdit : public QTuiDateTimeEdit {
    Q_OBJECT
public:
    explicit QTuiDateEdit(QTuiWidget *parent = nullptr);
    explicit QTuiDateEdit(const QDate &date, QTuiWidget *parent = nullptr);

protected:
    QList<Section> displaySections() const override;
};

class QTuiTimeEdit : public QTuiDateTimeEdit {
    Q_OBJECT
public:
    explicit QTuiTimeEdit(QTuiWidget *parent = nullptr);
    explicit QTuiTimeEdit(const QTime &time, QTuiWidget *parent = nullptr);

protected:
    QList<Section> displaySections() const override;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDATETIMEEDIT_H
