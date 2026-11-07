// QTuiWidget::close() and closeEvent(): a close hides the widget unless the
// handler ignores the event. Dialogs, MDI sub-windows and dock widgets close
// through it, and a close of the main window quits the application.
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTimer>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Counts the close events, and ignores them while keepOpen is true
template <typename Base>
class Closable : public Base {
public:
    using Base::Base;
    bool keepOpen = false;
    int closeEvents = 0;

protected:
    void closeEvent(TuiCloseEvent *event) override
    {
        ++closeEvents;
        if (keepOpen)
            event->ignore();
        else
            Base::closeEvent(event);
    }
};

// Direct delivery: most tests here have no application object
static void sendKey(QTuiWidget &w, int key)
{
    TuiKeyEvent ev(key, Qt::NoModifier);
    static_cast<QObject &>(w).event(&ev);
}

class TestCloseEvent : public QObject {
    Q_OBJECT
private slots:
    void closeHidesTheWidget()
    {
        Closable<QTuiWidget> w;
        w.setGeometry(QRect(0, 0, 10, 3));
        w.clearDirtyRegions();
        QVERIFY(w.close());
        QCOMPARE(w.closeEvents, 1);
        QVERIFY(!w.isVisible());
        QVERIFY(w.isDirty());
    }

    void ignoredCloseKeepsTheWidgetOpen()
    {
        Closable<QTuiWidget> w;
        w.keepOpen = true;
        QVERIFY(!w.close());
        QCOMPARE(w.closeEvents, 1);
        QVERIFY(w.isVisible());
    }

    void dockClosesThroughCloseEvent()
    {
        Closable<QTuiDockWidget> dock(QStringLiteral("Panel"));
        dock.setGeometry(QRect(0, 0, 20, 5));
        QSignalSpy spy(&dock, &QTuiDockWidget::visibilityChanged);

        // Escape and a click on [X] ask closeEvent() first
        dock.keepOpen = true;
        sendKey(dock, Qt::Key_Escape);
        TuiMouseEvent click(TuiMouseEvent::Press, Qt::LeftButton, QPoint(17, 0));
        static_cast<QObject &>(dock).event(&click);
        QCOMPARE(dock.closeEvents, 2);
        QVERIFY(dock.isVisible());
        QCOMPARE(spy.count(), 0);

        dock.keepOpen = false;
        static_cast<QObject &>(dock).event(&click);
        QVERIFY(!dock.isVisible());
        QCOMPARE(spy.count(), 1);
    }

    void mdiSubWindowClosesThroughCloseEvent()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 60, 20));
        QTuiMdiSubWindow *first = area.addSubWindow(new QTuiWidget);
        QTuiMdiSubWindow *second = area.addSubWindow(new QTuiWidget);
        QCOMPARE(area.subWindowList().size(), 2);

        // close() is now the function of QTuiWidget
        QTuiWidget *asWidget = first;
        QVERIFY(asWidget->close());
        QVERIFY(!first->isVisible());
        QCOMPARE(area.subWindowList(), QList<QTuiMdiSubWindow *>{second});

        sendKey(*second, Qt::Key_Escape);
        QVERIFY(!second->isVisible());
        QVERIFY(area.subWindowList().isEmpty());

        // A window that ignores the close stays in the area
        Closable<QTuiMdiSubWindow> stay(&area);
        stay.setGeometry(QRect(0, 0, 20, 5));
        stay.keepOpen = true;
        sendKey(stay, Qt::Key_Q);
        QCOMPARE(stay.closeEvents, 1);
        QVERIFY(stay.isVisible());
    }

    void dialogAndMainWindow()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        Closable<QTuiMainWindow> mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 10));
        app.setMainWindow(&mainWindow);
        mainWindow.setCentralWidget(new QTuiWidget);
        mainWindow.show();
        QCoreApplication::processEvents();

        // Escape closes a dialog through closeEvent(), which rejects it
        Closable<QTuiDialog> dialog;
        QSignalSpy rejected(&dialog, &QTuiDialog::rejected);
        dialog.open();
        QVERIFY(dialog.isVisible());
        dialog.keepOpen = true;
        sendKey(mainWindow, Qt::Key_Escape);
        QCOMPARE(dialog.closeEvents, 1);
        QVERIFY(dialog.isVisible());
        QCOMPARE(rejected.count(), 0);

        dialog.keepOpen = false;
        sendKey(mainWindow, Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(dialog.result(), int(QTuiDialog::Rejected));

        // reject() and accept() do not ask closeEvent()
        dialog.open();
        dialog.accept();
        QCOMPARE(dialog.closeEvents, 2);
        QVERIFY(!dialog.isVisible());

        // An ignored close of the main window does not quit
        QSignalSpy aboutToQuit(&app, &QCoreApplication::aboutToQuit);
        mainWindow.keepOpen = true;
        QVERIFY(!mainWindow.close());
        QVERIFY(mainWindow.isVisible());

        // An accepted close quits the application
        mainWindow.keepOpen = false;
        bool timedOut = false;
        QTimer::singleShot(0, &mainWindow, [&]() { QVERIFY(mainWindow.close()); });
        QTimer guard;
        guard.setSingleShot(true);
        connect(&guard, &QTimer::timeout, &app, [&]() {
            timedOut = true;
            app.quit();
        });
        guard.start(5000);
        app.exec();
        QVERIFY(!timedOut);
        QCOMPARE(aboutToQuit.count(), 1);
        QCOMPARE(mainWindow.closeEvents, 2);
    }
};

QTEST_APPLESS_MAIN(TestCloseEvent)
#include "test_close_event.moc"
