#ifndef QTTUI_QTUIDATETIMEEDIT_H
#define QTTUI_QTUIDATETIMEEDIT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QDateTime>
#include <QString>
#include <QList>

namespace QtTUI {

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

    bool calendarPopup() const;
    void setCalendarPopup(bool enable);

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void dateTimeChanged(const QDateTime &dateTime);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

    enum Section { Year, Month, Day, Hour, Minute, Second };
    virtual QList<Section> displaySections() const;

private:
    void stepSection(int delta);
    void nextSection();
    void prevSection();

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
