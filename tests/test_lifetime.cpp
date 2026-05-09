// Lifetime regression tests: widgets/models deleted while still referenced
// by layouts, views, main window caches, or theme overrides must not leave
// dangling pointers behind (no use-after-free, entries dropped or nulled).
#include <QtTest/QtTest>
#include <QStringListModel>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiFormLayout.h>
#include <QtTUI/QTuiGridLayout.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiStackedLayout.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// Concrete widget with configurable minimum size for testing
class LifetimeTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit LifetimeTestWidget(QSize minSize = QSize(5, 1), QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return m_minSize; }
private:
    QSize m_minSize;
};

// Minimal concrete QTuiLayout to exercise the base-class item list
class PassthroughLayout : public QTuiLayout {
public:
    explicit PassthroughLayout(QObject *parent = nullptr) : QTuiLayout(parent) {}
    void apply(const QRect &availableArea) override
    {
        for (int i = 0; i < count(); ++i)
            itemAt(i)->setGeometry(availableArea);
    }
};

class TestLifetime : public QObject {
    Q_OBJECT

private slots:

    // Deleting a widget still in a box layout must remove its entry;
    // a subsequent apply() must not touch the freed widget.
    void boxLayoutDropsDestroyedWidget()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        auto *w1 = new LifetimeTestWidget(QSize(5, 2));
        auto *w2 = new LifetimeTestWidget(QSize(5, 2));
        layout.addWidget(w1, 1);
        layout.addWidget(w2, 1);
        QCOMPARE(layout.count(), 2);

        delete w1;
        QCOMPARE(layout.count(), 1);

        layout.apply(QRect(0, 0, 40, 10)); // must not crash
        QCOMPARE(w2->geometry(), QRect(0, 0, 40, 10));

        delete w2;
        QCOMPARE(layout.count(), 0);
        layout.apply(QRect(0, 0, 40, 10)); // empty layout, no crash
    }

    // removeWidget must detach the destroyed() bookkeeping: deleting the
    // widget afterwards must not disturb the layout.
    void boxLayoutRemoveThenDelete()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);
        auto *w1 = new LifetimeTestWidget();
        auto *w2 = new LifetimeTestWidget();
        layout.addWidget(w1, 1);
        layout.addWidget(w2, 1);

        layout.removeWidget(w1);
        QCOMPARE(layout.count(), 1);
        delete w1;
        QCOMPARE(layout.count(), 1);

        layout.apply(QRect(0, 0, 20, 5));
        QCOMPARE(w2->geometry(), QRect(0, 0, 20, 5));
        delete w2;
    }

    void gridLayoutDropsDestroyedWidget()
    {
        QTuiGridLayout layout;
        auto *w1 = new LifetimeTestWidget(QSize(4, 1));
        auto *w2 = new LifetimeTestWidget(QSize(4, 1));
        layout.addWidget(w1, 0, 0);
        layout.addWidget(w2, 1, 0);

        delete w1;

        layout.apply(QRect(0, 0, 20, 10)); // must not crash
        QVERIFY(!w2->geometry().isEmpty());
        delete w2;
    }

    void formLayoutDropsRowOnFieldDestroyed()
    {
        QTuiFormLayout layout;
        auto *f1 = new LifetimeTestWidget();
        auto *f2 = new LifetimeTestWidget();
        layout.addRow(QStringLiteral("A:"), f1);
        layout.addRow(QStringLiteral("B:"), f2);
        QCOMPARE(layout.rowCount(), 2);

        delete f1;
        QCOMPARE(layout.rowCount(), 1);

        layout.apply(QRect(0, 0, 40, 10)); // must not crash
        QVERIFY(!f2->geometry().isEmpty());
        delete f2;
        QCOMPARE(layout.rowCount(), 0);
    }

    // Owned labels are now parented into the widget tree (so they render), so
    // the parent widget can destroy them before the layout does. Neither
    // order may double-delete or leave a dangling pointer.
    void formLayoutSurvivesParentDeletingOwnedLabel()
    {
        auto *container = new LifetimeTestWidget;
        auto *field = new LifetimeTestWidget(QSize(5, 1), container);

        QTuiFormLayout layout;
        layout.addRow(QStringLiteral("L:"), field);
        QCOMPARE(layout.rowCount(), 1);

        // Deletes field AND the owned label (both children of container).
        // The field watcher drops the row; the label watcher nulls the label.
        delete container;
        QCOMPARE(layout.rowCount(), 0);

        layout.apply(QRect(0, 0, 40, 10)); // must not crash
        // ~QTuiFormLayout must not double-delete the already-dead label
    }

    void formLayoutNullsDestroyedCustomLabel()
    {
        QTuiFormLayout layout;
        auto *label = new LifetimeTestWidget(QSize(8, 1));
        auto *field = new LifetimeTestWidget();
        layout.addRow(label, field);

        delete label;
        QCOMPARE(layout.rowCount(), 1); // row stays, label is gone

        layout.apply(QRect(0, 0, 40, 10)); // must not crash
        QVERIFY(!field->geometry().isEmpty());
        delete field;
    }

    void baseLayoutDropsDestroyedWidget()
    {
        PassthroughLayout layout;
        auto *w = new LifetimeTestWidget();
        layout.addWidget(w);
        QCOMPARE(layout.count(), 1);

        delete w;
        QCOMPARE(layout.count(), 0);
        layout.apply(QRect(0, 0, 10, 10)); // must not crash
    }

    void stackedLayoutDropsDestroyedWidget()
    {
        QTuiStackedLayout layout;
        auto *w1 = new LifetimeTestWidget();
        auto *w2 = new LifetimeTestWidget();
        layout.addWidget(w1);
        layout.addWidget(w2);
        QCOMPARE(layout.currentWidget(), static_cast<QTuiWidget *>(w1));

        delete w1;
        QCOMPARE(layout.currentWidget(), static_cast<QTuiWidget *>(w2));

        layout.apply(QRect(0, 0, 20, 10)); // must not crash
        QCOMPARE(w2->geometry(), QRect(0, 0, 20, 10));
        delete w2;
        QVERIFY(layout.currentWidget() == nullptr);
    }

    // Deleting the model set on a table view, then repainting through the
    // real pipeline, must not crash; the view forgets the model.
    void tableViewSurvivesModelDeletion()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);

        auto *view = new QTuiTableView(&mainWindow);
        view->setGeometry(QRect(0, 0, 40, 10));
        view->setVisible(true);

        auto *model = new QStringListModel(
            QStringList() << QStringLiteral("one")
                          << QStringLiteral("two")
                          << QStringLiteral("three"));
        view->setModel(model);
        view->setCurrentIndex(model->index(2, 0));

        app.triggerRender();

        delete model;

        QVERIFY(view->model() == nullptr);
        QVERIFY(!view->currentIndex().isValid());

        view->markDirty();
        app.triggerRender(); // must not crash; view paints nothing
    }

    // Destroying a widget with a theme override must drop the map entry so a
    // new widget reusing the address does not inherit the dead widget's look.
    void themeOverrideRemovedOnWidgetDestruction()
    {
        auto *engine = Theme_Engine::instance();
        const Color overrideColor(1, 2, 3);

        auto *w1 = new LifetimeTestWidget();
        ThemeProperties props;
        props.foreground = overrideColor;
        engine->setWidgetOverride(w1, props);
        QVERIFY(engine->effectiveProperties(w1).foreground == overrideColor);

        void *oldAddress = w1;
        delete w1;

        auto *w2 = new LifetimeTestWidget();
        if (static_cast<void *>(w2) != oldAddress) {
            delete w2;
            QSKIP("Allocator did not reuse the address; cannot observe stale override");
        }

        QVERIFY(!(engine->effectiveProperties(w2).foreground == overrideColor));
        delete w2;
    }

    // Main window caches (central widget, menu bar, ...) must be cleared
    // when the cached widget is destroyed externally.
    void mainWindowClearsDestroyedCentralWidget()
    {
        QTuiMainWindow window;
        window.setGeometry(QRect(0, 0, 80, 24));

        auto *central = new LifetimeTestWidget(QSize(5, 1), &window);
        window.setCentralWidget(central);
        QCOMPARE(window.centralWidget(), static_cast<QTuiWidget *>(central));

        delete central;
        QVERIFY(window.centralWidget() == nullptr);

        // Relayout path must not touch the stale pointer
        window.setGeometry(QRect(0, 0, 60, 20));
        window.show();
    }
};

QTEST_APPLESS_MAIN(TestLifetime)
#include "test_lifetime.moc"
