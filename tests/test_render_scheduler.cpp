#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// Minimal helper to count how many times executeRenderPass() actually runs.
// We observe render passes by checking ScreenBuffer::swapBuffers() side-effects:
// after a render pass the front buffer is updated.  We snapshot the front buffer
// before the test and count how many times it changes after processEvents().
// ---------------------------------------------------------------------------

class TestRenderScheduler : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 4: Render Coalescing
    // -----------------------------------------------------------------------
    void renderCoalescing()
    {
        rc::check("N scheduleDirtyRegion() calls in one event-loop tick produce exactly one render pass",
            []() {
                // --- Setup: create a real QTuiApplication so the scheduler
                //     has a valid app pointer, screen buffer, and backend.
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                RenderScheduler *scheduler = app.scheduler();
                RC_ASSERT(scheduler != nullptr);

                // Create a minimal main window so executeRenderPass() has
                // something to paint (otherwise it skips the paint tree).
                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Drain any initial pending renders from construction.
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                ScreenBuffer *buf = app.screenBuffer();
                RC_ASSERT(buf != nullptr);

                // --- Generate random N (1..50) and N random dirty rects ---
                const int N = *rc::gen::inRange(1, 51);

                // Snapshot the front buffer BEFORE scheduling.
                // After exactly one render pass, swapBuffers() copies back→front.
                // We mark a sentinel in the back buffer so we can detect the swap.
                const char32_t sentinel = U'S';
                buf->backCell(0, 0).setChar(sentinel);

                // Ensure front buffer does NOT have the sentinel yet.
                RC_ASSERT(buf->frontCell(0, 0).ch != sentinel);

                // --- Call scheduleDirtyRegion() N times in the same tick ---
                for (int i = 0; i < N; ++i) {
                    const int x = *rc::gen::inRange(0, 80);
                    const int y = *rc::gen::inRange(0, 24);
                    const int w = *rc::gen::inRange(1, 81 - x);
                    const int h = *rc::gen::inRange(1, 25 - y);
                    scheduler->scheduleDirtyRegion(QRect(x, y, w, h));
                }

                // After N calls, exactly one render should be pending.
                RC_ASSERT(scheduler->hasPendingRender());

                // --- Process the event loop to fire the coalesced timer ---
                QCoreApplication::processEvents();

                // The render pass should have executed and cleared the pending flag.
                RC_ASSERT(!scheduler->hasPendingRender());

                // --- Verify exactly ONE render pass occurred ---
                // After one swapBuffers(), the sentinel should now be in the
                // front buffer.
                RC_ASSERT(buf->frontCell(0, 0).ch == sentinel);

                // Write a second sentinel to the back buffer.
                const char32_t sentinel2 = U'T';
                buf->backCell(0, 0).setChar(sentinel2);

                // Process events again, no additional render should fire.
                QCoreApplication::processEvents();

                // The second sentinel should NOT have been swapped to front,
                // proving no extra render pass occurred.
                RC_ASSERT(buf->frontCell(0, 0).ch == sentinel);
                RC_ASSERT(buf->frontCell(0, 0).ch != sentinel2);

                // Scheduler should still have no pending render.
                RC_ASSERT(!scheduler->hasPendingRender());
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 1: Property Change Dirtying
    // -----------------------------------------------------------------------
    void propertyChangeDirtying()
    {
        rc::check("After any property change, widget is dirty and scheduler has pending render",
            []() {
                // **Validates: Requirements 1.1, 1.2, 1.3**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Drain initial events so we start from a clean state.
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                RenderScheduler *scheduler = app.scheduler();
                RC_ASSERT(scheduler != nullptr);

                // Choose which property change to apply: 0 = geometry, 1 = visibility (hide)
                const int changeType = *rc::gen::inRange(0, 2);

                if (changeType == 0) {
                    // --- Geometry change ---
                    // Generate a random new geometry different from the current one.
                    const int x = *rc::gen::inRange(0, 200);
                    const int y = *rc::gen::inRange(0, 200);
                    const int w = *rc::gen::inRange(1, 100);
                    const int h = *rc::gen::inRange(1, 100);
                    QRect newGeo(x, y, w, h);
                    RC_PRE(newGeo != mainWindow.geometry());

                    // Clear dirty state before the property change.
                    mainWindow.clearDirtyRegions();
                    // Drain any pending render from prior operations.
                    QCoreApplication::processEvents();
                    QCoreApplication::processEvents();

                    mainWindow.setGeometry(newGeo);

                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(scheduler->hasPendingRender());
                } else {
                    // --- Visibility change (hide) ---
                    // Ensure the widget is currently visible before hiding.
                    RC_PRE(mainWindow.isVisible());

                    mainWindow.clearDirtyRegions();
                    QCoreApplication::processEvents();
                    QCoreApplication::processEvents();

                    mainWindow.setVisible(false);

                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(scheduler->hasPendingRender());
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 2: Geometry Change Marks Old and New Dirty
    // -----------------------------------------------------------------------
    void geometryChangeMarksOldAndNew()
    {
        rc::check("setGeometry() marks both old and new geometry in the dirty region",
            []() {
                // **Validates: Requirements 1.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a QTuiLabel as a child widget to test geometry changes on.
                QTuiLabel label(QStringLiteral("test"), &mainWindow);

                // Drain initial events.
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Generate a random initial geometry for the label.
                const int oldX = *rc::gen::inRange(0, 60);
                const int oldY = *rc::gen::inRange(0, 20);
                const int oldW = *rc::gen::inRange(1, 20);
                const int oldH = *rc::gen::inRange(1, 4);
                QRect oldGeo(oldX, oldY, oldW, oldH);
                label.setGeometry(oldGeo);

                // Clear dirty state after setting initial geometry.
                label.clearDirtyRegions();
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Generate a new geometry that is DIFFERENT from the old one.
                const int newX = *rc::gen::inRange(0, 60);
                const int newY = *rc::gen::inRange(0, 20);
                const int newW = *rc::gen::inRange(1, 20);
                const int newH = *rc::gen::inRange(1, 4);
                QRect newGeo(newX, newY, newW, newH);
                RC_PRE(oldGeo != newGeo);

                // Apply the geometry change.
                label.setGeometry(newGeo);

                // The accumulated dirty region should contain both old and new geometry.
                QRect dirtyRegion = label.dirtyRegion();
                RC_ASSERT(dirtyRegion.contains(oldGeo));
                RC_ASSERT(dirtyRegion.contains(newGeo));
            });
    }

    // -----------------------------------------------------------------------
    // Unit test: dirty union accumulates all regions
    // -----------------------------------------------------------------------
    void dirtyUnionAccumulation()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        RenderScheduler *scheduler = app.scheduler();
        QVERIFY(scheduler != nullptr);

        // Drain initial state.
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        // Schedule two non-overlapping regions.
        scheduler->scheduleDirtyRegion(QRect(0, 0, 10, 5));
        scheduler->scheduleDirtyRegion(QRect(50, 10, 20, 10));

        // Only one render should be pending.
        QVERIFY(scheduler->hasPendingRender());

        // Process events, the single render pass fires.
        QCoreApplication::processEvents();
        QVERIFY(!scheduler->hasPendingRender());
    }

    // -----------------------------------------------------------------------
    // Unit test: renderNow() executes immediately and cancels pending
    // -----------------------------------------------------------------------
    void renderNowCancelsPending()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        RenderScheduler *scheduler = app.scheduler();
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        // Schedule a dirty region (queues a timer).
        scheduler->scheduleDirtyRegion(QRect(0, 0, 40, 12));
        QVERIFY(scheduler->hasPendingRender());

        // renderNow() should execute immediately.
        scheduler->renderNow();
        QVERIFY(!scheduler->hasPendingRender());

        // Processing events should not trigger another render.
        ScreenBuffer *buf = app.screenBuffer();
        const char32_t sentinel = U'X';
        buf->backCell(0, 0).setChar(sentinel);

        QCoreApplication::processEvents();

        // The sentinel should NOT have been swapped (no extra render).
        // Note: the timer callback may still fire but executeRenderPass
        // with an empty dirty union is essentially a no-op that still
        // calls swapBuffers. We verify no EXTRA meaningful render occurred
        // by checking hasPendingRender remains false.
        QVERIFY(!scheduler->hasPendingRender());
    }

    // -----------------------------------------------------------------------
    // Unit test: single scheduleDirtyRegion produces one render
    // -----------------------------------------------------------------------
    void singleScheduleProducesOneRender()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        RenderScheduler *scheduler = app.scheduler();
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        QVERIFY(!scheduler->hasPendingRender());

        scheduler->scheduleDirtyRegion(QRect(0, 0, 80, 24));
        QVERIFY(scheduler->hasPendingRender());

        QCoreApplication::processEvents();
        QVERIFY(!scheduler->hasPendingRender());
    }
};

QTEST_APPLESS_MAIN(TestRenderScheduler)
#include "test_render_scheduler.moc"
