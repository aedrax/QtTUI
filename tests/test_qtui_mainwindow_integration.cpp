#include <QtTest/QtTest>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/QTuiStatusBar.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Concrete widget for testing since QTuiWidget is abstract
class StubWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit StubWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
};

class TestMainWindowIntegration : public QObject {
    Q_OBJECT

private:
    void resizeWindow(QTuiMainWindow &mw, int w, int h)
    {
        TuiResizeEvent ev(QSize(w, h));
        QCoreApplication::sendEvent(&mw, &ev);
    }

private slots:

    // --- 34.1: Accessor tests ---

    void setMenuBarStoresAndReturns()
    {
        QTuiMainWindow mw;
        QVERIFY(mw.menuBar() == nullptr);

        auto *mb = new QTuiMenuBar;
        mw.setMenuBar(mb);
        QCOMPARE(mw.menuBar(), mb);
    }

    void addToolBarStoresAndReturns()
    {
        QTuiMainWindow mw;
        QVERIFY(mw.toolBars().isEmpty());

        auto *tb1 = new QTuiToolBar;
        auto *tb2 = new QTuiToolBar;
        mw.addToolBar(tb1);
        mw.addToolBar(tb2);
        QCOMPARE(mw.toolBars().size(), 2);
        QCOMPARE(mw.toolBars().at(0), tb1);
        QCOMPARE(mw.toolBars().at(1), tb2);
    }

    void removeToolBarWorks()
    {
        QTuiMainWindow mw;
        auto *tb = new QTuiToolBar;
        mw.addToolBar(tb);
        QCOMPARE(mw.toolBars().size(), 1);

        mw.removeToolBar(tb);
        QVERIFY(mw.toolBars().isEmpty());
    }

    void setStatusBarStoresAndReturns()
    {
        QTuiMainWindow mw;
        QVERIFY(mw.statusBar() == nullptr);

        auto *sb = new QTuiStatusBar;
        mw.setStatusBar(sb);
        QCOMPARE(mw.statusBar(), sb);
    }

    void addDockWidgetStores()
    {
        QTuiMainWindow mw;
        auto *dock = new QTuiDockWidget(QStringLiteral("Test"));
        mw.addDockWidget(Qt::LeftDockWidgetArea, dock);
        // Dock should be a child of the main window
        QCOMPARE(dock->parentWidget(), &mw);
    }

    void removeDockWidgetDetaches()
    {
        QTuiMainWindow mw;
        auto *dock = new QTuiDockWidget(QStringLiteral("Test"));
        mw.addDockWidget(Qt::LeftDockWidgetArea, dock);
        mw.removeDockWidget(dock);
        QVERIFY(dock->parentWidget() != &mw);
        delete dock;
    }

    // --- 34.2 / 34.5 / 34.6: Layout allocation order tests ---

    void layoutMenuBarAtTop()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *mb = new QTuiMenuBar;
        mw.setMenuBar(mb);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        // Menu bar should be at row 0, height 1
        QCOMPARE(mb->geometry().y(), 0);
        QCOMPARE(mb->geometry().height(), 1);
        QCOMPARE(mb->geometry().width(), 80);

        // Central widget should start at row 1
        QCOMPARE(central->geometry().y(), 1);
        QCOMPARE(central->geometry().height(), 23);
    }

    void layoutToolbarsBelowMenuBar()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *mb = new QTuiMenuBar;
        mw.setMenuBar(mb);

        auto *tb1 = new QTuiToolBar;
        auto *tb2 = new QTuiToolBar;
        mw.addToolBar(tb1);
        mw.addToolBar(tb2);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        // Menu bar at row 0
        QCOMPARE(mb->geometry().y(), 0);
        // Toolbar 1 at row 1
        QCOMPARE(tb1->geometry().y(), 1);
        QCOMPARE(tb1->geometry().height(), 1);
        // Toolbar 2 at row 2
        QCOMPARE(tb2->geometry().y(), 2);
        QCOMPARE(tb2->geometry().height(), 1);
        // Central starts at row 3
        QCOMPARE(central->geometry().y(), 3);
    }

    void layoutStatusBarAtBottom()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *sb = new QTuiStatusBar;
        mw.setStatusBar(sb);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        // Status bar at bottom row (row 23)
        QCOMPARE(sb->geometry().y(), 23);
        QCOMPARE(sb->geometry().height(), 1);
        QCOMPARE(sb->geometry().width(), 80);

        // Central widget should not overlap status bar
        QCOMPARE(central->geometry().y(), 0);
        QCOMPARE(central->geometry().height(), 23);
    }

    void layoutFullComponentOrder()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *mb = new QTuiMenuBar;
        mw.setMenuBar(mb);

        auto *tb = new QTuiToolBar;
        mw.addToolBar(tb);

        auto *sb = new QTuiStatusBar;
        mw.setStatusBar(sb);

        auto *leftDock = new QTuiDockWidget(QStringLiteral("Left"));
        mw.addDockWidget(Qt::LeftDockWidgetArea, leftDock);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        // Menu bar: row 0, full width
        QCOMPARE(mb->geometry(), QRect(0, 0, 80, 1));

        // Toolbar: row 1, full width
        QCOMPARE(tb->geometry(), QRect(0, 1, 80, 1));

        // Status bar: bottom row 23, full width
        QCOMPARE(sb->geometry(), QRect(0, 23, 80, 1));

        // Left dock: starts at row 2 (below toolbar), ends at row 22 (above status bar)
        // Width comes from dock's minimumSize
        QRect dockGeo = leftDock->geometry();
        QCOMPARE(dockGeo.y(), 2);
        QCOMPARE(dockGeo.x(), 0);
        QCOMPARE(dockGeo.height(), 21); // rows 2..22

        // Central widget: to the right of left dock, between toolbar and status bar
        QRect centralGeo = central->geometry();
        QCOMPARE(centralGeo.x(), dockGeo.width());
        QCOMPARE(centralGeo.y(), 2);
        QCOMPARE(centralGeo.height(), 21);
        QCOMPARE(centralGeo.x() + centralGeo.width(), 80);
    }

    void resizeRecalculatesAllPositions()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *mb = new QTuiMenuBar;
        mw.setMenuBar(mb);

        auto *sb = new QTuiStatusBar;
        mw.setStatusBar(sb);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        // Verify initial layout
        QCOMPARE(mb->geometry(), QRect(0, 0, 80, 1));
        QCOMPARE(sb->geometry(), QRect(0, 23, 80, 1));
        QCOMPARE(central->geometry(), QRect(0, 1, 80, 22));

        // Resize to 100x30
        resizeWindow(mw, 100, 30);

        // All positions should be recalculated
        QCOMPARE(mb->geometry(), QRect(0, 0, 100, 1));
        QCOMPARE(sb->geometry(), QRect(0, 29, 100, 1));
        QCOMPARE(central->geometry(), QRect(0, 1, 100, 28));
    }

    void addRemoveDockUpdatesLayout()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        // Initially central fills everything
        QCOMPARE(central->geometry(), QRect(0, 0, 80, 24));

        // Add a left dock
        auto *dock = new QTuiDockWidget(QStringLiteral("Dock"));
        mw.addDockWidget(Qt::LeftDockWidgetArea, dock);

        // Central should now be narrower
        int dockWidth = dock->geometry().width();
        QVERIFY(dockWidth > 0);
        QCOMPARE(central->geometry().x(), dockWidth);
        QCOMPARE(central->geometry().width(), 80 - dockWidth);

        // Remove the dock
        mw.removeDockWidget(dock);

        // Central should fill everything again
        QCOMPARE(central->geometry(), QRect(0, 0, 80, 24));

        delete dock;
    }

    void rightDockAllocatesFromRight()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        auto *dock = new QTuiDockWidget(QStringLiteral("Right"));
        mw.addDockWidget(Qt::RightDockWidgetArea, dock);

        int dockWidth = dock->geometry().width();
        QVERIFY(dockWidth > 0);

        // Dock should be at the right edge
        QCOMPARE(dock->geometry().x() + dock->geometry().width(), 80);

        // Central should end where dock starts
        QCOMPARE(central->geometry().x(), 0);
        QCOMPARE(central->geometry().width(), 80 - dockWidth);
    }

    void topAndBottomDocks()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        auto *topDock = new QTuiDockWidget(QStringLiteral("Top"));
        mw.addDockWidget(Qt::TopDockWidgetArea, topDock);

        auto *bottomDock = new QTuiDockWidget(QStringLiteral("Bottom"));
        mw.addDockWidget(Qt::BottomDockWidgetArea, bottomDock);

        int topH = topDock->geometry().height();
        int bottomH = bottomDock->geometry().height();
        QVERIFY(topH > 0);
        QVERIFY(bottomH > 0);

        // Top dock at top
        QCOMPARE(topDock->geometry().y(), 0);
        // Bottom dock at bottom
        QCOMPARE(bottomDock->geometry().y() + bottomDock->geometry().height(), 24);
        // Central in between
        QCOMPARE(central->geometry().y(), topH);
        QCOMPARE(central->geometry().height(), 24 - topH - bottomH);
    }

    void replacingMenuBarRelayouts()
    {
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        auto *mb1 = new QTuiMenuBar;
        mw.setMenuBar(mb1);
        QCOMPARE(central->geometry().y(), 1);

        // Replace with a new menu bar, layout should still work
        auto *mb2 = new QTuiMenuBar;
        mw.setMenuBar(mb2);
        QCOMPARE(mw.menuBar(), mb2);
        QCOMPARE(mb2->geometry(), QRect(0, 0, 80, 1));
        QCOMPARE(central->geometry().y(), 1);

        delete mb1;
    }

    void noComponentsFullCentral()
    {
        // Without any window components, central widget should fill the entire window
        QTuiMainWindow mw;
        resizeWindow(mw, 80, 24);

        auto *central = new StubWidget;
        mw.setCentralWidget(central);

        QCOMPARE(central->geometry(), QRect(0, 0, 80, 24));
    }
};

QTEST_MAIN(TestMainWindowIntegration)
#include "test_qtui_mainwindow_integration.moc"
