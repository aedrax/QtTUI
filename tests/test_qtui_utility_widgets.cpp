#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiFocusFrame.h>
#include <QtTUI/QTuiSizeGrip.h>
#include <QtTUI/QTuiCalendarWidget.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// Simple focusable test widget
class UtilTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit UtilTestWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        for (int row = 0; row < geo.height(); ++row)
            for (int col = 0; col < geo.width(); ++col) {
                int px = geo.x() + col;
                int py = geo.y() + row;
                if (px < buffer.width() && py < buffer.height())
                    buffer.backCell(px, py).character = QLatin1Char('X');
            }
    }
    QSize minimumSize() const override { return QSize(4, 2); }
};

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiUtilityWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiFocusFrame tests
    // ---------------------------------------------------------------

    void focusFrame_defaultConstruction()
    {
        QTuiFocusFrame ff;
        QVERIFY(ff.widget() == nullptr);
        QCOMPARE(ff.isVisible(), false);
    }

    void focusFrame_setWidget()
    {
        QTuiFocusFrame ff;
        UtilTestWidget target;
        target.setGeometry(QRect(5, 5, 10, 4));

        ff.setWidget(&target);
        QCOMPARE(ff.widget(), &target);
    }

    void focusFrame_visibleWhenTargetHasFocus()
    {
        QTuiFocusFrame ff;
        UtilTestWidget target;
        target.setGeometry(QRect(5, 5, 10, 4));
        ff.setWidget(&target);

        QCOMPARE(ff.isVisible(), false);

        // Give target focus
        target.setFocus();
        QCOMPARE(ff.isVisible(), true);
    }

    void focusFrame_hiddenWhenTargetLosesFocus()
    {
        UtilTestWidget target;
        UtilTestWidget other;
        target.setGeometry(QRect(5, 5, 10, 4));
        other.setGeometry(QRect(20, 5, 10, 4));

        QTuiFocusFrame ff;
        ff.setWidget(&target);

        // Give target focus, frame should be visible
        target.setFocus();
        QCOMPARE(ff.isVisible(), true);

        // Move focus to another widget, frame should hide
        other.setFocus();
        QCOMPARE(ff.isVisible(), false);
    }

    void focusFrame_geometryWrapsTarget()
    {
        QTuiFocusFrame ff;
        UtilTestWidget target;
        target.setGeometry(QRect(5, 5, 10, 4));
        ff.setWidget(&target);

        // Give focus so frame updates geometry
        target.setFocus();

        // Frame should be 1 cell larger on each side
        const QRect expected(4, 4, 12, 6);
        QCOMPARE(ff.geometry(), expected);
    }

    void focusFrame_setWidgetNull()
    {
        QTuiFocusFrame ff;
        UtilTestWidget target;
        target.setGeometry(QRect(5, 5, 10, 4));
        ff.setWidget(&target);
        target.setFocus();
        QCOMPARE(ff.isVisible(), true);

        // Setting widget to nullptr should disconnect and hide
        ff.setWidget(nullptr);
        QVERIFY(ff.widget() == nullptr);
        QCOMPARE(ff.isVisible(), false);
    }

    void focusFrame_switchTarget()
    {
        UtilTestWidget target1;
        UtilTestWidget target2;
        target1.setGeometry(QRect(5, 5, 10, 4));
        target2.setGeometry(QRect(20, 10, 8, 3));

        QTuiFocusFrame ff;
        ff.setWidget(&target1);
        target1.setFocus();
        QCOMPARE(ff.isVisible(), true);
        QCOMPARE(ff.widget(), &target1);

        // Switch to target2 (which doesn't have focus)
        ff.setWidget(&target2);
        QCOMPARE(ff.widget(), &target2);
        QCOMPARE(ff.isVisible(), false);

        // Give target2 focus
        target2.setFocus();
        QCOMPARE(ff.isVisible(), true);

        // Old target1 gaining focus should NOT affect the frame anymore
        // (frame is now tracking target2)
        const QRect expectedGeo(19, 9, 10, 5);
        QCOMPARE(ff.geometry(), expectedGeo);
    }

    // ---------------------------------------------------------------
    // QTuiSizeGrip tests
    // ---------------------------------------------------------------

    void sizeGrip_defaultConstruction()
    {
        QTuiSizeGrip grip;
        // Should inherit QTuiWidget (verified by compilation)
        QVERIFY(qobject_cast<QTuiWidget *>(&grip) != nullptr);
    }

    void sizeGrip_minimumSize()
    {
        QTuiSizeGrip grip;
        QCOMPARE(grip.minimumSize(), QSize(3, 3));
    }

    void sizeGrip_paintEventRendersGrip()
    {
        QTuiSizeGrip grip;
        grip.setGeometry(QRect(0, 0, 3, 3));

        ScreenBuffer buffer(3, 3);
        grip.paintEvent(buffer);

        // The diagonal pattern should have non-space characters
        // Pattern: row0 col2, row1 col1, row2 col0
        bool foundDiag = false;
        for (int i = 0; i < 3; ++i) {
            const Cell &cell = buffer.backCell(2 - i, i);
            if (cell.character != QLatin1Char(' ')) {
                foundDiag = true;
                break;
            }
        }
        QVERIFY(foundDiag);
    }

    void sizeGrip_arrowKeysResizeParent()
    {
        QTuiMainWindow mw;
        mw.setGeometry(QRect(0, 0, 40, 20));

        auto *grip = new QTuiSizeGrip(&mw);
        grip->setGeometry(QRect(37, 17, 3, 3));

        // Right arrow should increase width
        sendKey(*grip, Qt::Key_Right);
        QCOMPARE(mw.geometry().width(), 41);

        // Down arrow should increase height
        sendKey(*grip, Qt::Key_Down);
        QCOMPARE(mw.geometry().height(), 21);

        // Left arrow should decrease width
        sendKey(*grip, Qt::Key_Left);
        QCOMPARE(mw.geometry().width(), 40);

        // Up arrow should decrease height
        sendKey(*grip, Qt::Key_Up);
        QCOMPARE(mw.geometry().height(), 20);
    }

    // ---------------------------------------------------------------
    // QTuiCalendarWidget tests
    // ---------------------------------------------------------------

    void calendar_defaultConstruction()
    {
        QTuiCalendarWidget cal;
        const QDate today = QDate::currentDate();
        QCOMPARE(cal.selectedDate(), today);
        QCOMPARE(cal.yearShown(), today.year());
        QCOMPARE(cal.monthShown(), today.month());
    }

    void calendar_setSelectedDate()
    {
        QTuiCalendarWidget cal;
        QSignalSpy spy(&cal, &QTuiCalendarWidget::selectionChanged);

        const QDate newDate(2024, 6, 15);
        cal.setSelectedDate(newDate);
        QCOMPARE(cal.selectedDate(), newDate);
        QCOMPARE(spy.count(), 1);

        // Setting same date again should not emit
        cal.setSelectedDate(newDate);
        QCOMPARE(spy.count(), 1);
    }

    void calendar_minimumSize()
    {
        QTuiCalendarWidget cal;
        QCOMPARE(cal.minimumSize(), QSize(22, 9));
    }

    void calendar_monthNavigation()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 15));
        QCOMPARE(cal.monthShown(), 3);
        QCOMPARE(cal.yearShown(), 2025);

        // PageUp goes to previous month
        sendKey(cal, Qt::Key_PageUp);
        QCOMPARE(cal.monthShown(), 2);
        QCOMPARE(cal.yearShown(), 2025);

        // PageDown goes to next month
        sendKey(cal, Qt::Key_PageDown);
        QCOMPARE(cal.monthShown(), 3);
        QCOMPARE(cal.yearShown(), 2025);

        // PageUp from January wraps to December of previous year
        cal.setSelectedDate(QDate(2025, 1, 10));
        sendKey(cal, Qt::Key_PageUp);
        QCOMPARE(cal.monthShown(), 12);
        QCOMPARE(cal.yearShown(), 2024);
    }

    void calendar_dayNavigation()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 15));

        // Right moves cursor +1 day, Left moves -1 day
        sendKey(cal, Qt::Key_Right);
        // Confirm by pressing Enter and checking activated signal
        QSignalSpy activatedSpy(&cal, &QTuiCalendarWidget::activated);
        sendKey(cal, Qt::Key_Return);
        QCOMPARE(activatedSpy.count(), 1);
        QCOMPARE(activatedSpy.at(0).at(0).toDate(), QDate(2025, 3, 16));

        // Reset
        cal.setSelectedDate(QDate(2025, 3, 15));

        // Up moves -7 days
        sendKey(cal, Qt::Key_Up);
        QSignalSpy activatedSpy2(&cal, &QTuiCalendarWidget::activated);
        sendKey(cal, Qt::Key_Return);
        QCOMPARE(activatedSpy2.count(), 1);
        QCOMPARE(activatedSpy2.at(0).at(0).toDate(), QDate(2025, 3, 8));

        // Reset
        cal.setSelectedDate(QDate(2025, 3, 15));

        // Down moves +7 days
        sendKey(cal, Qt::Key_Down);
        QSignalSpy activatedSpy3(&cal, &QTuiCalendarWidget::activated);
        sendKey(cal, Qt::Key_Return);
        QCOMPARE(activatedSpy3.count(), 1);
        QCOMPARE(activatedSpy3.at(0).at(0).toDate(), QDate(2025, 3, 22));
    }

    void calendar_enterSelectsDate()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 15));

        // Move cursor right by one day
        sendKey(cal, Qt::Key_Right);

        QSignalSpy activatedSpy(&cal, &QTuiCalendarWidget::activated);
        QSignalSpy selectionSpy(&cal, &QTuiCalendarWidget::selectionChanged);

        sendKey(cal, Qt::Key_Return);

        QCOMPARE(activatedSpy.count(), 1);
        QCOMPARE(activatedSpy.at(0).at(0).toDate(), QDate(2025, 3, 16));
        // selectionChanged emitted because cursor date differs from old selected
        QCOMPARE(selectionSpy.count(), 1);
        QCOMPARE(cal.selectedDate(), QDate(2025, 3, 16));
    }

    void calendar_minMaxConstraints()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 15));

        cal.setMinimumDate(QDate(2025, 3, 10));
        cal.setMaximumDate(QDate(2025, 3, 20));

        // Try to navigate before minimum: Left many times
        for (int i = 0; i < 10; ++i)
            sendKey(cal, Qt::Key_Left);

        // Press Enter to select, should be clamped to min
        QSignalSpy activatedSpy(&cal, &QTuiCalendarWidget::activated);
        sendKey(cal, Qt::Key_Return);
        QVERIFY(activatedSpy.count() >= 1);
        const QDate selected = activatedSpy.last().at(0).toDate();
        QVERIFY(selected >= QDate(2025, 3, 10));

        // Try to navigate past maximum
        cal.setSelectedDate(QDate(2025, 3, 15));
        for (int i = 0; i < 10; ++i)
            sendKey(cal, Qt::Key_Right);

        QSignalSpy activatedSpy2(&cal, &QTuiCalendarWidget::activated);
        sendKey(cal, Qt::Key_Return);
        QVERIFY(activatedSpy2.count() >= 1);
        const QDate selected2 = activatedSpy2.last().at(0).toDate();
        QVERIFY(selected2 <= QDate(2025, 3, 20));
    }

    void calendar_setMinimumDateClampsCurrent()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 5));

        QSignalSpy spy(&cal, &QTuiCalendarWidget::selectionChanged);

        // Setting minimum after current date should clamp
        cal.setMinimumDate(QDate(2025, 3, 10));
        QCOMPARE(cal.selectedDate(), QDate(2025, 3, 10));
        QCOMPARE(spy.count(), 1);
    }

    void calendar_setMaximumDateClampsCurrent()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 25));

        QSignalSpy spy(&cal, &QTuiCalendarWidget::selectionChanged);

        // Setting maximum before current date should clamp
        cal.setMaximumDate(QDate(2025, 3, 20));
        QCOMPARE(cal.selectedDate(), QDate(2025, 3, 20));
        QCOMPARE(spy.count(), 1);
    }

    void calendar_paintEventRendersGrid()
    {
        QTuiCalendarWidget cal;
        cal.setSelectedDate(QDate(2025, 3, 15));
        cal.setGeometry(QRect(0, 0, 22, 9));

        ScreenBuffer buffer(22, 9);
        cal.paintEvent(buffer);

        // Row 0 should contain month name header
        QString headerRow;
        for (int col = 0; col < 22; ++col)
            headerRow += buffer.backCell(col, 0).character;
        QVERIFY(headerRow.contains(QStringLiteral("2025")));

        // Row 1 should contain weekday abbreviations
        QString weekdayRow;
        for (int col = 0; col < 22; ++col)
            weekdayRow += buffer.backCell(col, 1).character;
        QVERIFY(weekdayRow.contains(QStringLiteral("Mo")));
        QVERIFY(weekdayRow.contains(QStringLiteral("Su")));

        // Day numbers should appear in the grid (rows 2-7)
        // Look for "15" somewhere in the grid
        bool found15 = false;
        for (int row = 2; row < 8; ++row) {
            QString gridRow;
            for (int col = 0; col < 22; ++col)
                gridRow += buffer.backCell(col, row).character;
            if (gridRow.contains(QStringLiteral("15"))) {
                found15 = true;
                break;
            }
        }
        QVERIFY(found15);
    }
};

QTEST_MAIN(TestQTuiUtilityWidgets)
#include "test_qtui_utility_widgets.moc"
