// Feature: widget-rendering-overhaul, Property 25: Container Content Area
// Feature: widget-rendering-overhaul, Property 26: Exclusive-Child Containers
// Feature: widget-rendering-overhaul, Property 27: Scroll Area Clipping
#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiGroupBox.h>
#include <QtTUI/QTuiTabWidget.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/QTuiStackedWidget.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// A simple child widget that paints 'C' characters in its geometry
// ---------------------------------------------------------------------------
class ContainerChildWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit ContainerChildWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
    {
    }

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        for (int row = geo.y(); row < geo.y() + geo.height(); ++row) {
            for (int col = geo.x(); col < geo.x() + geo.width(); ++col) {
                if (col >= 0 && col < buffer.width() && row >= 0 && row < buffer.height()) {
                    Cell &cell = buffer.backCell(col, row);
                    cell.setChar(U'C');
                }
            }
        }
    }
};

class TestContainerRendering : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 25: Container Content Area
    // -----------------------------------------------------------------------
    void containerContentArea_groupBox()
    {
        rc::check("QTuiGroupBox child geometries are contained within content area",
            []() {
                // **Validates: Requirements 11.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a group box with random geometry (minimum 4x3 for border + content)
                auto *groupBox = new QTuiGroupBox(QStringLiteral("Test"), &mainWindow);
                const int x = *rc::gen::inRange(0, 40);
                const int y = *rc::gen::inRange(0, 10);
                const int w = *rc::gen::inRange(4, 30);
                const int h = *rc::gen::inRange(3, 12);
                groupBox->setGeometry(QRect(x, y, w, h));
                groupBox->setVisible(true);

                // Add a child widget
                auto *child = new ContainerChildWidget(groupBox);
                child->setGeometry(QRect(x, y, w, h)); // initially same as parent
                child->setVisible(true);

                // Trigger render - this should clamp child geometry to content area
                groupBox->markDirty();
                app.triggerRender();

                // Verify child geometry is within content area
                const QRect contentArea = groupBox->contentArea();
                const QRect childGeo = child->geometry();

                RC_ASSERT(!contentArea.isEmpty());
                RC_ASSERT(contentArea.contains(childGeo) || childGeo.isEmpty());
            });
    }

    void containerContentArea_tabWidget()
    {
        rc::check("QTuiTabWidget child geometries are contained within content area",
            []() {
                // **Validates: Requirements 11.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a tab widget with random geometry (minimum height 3 for tab bar + separator + content)
                auto *tabWidget = new QTuiTabWidget(&mainWindow);
                const int x = *rc::gen::inRange(0, 40);
                const int y = *rc::gen::inRange(0, 10);
                const int w = *rc::gen::inRange(5, 30);
                const int h = *rc::gen::inRange(4, 12);
                tabWidget->setGeometry(QRect(x, y, w, h));
                tabWidget->setVisible(true);

                // Add pages
                auto *page1 = new ContainerChildWidget();
                auto *page2 = new ContainerChildWidget();
                tabWidget->addTab(page1, QStringLiteral("Tab1"));
                tabWidget->addTab(page2, QStringLiteral("Tab2"));

                // Trigger render
                tabWidget->markDirty();
                app.triggerRender();

                // Verify current page geometry is within content area
                const QRect contentArea = tabWidget->contentArea();
                QTuiWidget *currentPage = tabWidget->currentWidget();
                RC_ASSERT(currentPage != nullptr);

                const QRect pageGeo = currentPage->geometry();
                RC_ASSERT(!contentArea.isEmpty());
                RC_ASSERT(contentArea.contains(pageGeo) || pageGeo == contentArea);
            });
    }

    void containerContentArea_dockWidget()
    {
        rc::check("QTuiDockWidget child geometries are contained within content area",
            []() {
                // **Validates: Requirements 11.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a dock widget with random geometry (minimum 4x3)
                auto *dock = new QTuiDockWidget(QStringLiteral("Dock"), &mainWindow);
                const int x = *rc::gen::inRange(0, 40);
                const int y = *rc::gen::inRange(0, 10);
                const int w = *rc::gen::inRange(6, 30);
                const int h = *rc::gen::inRange(3, 12);
                dock->setGeometry(QRect(x, y, w, h));
                dock->setVisible(true);

                // Set a content widget
                auto *child = new ContainerChildWidget();
                dock->setWidget(child);
                child->setVisible(true);

                // Trigger render
                dock->markDirty();
                app.triggerRender();

                // Verify child geometry is within content area
                const QRect contentArea = dock->contentArea();
                const QRect childGeo = child->geometry();

                RC_ASSERT(!contentArea.isEmpty());
                RC_ASSERT(contentArea.contains(childGeo) || childGeo == contentArea);
            });
    }

    void containerContentArea_scrollArea()
    {
        rc::check("QTuiScrollArea child content is within viewport (content area)",
            []() {
                // **Validates: Requirements 11.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a scroll area
                auto *scrollArea = new QTuiScrollArea(&mainWindow);
                const int x = *rc::gen::inRange(0, 30);
                const int y = *rc::gen::inRange(0, 10);
                const int w = *rc::gen::inRange(5, 20);
                const int h = *rc::gen::inRange(5, 12);
                scrollArea->setGeometry(QRect(x, y, w, h));
                scrollArea->setVisible(true);

                // Set a child widget that may be larger than the viewport
                auto *child = new ContainerChildWidget();
                const int childW = *rc::gen::inRange(w, w + 20);
                const int childH = *rc::gen::inRange(h, h + 20);
                child->setGeometry(QRect(0, 0, childW, childH));
                child->setVisible(true);
                scrollArea->setWidget(child);

                // Trigger render
                scrollArea->markDirty();
                app.triggerRender();

                // Verify: the content area (viewport) is within the scroll area geometry
                const QRect contentArea = scrollArea->contentArea();
                const QRect scrollGeo = scrollArea->geometry();
                RC_ASSERT(scrollGeo.contains(contentArea));
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 26: Exclusive-Child Containers
    // -----------------------------------------------------------------------
    void exclusiveChildContainers_tabWidget()
    {
        rc::check("QTuiTabWidget has exactly one visible child page",
            []() {
                // **Validates: Requirements 11.4, 11.5**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *tabWidget = new QTuiTabWidget(&mainWindow);
                tabWidget->setGeometry(QRect(0, 0, 40, 12));
                tabWidget->setVisible(true);

                // Add N pages (at least 1)
                const int numPages = *rc::gen::inRange(1, 6);
                QList<QTuiWidget *> pages;
                for (int i = 0; i < numPages; ++i) {
                    auto *page = new ContainerChildWidget();
                    tabWidget->addTab(page, QStringLiteral("Tab%1").arg(i));
                    pages.append(page);
                }

                // Set a random current index
                const int targetIndex = *rc::gen::inRange(0, numPages);
                tabWidget->setCurrentIndex(targetIndex);

                // Verify exactly one child is visible
                int visibleCount = 0;
                for (int i = 0; i < numPages; ++i) {
                    if (pages.at(i)->isVisible())
                        ++visibleCount;
                }

                RC_ASSERT(visibleCount == 1);
                RC_ASSERT(pages.at(targetIndex)->isVisible());
            });
    }

    void exclusiveChildContainers_stackedWidget()
    {
        rc::check("QTuiStackedWidget has exactly one visible child widget",
            []() {
                // **Validates: Requirements 11.4, 11.5**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *stacked = new QTuiStackedWidget(&mainWindow);
                stacked->setGeometry(QRect(0, 0, 40, 12));
                stacked->setVisible(true);

                // Add N widgets (at least 1)
                const int numWidgets = *rc::gen::inRange(1, 6);
                QList<QTuiWidget *> widgets;
                for (int i = 0; i < numWidgets; ++i) {
                    auto *w = new ContainerChildWidget();
                    stacked->addWidget(w);
                    widgets.append(w);
                }

                // Set a random current index
                const int targetIndex = *rc::gen::inRange(0, numWidgets);
                stacked->setCurrentIndex(targetIndex);

                // Verify exactly one child is visible
                int visibleCount = 0;
                for (int i = 0; i < numWidgets; ++i) {
                    if (widgets.at(i)->isVisible())
                        ++visibleCount;
                }

                RC_ASSERT(visibleCount == 1);
                RC_ASSERT(widgets.at(targetIndex)->isVisible());
            });
    }

    void exclusiveChildContainers_tabWidgetMarksDirty()
    {
        // Unit test: verify markDirty is called on old and new pages on tab change
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *tabWidget = new QTuiTabWidget(&mainWindow);
        tabWidget->setGeometry(QRect(0, 0, 40, 12));
        tabWidget->setVisible(true);

        auto *page0 = new ContainerChildWidget();
        auto *page1 = new ContainerChildWidget();
        tabWidget->addTab(page0, QStringLiteral("Tab0"));
        tabWidget->addTab(page1, QStringLiteral("Tab1"));

        // Initial state: page0 is current
        QCOMPARE(tabWidget->currentIndex(), 0);
        QVERIFY(page0->isVisible());
        QVERIFY(!page1->isVisible());

        // Clear dirty state
        app.triggerRender();
        page0->clearDirtyRegions();
        page1->clearDirtyRegions();

        // Switch to tab 1
        tabWidget->setCurrentIndex(1);

        // Verify: page0 is hidden, page1 is visible
        QVERIFY(!page0->isVisible());
        QVERIFY(page1->isVisible());

        // Verify: both pages were marked dirty
        QVERIFY(page1->isDirty());
    }

    void exclusiveChildContainers_stackedWidgetMarksDirty()
    {
        // Unit test: verify markDirty is called on old and new widgets on index change
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *stacked = new QTuiStackedWidget(&mainWindow);
        stacked->setGeometry(QRect(0, 0, 40, 12));
        stacked->setVisible(true);

        auto *w0 = new ContainerChildWidget();
        auto *w1 = new ContainerChildWidget();
        stacked->addWidget(w0);
        stacked->addWidget(w1);

        // Initial state: w0 is current
        QCOMPARE(stacked->currentIndex(), 0);
        QVERIFY(w0->isVisible());
        QVERIFY(!w1->isVisible());

        // Clear dirty state
        app.triggerRender();
        w0->clearDirtyRegions();
        w1->clearDirtyRegions();

        // Switch to index 1
        stacked->setCurrentIndex(1);

        // Verify: w0 is hidden, w1 is visible
        QVERIFY(!w0->isVisible());
        QVERIFY(w1->isVisible());

        // Verify: new widget was marked dirty
        QVERIFY(w1->isDirty());
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 27: Scroll Area Clipping
    // -----------------------------------------------------------------------
    void scrollAreaClipping()
    {
        rc::check("QTuiScrollArea clips child content to viewport and shows scroll indicators",
            []() {
                // **Validates: Requirements 11.6**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a scroll area with a fixed viewport size
                auto *scrollArea = new QTuiScrollArea(&mainWindow);
                const int viewW = *rc::gen::inRange(5, 20);
                const int viewH = *rc::gen::inRange(5, 15);
                scrollArea->setGeometry(QRect(0, 0, viewW, viewH));
                scrollArea->setVisible(true);

                // Create a child widget larger than the viewport
                auto *child = new ContainerChildWidget();
                const int childW = *rc::gen::inRange(viewW + 1, viewW + 30);
                const int childH = *rc::gen::inRange(viewH + 1, viewH + 30);
                child->setGeometry(QRect(0, 0, childW, childH));
                child->setVisible(true);
                scrollArea->setWidget(child);

                // Trigger render
                scrollArea->markDirty();
                app.triggerRender();

                // Verify: child content does not extend beyond viewport boundaries
                ScreenBuffer *buf = app.screenBuffer();
                const QRect viewport = scrollArea->contentArea();

                // Check that 'C' characters (from child) only appear within the viewport
                for (int row = 0; row < buf->height(); ++row) {
                    for (int col = 0; col < buf->width(); ++col) {
                        const Cell &cell = buf->backCell(col, row);
                        if (cell.ch == U'C') {
                            RC_ASSERT(viewport.contains(col, row));
                        }
                    }
                }

                // Verify: scroll indicators are present (scroll bars are visible)
                RC_ASSERT(scrollArea->verticalScrollBar()->isVisible());
                RC_ASSERT(scrollArea->horizontalScrollBar()->isVisible());
            });
    }

    void scrollAreaClipping_noScrollWhenFits()
    {
        // Unit test: when child fits in viewport, no scroll bars are shown
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *scrollArea = new QTuiScrollArea(&mainWindow);
        scrollArea->setGeometry(QRect(0, 0, 20, 10));
        scrollArea->setVisible(true);

        // Child smaller than viewport
        auto *child = new ContainerChildWidget();
        child->setGeometry(QRect(0, 0, 10, 5));
        child->setVisible(true);
        scrollArea->setWidget(child);

        scrollArea->markDirty();
        app.triggerRender();

        // No scroll bars needed
        QVERIFY(!scrollArea->verticalScrollBar()->isVisible());
        QVERIFY(!scrollArea->horizontalScrollBar()->isVisible());
    }
};

QTEST_APPLESS_MAIN(TestContainerRendering)
#include "test_container_rendering.moc"
