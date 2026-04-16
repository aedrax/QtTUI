#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiGridLayout.h>
#include <QtTUI/QTuiFormLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/RenderScheduler.h>

using namespace QtTUI;

class TestLayoutInvalidation : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 6: Layout Apply Dirtying
    // -----------------------------------------------------------------------
    void layoutApplyDirtying()
    {
        rc::check("Layout apply() that changes child geometry marks affected widgets dirty",
            []() {
                // **Validates: Requirements 4.1, 4.3, 11.3**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a box layout with two child widgets
                auto *layout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, &mainWindow);
                QTuiLabel label1(QStringLiteral("Label1"), &mainWindow);
                QTuiLabel label2(QStringLiteral("Label2"), &mainWindow);
                label1.setGeometry(QRect(0, 0, 80, 12));
                label2.setGeometry(QRect(0, 12, 80, 12));
                layout->addWidget(&label1, 1);
                layout->addWidget(&label2, 1);

                // Drain events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                label1.clearDirtyRegions();
                label2.clearDirtyRegions();

                // Generate a random new available area that will cause geometry changes
                const int w = *rc::gen::inRange(20, 80);
                const int h = *rc::gen::inRange(4, 24);
                QRect newArea(0, 0, w, h);

                // Apply the layout with the new area — this should change child geometries
                layout->apply(newArea);

                // At least one widget should be dirty if its geometry changed
                bool label1Changed = (label1.geometry() != QRect(0, 0, 80, 12));
                bool label2Changed = (label2.geometry() != QRect(0, 12, 80, 12));

                if (label1Changed) {
                    RC_ASSERT(label1.isDirty());
                }
                if (label2Changed) {
                    RC_ASSERT(label2.isDirty());
                }

                // If any geometry changed, the scheduler should have a pending render
                if (label1Changed || label2Changed) {
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 6: Layout Apply Dirtying (Grid)
    // -----------------------------------------------------------------------
    void gridLayoutApplyDirtying()
    {
        rc::check("Grid layout apply() that changes child geometry marks affected widgets dirty",
            []() {
                // **Validates: Requirements 4.1, 4.3**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *layout = new QTuiGridLayout(&mainWindow);
                QTuiLabel label1(QStringLiteral("A"), &mainWindow);
                QTuiLabel label2(QStringLiteral("B"), &mainWindow);
                label1.setGeometry(QRect(0, 0, 40, 12));
                label2.setGeometry(QRect(40, 0, 40, 12));
                layout->addWidget(&label1, 0, 0);
                layout->addWidget(&label2, 0, 1);

                // Drain events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                label1.clearDirtyRegions();
                label2.clearDirtyRegions();

                // Apply with a different area to trigger geometry changes
                const int w = *rc::gen::inRange(10, 80);
                const int h = *rc::gen::inRange(2, 24);
                QRect newArea(0, 0, w, h);

                QRect oldGeo1 = label1.geometry();
                QRect oldGeo2 = label2.geometry();

                layout->apply(newArea);

                if (label1.geometry() != oldGeo1) {
                    RC_ASSERT(label1.isDirty());
                }
                if (label2.geometry() != oldGeo2) {
                    RC_ASSERT(label2.isDirty());
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 6: Layout Apply Dirtying (Form)
    // -----------------------------------------------------------------------
    void formLayoutApplyDirtying()
    {
        rc::check("Form layout apply() that changes child geometry marks affected widgets dirty",
            []() {
                // **Validates: Requirements 4.1, 4.3**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *layout = new QTuiFormLayout(&mainWindow);
                QTuiLabel field1(QStringLiteral("Value1"), &mainWindow);
                QTuiLabel field2(QStringLiteral("Value2"), &mainWindow);
                field1.setGeometry(QRect(10, 0, 70, 1));
                field2.setGeometry(QRect(10, 1, 70, 1));
                layout->addRow(&field1, &field2);

                // Drain events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                field1.clearDirtyRegions();
                field2.clearDirtyRegions();

                // Apply with a different area
                const int w = *rc::gen::inRange(10, 80);
                const int h = *rc::gen::inRange(2, 24);
                QRect newArea(0, 0, w, h);

                QRect oldGeo1 = field1.geometry();
                QRect oldGeo2 = field2.geometry();

                layout->apply(newArea);

                if (field1.geometry() != oldGeo1) {
                    RC_ASSERT(field1.isDirty());
                }
                if (field2.geometry() != oldGeo2) {
                    RC_ASSERT(field2.isDirty());
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 7: Layout Apply Idempotence
    // -----------------------------------------------------------------------
    void layoutApplyIdempotence()
    {
        rc::check("Calling apply() twice with the same area does not produce dirty regions on the second call",
            []() {
                // **Validates: Requirements 4.4**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *layout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, &mainWindow);
                QTuiLabel label1(QStringLiteral("Label1"), &mainWindow);
                QTuiLabel label2(QStringLiteral("Label2"), &mainWindow);
                layout->addWidget(&label1, 1);
                layout->addWidget(&label2, 1);

                // Generate a random available area
                const int w = *rc::gen::inRange(10, 80);
                const int h = *rc::gen::inRange(4, 24);
                QRect area(0, 0, w, h);

                // First apply — establishes geometries
                layout->apply(area);

                // Drain events and clear all dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                label1.clearDirtyRegions();
                label2.clearDirtyRegions();
                mainWindow.clearDirtyRegions();

                // Drain the scheduler's pending render
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Second apply with the SAME area — should NOT produce dirty regions
                layout->apply(area);

                // Widgets should NOT be dirty since geometries didn't change
                RC_ASSERT(!label1.isDirty());
                RC_ASSERT(!label2.isDirty());
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 7: Layout Apply Idempotence (Grid)
    // -----------------------------------------------------------------------
    void gridLayoutApplyIdempotence()
    {
        rc::check("Grid layout apply() twice with same area does not dirty on second call",
            []() {
                // **Validates: Requirements 4.4**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *layout = new QTuiGridLayout(&mainWindow);
                QTuiLabel label1(QStringLiteral("A"), &mainWindow);
                QTuiLabel label2(QStringLiteral("B"), &mainWindow);
                layout->addWidget(&label1, 0, 0);
                layout->addWidget(&label2, 0, 1);

                const int w = *rc::gen::inRange(10, 80);
                const int h = *rc::gen::inRange(2, 24);
                QRect area(0, 0, w, h);

                // First apply
                layout->apply(area);

                // Drain and clear
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                label1.clearDirtyRegions();
                label2.clearDirtyRegions();
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Second apply — same area
                layout->apply(area);

                RC_ASSERT(!label1.isDirty());
                RC_ASSERT(!label2.isDirty());
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 8: Layout Add/Remove Invalidation
    // -----------------------------------------------------------------------
    void layoutAddRemoveInvalidation()
    {
        rc::check("Adding or removing a widget from a layout marks the parent widget dirty",
            []() {
                // **Validates: Requirements 4.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a box layout parented to the main window
                auto *layout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, &mainWindow);
                QTuiLabel label1(QStringLiteral("Label1"), &mainWindow);
                layout->addWidget(&label1, 1);

                // Drain events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                mainWindow.clearDirtyRegions();
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Choose whether to test add or remove: 0 = add, 1 = remove
                const int action = *rc::gen::inRange(0, 2);

                if (action == 0) {
                    // --- Add a new widget ---
                    QTuiLabel label2(QStringLiteral("Label2"), &mainWindow);
                    layout->addWidget(&label2, 1);

                    // The parent widget (mainWindow) should be marked dirty
                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                } else {
                    // --- Remove an existing widget ---
                    layout->removeWidget(&label1);

                    // The parent widget (mainWindow) should be marked dirty
                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 8: Layout Add/Remove Invalidation (Grid)
    // -----------------------------------------------------------------------
    void gridLayoutAddRemoveInvalidation()
    {
        rc::check("Adding or removing a widget from a grid layout marks the parent widget dirty",
            []() {
                // **Validates: Requirements 4.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *layout = new QTuiGridLayout(&mainWindow);
                QTuiLabel label1(QStringLiteral("A"), &mainWindow);
                layout->addWidget(&label1, 0, 0);

                // Drain events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                mainWindow.clearDirtyRegions();
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                const int action = *rc::gen::inRange(0, 2);

                if (action == 0) {
                    // --- Add a new widget ---
                    QTuiLabel label2(QStringLiteral("B"), &mainWindow);
                    layout->addWidget(&label2, 0, 1);

                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                } else {
                    // --- Remove an existing widget ---
                    layout->removeWidget(&label1);

                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 8: Layout Add/Remove Invalidation (Form)
    // -----------------------------------------------------------------------
    void formLayoutAddRemoveInvalidation()
    {
        rc::check("Adding or removing a row from a form layout marks the parent widget dirty",
            []() {
                // **Validates: Requirements 4.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                auto *layout = new QTuiFormLayout(&mainWindow);
                QTuiLabel field1(QStringLiteral("Value1"), &mainWindow);
                layout->addRow(QStringLiteral("Label:"), &field1);

                // Drain events and clear dirty state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                mainWindow.clearDirtyRegions();
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                const int action = *rc::gen::inRange(0, 2);

                if (action == 0) {
                    // --- Add a new row ---
                    QTuiLabel field2(QStringLiteral("Value2"), &mainWindow);
                    layout->addRow(QStringLiteral("Label2:"), &field2);

                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                } else {
                    // --- Remove a row ---
                    layout->removeRow(0);

                    RC_ASSERT(mainWindow.isDirty());
                    RC_ASSERT(app.scheduler()->hasPendingRender());
                }
            });
    }
};

QTEST_APPLESS_MAIN(TestLayoutInvalidation)
#include "test_layout_invalidation.moc"
