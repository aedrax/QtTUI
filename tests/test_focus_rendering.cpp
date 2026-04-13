// Feature: widget-rendering-overhaul, Property 28: Focus Change Dirtying
#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/RenderScheduler.h>

using namespace QtTUI;

class TestFocusRendering : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 28: Focus Change Dirtying
    // -----------------------------------------------------------------------
    void focusChangeDirtying()
    {
        rc::check("Focus transfer from widget A to widget B marks both dirty",
            []() {
                // **Validates: Requirements 12.1, 12.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Generate random geometries for two labels
                const int x1 = *rc::gen::inRange(0, 61);
                const int y1 = *rc::gen::inRange(0, 61);
                const int w1 = *rc::gen::inRange(1, 21);
                const int h1 = *rc::gen::inRange(1, 21);

                const int x2 = *rc::gen::inRange(0, 61);
                const int y2 = *rc::gen::inRange(0, 61);
                const int w2 = *rc::gen::inRange(1, 21);
                const int h2 = *rc::gen::inRange(1, 21);

                QTuiLabel label1(QStringLiteral("Label A"), &mainWindow);
                label1.setGeometry(QRect(x1, y1, w1, h1));

                QTuiLabel label2(QStringLiteral("Label B"), &mainWindow);
                label2.setGeometry(QRect(x2, y2, w2, h2));

                // Drain initial events from construction
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Give focus to widget A
                label1.setFocus();

                // Drain pending renders so we start from a clean state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Clear dirty regions on both widgets
                label1.clearDirtyRegions();
                label2.clearDirtyRegions();

                // Drain any pending renders from clearing
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Verify both are clean before the focus transfer
                RC_ASSERT(!label1.isDirty());
                RC_ASSERT(!label2.isDirty());

                // Transfer focus from widget A to widget B
                label2.setFocus();

                // Verify: label1 (lost focus) should be marked dirty
                RC_ASSERT(label1.isDirty());

                // Verify: label2 (gained focus) should be marked dirty
                RC_ASSERT(label2.isDirty());

                // Verify: scheduler has a pending render
                RC_ASSERT(app.scheduler()->hasPendingRender());
            });
    }
};

QTEST_APPLESS_MAIN(TestFocusRendering)
#include "test_focus_rendering.moc"
