#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiDateTimeEdit.h>
#include <QtTUI/QTuiCalendarWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/QTuiLCDNumber.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

// Test helper to access protected displaySections() and Section enum
class TestableDateTimeEdit : public QTuiDateTimeEdit {
public:
    using QTuiDateTimeEdit::QTuiDateTimeEdit;
    using QTuiDateTimeEdit::Section;
    using QTuiDateTimeEdit::displaySections;
    // Re-export enum values as public static constexpr
    static constexpr Section SYear   = Year;
    static constexpr Section SMonth  = Month;
    static constexpr Section SDay    = Day;
    static constexpr Section SHour   = Hour;
    static constexpr Section SMinute = Minute;
    static constexpr Section SSecond = Second;
};

class TestableDateEdit : public QTuiDateEdit {
public:
    using QTuiDateEdit::QTuiDateEdit;
    using QTuiDateTimeEdit::Section;
    using QTuiDateEdit::displaySections;
};

class TestableTimeEdit : public QTuiTimeEdit {
public:
    using QTuiTimeEdit::QTuiTimeEdit;
    using QTuiDateTimeEdit::Section;
    using QTuiTimeEdit::displaySections;
};

class TestQTuiDateTimeLCD : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiDateTimeEdit tests
    // ---------------------------------------------------------------

    void dateTimeEdit_defaultConstruction()
    {
        QTuiDateTimeEdit dte;
        QVERIFY(dte.dateTime().isValid());
        QCOMPARE(dte.displayFormat(), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    void dateTimeEdit_constructWithDateTime()
    {
        QDateTime dt(QDate(2025, 3, 15), QTime(14, 30, 0));
        QTuiDateTimeEdit dte(dt);
        QCOMPARE(dte.dateTime(), dt);
    }

    void dateTimeEdit_setDateTime()
    {
        QTuiDateTimeEdit dte;
        QSignalSpy spy(&dte, &QTuiDateTimeEdit::dateTimeChanged);

        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        dte.setDateTime(dt);
        QCOMPARE(dte.dateTime(), dt);
        QCOMPARE(spy.count(), 1);
    }

    void dateTimeEdit_setDateTimeClamps()
    {
        QTuiDateTimeEdit dte;
        QDateTime minDt(QDate(2025, 1, 1), QTime(0, 0, 0));
        QDateTime maxDt(QDate(2025, 12, 31), QTime(23, 59, 59));
        dte.setMinimumDateTime(minDt);
        dte.setMaximumDateTime(maxDt);

        // Set below min -> clamped to min
        QDateTime belowMin(QDate(2024, 6, 1), QTime(0, 0, 0));
        dte.setDateTime(belowMin);
        QCOMPARE(dte.dateTime(), minDt);

        // Set above max -> clamped to max
        QDateTime aboveMax(QDate(2026, 6, 1), QTime(0, 0, 0));
        dte.setDateTime(aboveMax);
        QCOMPARE(dte.dateTime(), maxDt);
    }

    void dateTimeEdit_setDate()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        dte.setDate(QDate(2026, 1, 1));
        QCOMPARE(dte.date(), QDate(2026, 1, 1));
        // Time should be preserved
        QCOMPARE(dte.time(), QTime(10, 30, 45));
    }

    void dateTimeEdit_setTime()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        dte.setTime(QTime(23, 59, 59));
        QCOMPARE(dte.time(), QTime(23, 59, 59));
        // Date should be preserved
        QCOMPARE(dte.date(), QDate(2025, 6, 15));
    }

    void dateTimeEdit_displayFormat()
    {
        QTuiDateTimeEdit dte;
        dte.setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
        QCOMPARE(dte.displayFormat(), QStringLiteral("dd/MM/yyyy HH:mm"));
    }

    void dateTimeEdit_minMaxConstraints()
    {
        QTuiDateTimeEdit dte;
        QDateTime minDt(QDate(2025, 1, 1), QTime(0, 0, 0));
        QDateTime maxDt(QDate(2025, 12, 31), QTime(23, 59, 59));

        dte.setMinimumDateTime(minDt);
        dte.setMaximumDateTime(maxDt);
        QCOMPARE(dte.minimumDateTime(), minDt);
        QCOMPARE(dte.maximumDateTime(), maxDt);

        // Setting a value within range should work
        QDateTime inRange(QDate(2025, 6, 15), QTime(12, 0, 0));
        dte.setDateTime(inRange);
        QCOMPARE(dte.dateTime(), inRange);
    }

    void dateTimeEdit_sectionNavigationRight()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Default section is 0 (Year). Move right to Month.
        sendKey(dte, Qt::Key_Right);
        // Now step up to verify we're on Month section
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().date().month(), 7);
    }

    void dateTimeEdit_sectionNavigationLeft()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Move right to Month, then left back to Year
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Left);
        // Now step up to verify we're on Year section
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().date().year(), 2026);
    }

    void dateTimeEdit_sectionsFollowTheFormatOrder()
    {
        QTuiDateTimeEdit dte(QDateTime(QDate(2025, 6, 15), QTime(10, 30, 45)));
        dte.setDisplayFormat(QStringLiteral("dd.MMM.yy"));

        // The first section is the day, then the month
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.date(), QDate(2025, 6, 16));
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.date(), QDate(2025, 7, 16));
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.date(), QDate(2026, 7, 16));
    }

    void dateTimeEdit_sectionStepUp()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Default section is Year
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().date().year(), 2026);
    }

    void dateTimeEdit_sectionStepDown()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Default section is Year
        sendKey(dte, Qt::Key_Down);
        QCOMPARE(dte.dateTime().date().year(), 2024);
    }

    void dateTimeEdit_monthStepping()
    {
        QDateTime dt(QDate(2025, 12, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Navigate to Month section
        sendKey(dte, Qt::Key_Right);
        // Step up from December -> wraps to January (next year)
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().date().month(), 1);

        // Step down from January -> wraps to December (prev year)
        sendKey(dte, Qt::Key_Down);
        QCOMPARE(dte.dateTime().date().month(), 12);
    }

    void dateTimeEdit_dayStepping()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Navigate to Day section (Right, Right)
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().date().day(), 16);
    }

    void dateTimeEdit_hourStepping()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(23, 30, 45));
        QTuiDateTimeEdit dte(dt);

        // Navigate to Hour section (Right x3)
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        // Step up from 23 -> wraps to 0
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().time().hour(), 0);

        // Step down from 0 -> wraps to 23
        sendKey(dte, Qt::Key_Down);
        QCOMPARE(dte.dateTime().time().hour(), 23);
    }

    void dateTimeEdit_minuteStepping()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 59, 45));
        QTuiDateTimeEdit dte(dt);

        // Navigate to Minute section (Right x4)
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        // Step up from 59 -> wraps to 0
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().time().minute(), 0);

        // Step down from 0 -> wraps to 59
        sendKey(dte, Qt::Key_Down);
        QCOMPARE(dte.dateTime().time().minute(), 59);
    }

    void dateTimeEdit_secondStepping()
    {
        QDateTime dt(QDate(2025, 6, 15), QTime(10, 30, 59));
        QTuiDateTimeEdit dte(dt);

        // Navigate to Second section (Right x5)
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        sendKey(dte, Qt::Key_Right);
        // Step up from 59 -> wraps to 0
        sendKey(dte, Qt::Key_Up);
        QCOMPARE(dte.dateTime().time().second(), 0);

        // Step down from 0 -> wraps to 59
        sendKey(dte, Qt::Key_Down);
        QCOMPARE(dte.dateTime().time().second(), 59);
    }

    void dateTimeEdit_calendarPopupFlag()
    {
        QTuiDateTimeEdit dte;
        QCOMPARE(dte.calendarPopup(), false);
        dte.setCalendarPopup(true);
        QCOMPARE(dte.calendarPopup(), true);
    }

    void dateTimeEdit_displaySections()
    {
        TestableDateTimeEdit dte;
        auto sections = dte.displaySections();
        QCOMPARE(sections.size(), 6);
        QCOMPARE(sections.at(0), TestableDateTimeEdit::SYear);
        QCOMPARE(sections.at(1), TestableDateTimeEdit::SMonth);
        QCOMPARE(sections.at(2), TestableDateTimeEdit::SDay);
        QCOMPARE(sections.at(3), TestableDateTimeEdit::SHour);
        QCOMPARE(sections.at(4), TestableDateTimeEdit::SMinute);
        QCOMPARE(sections.at(5), TestableDateTimeEdit::SSecond);
    }

    // ---------------------------------------------------------------
    // QTuiDateEdit tests
    // ---------------------------------------------------------------

    void dateEdit_displaySectionsDateOnly()
    {
        TestableDateEdit de;
        auto sections = de.displaySections();
        QCOMPARE(sections.size(), 3);
        QCOMPARE(sections.at(0), TestableDateTimeEdit::SYear);
        QCOMPARE(sections.at(1), TestableDateTimeEdit::SMonth);
        QCOMPARE(sections.at(2), TestableDateTimeEdit::SDay);
    }

    void dateEdit_defaultFormat()
    {
        QTuiDateEdit de;
        QCOMPARE(de.displayFormat(), QStringLiteral("yyyy-MM-dd"));
    }

    void dateEdit_constructWithDate()
    {
        QDate d(2025, 3, 15);
        QTuiDateEdit de(d);
        QCOMPARE(de.date(), d);
    }

    // ---------------------------------------------------------------
    // QTuiTimeEdit tests
    // ---------------------------------------------------------------

    void timeEdit_displaySectionsTimeOnly()
    {
        TestableTimeEdit te;
        auto sections = te.displaySections();
        QCOMPARE(sections.size(), 3);
        QCOMPARE(sections.at(0), TestableDateTimeEdit::SHour);
        QCOMPARE(sections.at(1), TestableDateTimeEdit::SMinute);
        QCOMPARE(sections.at(2), TestableDateTimeEdit::SSecond);
    }

    void timeEdit_defaultFormat()
    {
        QTuiTimeEdit te;
        QCOMPARE(te.displayFormat(), QStringLiteral("HH:mm:ss"));
    }

    void timeEdit_constructWithTime()
    {
        QTime t(14, 30, 0);
        QTuiTimeEdit te(t);
        QCOMPARE(te.time(), t);
    }

    // Typed digits, the calendar popup and the mouse

    static void typeDigits(QTuiWidget &w, const QString &digits)
    {
        for (const QChar ch : digits) {
            TuiKeyEvent ev(Qt::Key_0 + ch.digitValue(), Qt::NoModifier, QString(ch));
            QCoreApplication::sendEvent(&w, &ev);
        }
    }

    static void click(QTuiWidget &w, int x, int y)
    {
        TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, QPoint(x, y));
        QCoreApplication::sendEvent(&w, &ev);
    }

    static QTuiCalendarWidget *popupOf(QTuiWidget &edit)
    {
        for (QTuiWidget *child : edit.childWidgets()) {
            if (auto *calendar = qobject_cast<QTuiCalendarWidget *>(child))
                return calendar;
        }
        return nullptr;
    }

    void dateEdit_typedDigitsChangeTheSection()
    {
        QTuiDateEdit edit(QDate(2026, 10, 29));
        QVERIFY(edit.acceptsTextInput());
        sendKey(edit, Qt::Key_Right);
        sendKey(edit, Qt::Key_Right); // day

        typeDigits(edit, QStringLiteral("1"));
        QCOMPARE(edit.date(), QDate(2026, 10, 1));
        typeDigits(edit, QStringLiteral("5"));
        QCOMPARE(edit.date(), QDate(2026, 10, 15));

        // "45" is not a day: the second digit starts again
        typeDigits(edit, QStringLiteral("45"));
        QCOMPARE(edit.date(), QDate(2026, 10, 5));
        // A leading zero waits for the next digit
        sendKey(edit, Qt::Key_Left);
        sendKey(edit, Qt::Key_Right);
        typeDigits(edit, QStringLiteral("0"));
        QCOMPARE(edit.date(), QDate(2026, 10, 5));
        typeDigits(edit, QStringLiteral("7"));
        QCOMPARE(edit.date(), QDate(2026, 10, 7));

        // The focus change discards the digit that waits
        QTuiDateEdit other;
        edit.setFocus();
        typeDigits(edit, QStringLiteral("1"));
        other.setFocus();
        edit.setFocus();
        typeDigits(edit, QStringLiteral("2"));
        QCOMPARE(edit.date(), QDate(2026, 10, 2));
        typeDigits(edit, QStringLiteral("07"));
        QCOMPARE(edit.date(), QDate(2026, 10, 7));

        // A year needs 4 digits; then the month is the current section
        sendKey(edit, Qt::Key_Left);
        sendKey(edit, Qt::Key_Left);
        typeDigits(edit, QStringLiteral("202"));
        QCOMPARE(edit.date(), QDate(2026, 10, 7));
        typeDigits(edit, QStringLiteral("7"));
        QCOMPARE(edit.date(), QDate(2027, 10, 7));
        typeDigits(edit, QStringLiteral("02"));
        QCOMPARE(edit.date(), QDate(2027, 2, 7));
        // The full month moved to the day
        typeDigits(edit, QStringLiteral("28"));
        QCOMPARE(edit.date(), QDate(2027, 2, 28));
        // February has no day 30
        typeDigits(edit, QStringLiteral("30"));
        QCOMPARE(edit.date(), QDate(2027, 2, 3));
    }

    void timeEdit_typedDigits()
    {
        QTuiTimeEdit edit(QTime(8, 15, 0));
        typeDigits(edit, QStringLiteral("2359"));
        QCOMPARE(edit.time(), QTime(23, 59, 0));
        // 7 then 5 in the seconds: 75 is not valid, so 5 starts again
        typeDigits(edit, QStringLiteral("75"));
        QCOMPARE(edit.time(), QTime(23, 59, 5));
    }

    void dateEdit_clickSelectsTheSection()
    {
        QTuiDateEdit edit(QDate(2026, 10, 29));
        edit.setGeometry(QRect(2, 1, 14, 3)); // "│ 2026-10-29 │", text from x = 4
        edit.clearDirtyRegions();
        click(edit, 12, 2); // the "2" of the day
        QVERIFY(edit.isDirty());
        sendKey(edit, Qt::Key_Up);
        QCOMPARE(edit.date(), QDate(2026, 10, 30));

        click(edit, 9, 2); // the "1" of the month
        sendKey(edit, Qt::Key_Up);
        QCOMPARE(edit.date(), QDate(2026, 11, 30));

        // A click on a separator or on a different row changes nothing
        click(edit, 8, 2);
        click(edit, 5, 1);
        sendKey(edit, Qt::Key_Down);
        QCOMPARE(edit.date(), QDate(2026, 10, 30));
    }

    void dateEdit_calendarPopup()
    {
        QTuiDateEdit edit(QDate(2026, 10, 29));
        const int plainWidth = edit.minimumSize().width();

        // Without the property, F4 opens nothing and the edit has no "▼"
        sendKey(edit, Qt::Key_F4);
        QVERIFY(!popupOf(edit));

        edit.clearDirtyRegions();
        edit.setCalendarPopup(true);
        QVERIFY(edit.isDirty());
        QCOMPARE(edit.minimumSize().width(), plainWidth + 2);
        edit.setGeometry(QRect(0, 0, plainWidth + 2, 3));
        ScreenBuffer buffer(30, 12);
        edit.paintEvent(buffer);
        QCOMPARE(buffer.backCell(plainWidth, 1).ch, U'\u25BC');
        QCOMPARE(buffer.backCell(2, 1).text(), QStringLiteral("2"));

        // F4 opens the calendar below the edit, on the date of the edit
        sendKey(edit, Qt::Key_F4);
        QTuiCalendarWidget *calendar = popupOf(edit);
        QVERIFY(calendar);
        QVERIFY(calendar->isVisible());
        QVERIFY(calendar->isPopup());
        QVERIFY(calendar->hasFocus());
        QCOMPARE(calendar->geometry().topLeft(), QPoint(0, 3));
        QCOMPARE(calendar->selectedDate(), QDate(2026, 10, 29));

        // Escape closes it without a change
        sendKey(*calendar, Qt::Key_Left);
        sendKey(*calendar, Qt::Key_Escape);
        QVERIFY(!calendar->isVisible());
        QCOMPARE(edit.date(), QDate(2026, 10, 29));

        // Alt+Down opens it and does not step the section; Enter sets the date
        sendKey(edit, Qt::Key_Down, Qt::AltModifier);
        QVERIFY(calendar->isVisible());
        QCOMPARE(edit.date(), QDate(2026, 10, 29));
        // The calendar starts on the date of the edit again, not on the
        // cursor of the last time
        sendKey(*calendar, Qt::Key_Left);
        sendKey(*calendar, Qt::Key_Return);
        QVERIFY(!calendar->isVisible());
        QCOMPARE(edit.date(), QDate(2026, 10, 28));

        // A click on "▼" opens and closes it; a click on a day sets the date
        click(edit, plainWidth, 1);
        QVERIFY(calendar->isVisible());
        click(edit, plainWidth, 1);
        QVERIFY(!calendar->isVisible());
        click(edit, plainWidth, 1);
        // 2026-10-01 is a Thursday: row 2 of the calendar, the fourth day column
        const QRect cal = calendar->geometry();
        click(*calendar, cal.x() + 1 + 3 * 3, cal.y() + 2);
        QVERIFY(!calendar->isVisible());
        QCOMPARE(edit.date(), QDate(2026, 10, 1));

        // A hidden edit closes its calendar
        sendKey(edit, Qt::Key_F4);
        QVERIFY(calendar->isVisible());
        edit.setVisible(false);
        QVERIFY(!calendar->isVisible());
    }

    void timeEdit_hasNoCalendarPopup()
    {
        QTuiTimeEdit edit(QTime(8, 15, 0));
        const int width = edit.minimumSize().width();
        edit.setCalendarPopup(true);
        QCOMPARE(edit.minimumSize().width(), width);
        sendKey(edit, Qt::Key_F4);
        QVERIFY(!popupOf(edit));
    }

    // ---------------------------------------------------------------
    // QTuiLCDNumber tests
    // ---------------------------------------------------------------

    void lcdNumber_defaultConstruction()
    {
        QTuiLCDNumber lcd;
        QCOMPARE(lcd.digitCount(), 5);
        QCOMPARE(lcd.mode(), QTuiLCDNumber::Decimal);
        QCOMPARE(lcd.intValue(), 0);
    }

    void lcdNumber_displayInt()
    {
        QTuiLCDNumber lcd;
        lcd.display(42);
        QCOMPARE(lcd.intValue(), 42);
        QCOMPARE(lcd.doubleValue(), 42.0);
    }

    void lcdNumber_displayDouble()
    {
        QTuiLCDNumber lcd;
        lcd.display(3.14);
        QVERIFY(qAbs(lcd.doubleValue() - 3.14) < 0.001);
    }

    void lcdNumber_displayString()
    {
        QTuiLCDNumber lcd;
        lcd.display(QStringLiteral("HELLO"));
        // String display stores the text; intValue/doubleValue from toInt/toDouble
        QCOMPARE(lcd.intValue(), 0);
    }

    void lcdNumber_setDigitCount()
    {
        QTuiLCDNumber lcd;
        lcd.setDigitCount(10);
        QCOMPARE(lcd.digitCount(), 10);

        // Minimum digit count is 1
        lcd.setDigitCount(0);
        QCOMPARE(lcd.digitCount(), 1);
    }

    void lcdNumber_setMode()
    {
        QTuiLCDNumber lcd;

        lcd.setMode(QTuiLCDNumber::Decimal);
        QCOMPARE(lcd.mode(), QTuiLCDNumber::Decimal);

        lcd.setMode(QTuiLCDNumber::Hexadecimal);
        QCOMPARE(lcd.mode(), QTuiLCDNumber::Hexadecimal);

        lcd.setMode(QTuiLCDNumber::Octal);
        QCOMPARE(lcd.mode(), QTuiLCDNumber::Octal);

        lcd.setMode(QTuiLCDNumber::Binary);
        QCOMPARE(lcd.mode(), QTuiLCDNumber::Binary);
    }

    void lcdNumber_setModeKeepsTheDoubleValue()
    {
        QTuiLCDNumber lcd;
        lcd.display(2.75);
        lcd.setMode(QTuiLCDNumber::Hexadecimal);
        QCOMPARE(lcd.intValue(), 3);
        QCOMPARE(lcd.doubleValue(), 2.75);
        lcd.setMode(QTuiLCDNumber::Decimal);
        QCOMPARE(lcd.doubleValue(), 2.75);
    }

    void lcdNumber_hexMode()
    {
        QTuiLCDNumber lcd;
        lcd.setMode(QTuiLCDNumber::Hexadecimal);
        lcd.display(255);
        QCOMPARE(lcd.intValue(), 255);
    }

    void lcdNumber_octalMode()
    {
        QTuiLCDNumber lcd;
        lcd.setMode(QTuiLCDNumber::Octal);
        lcd.display(8);
        QCOMPARE(lcd.intValue(), 8);
    }

    void lcdNumber_binaryMode()
    {
        QTuiLCDNumber lcd;
        lcd.setMode(QTuiLCDNumber::Binary);
        lcd.display(5);
        QCOMPARE(lcd.intValue(), 5);
    }

    void lcdNumber_overflowDetected()
    {
        QTuiLCDNumber lcd;
        lcd.setDigitCount(2);
        QSignalSpy spy(&lcd, &QTuiLCDNumber::overflow);

        lcd.display(100); // 3 digits > 2 digit count
        QCOMPARE(spy.count(), 1);
    }

    void lcdNumber_noOverflow()
    {
        QTuiLCDNumber lcd;
        lcd.setDigitCount(3);
        QSignalSpy spy(&lcd, &QTuiLCDNumber::overflow);

        lcd.display(100); // 3 digits == 3 digit count
        QCOMPARE(spy.count(), 0);
    }

    void lcdNumber_checkOverflowInt()
    {
        QTuiLCDNumber lcd;
        lcd.setDigitCount(2);

        QCOMPARE(lcd.checkOverflow(99), false);  // 2 digits fits in 2
        QCOMPARE(lcd.checkOverflow(100), true);   // 3 digits > 2
        QCOMPARE(lcd.checkOverflow(9), false);    // 1 digit fits in 2
    }

    void lcdNumber_checkOverflowDouble()
    {
        QTuiLCDNumber lcd;
        lcd.setDigitCount(3);

        QCOMPARE(lcd.checkOverflow(9.99), false);   // digits: 3 (9, 9, 9) fits
        QCOMPARE(lcd.checkOverflow(99.99), true);    // digits: 4 (9, 9, 9, 9) > 3
    }

    void lcdNumber_segmentArtViaMinimumSize()
    {
        // segmentArt is private, so we test indirectly via minimumSize
        // which uses digitCount to compute width based on 3-char-wide segments
        QTuiLCDNumber lcd(3);
        QSize minSize = lcd.minimumSize();
        // 3 digits * 3 chars + 2 gaps + 2 borders = 13 wide, 5 tall
        QCOMPARE(minSize.width(), 13);
        QCOMPARE(minSize.height(), 5);
    }
};

QTEST_MAIN(TestQTuiDateTimeLCD)
#include "test_qtui_datetime_lcd.moc"
