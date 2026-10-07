#include <QtTest/QtTest>
#include <QStringListModel>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiToolButton.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Each action that the mouse can do also has a key

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestKeyboardAccess : public QObject {
    Q_OBJECT
private slots:
    void altArrowsResizeAndSortTableColumns()
    {
        QStringListModel model({QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("a")});
        auto first = [&model]() { return model.index(0).data().toString(); };
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 40, 6));
        table.setModel(&model);

        table.setColumnWidth(0, 10);
        const int width = table.columnWidth(0);
        sendKey(table, Qt::Key_Right, Qt::AltModifier);
        QCOMPARE(table.columnWidth(0), width + 1);
        sendKey(table, Qt::Key_Left, Qt::AltModifier);
        sendKey(table, Qt::Key_Left, Qt::AltModifier);
        QCOMPARE(table.columnWidth(0), width - 1);
        QCOMPARE(table.currentIndex().column(), 0); // Alt+arrows do not move

        // Without sorting enabled, Alt+Up does nothing
        sendKey(table, Qt::Key_Up, Qt::AltModifier);
        QCOMPARE(first(), QStringLiteral("b"));
        table.setSortingEnabled(true);
        sendKey(table, Qt::Key_Up, Qt::AltModifier);
        QCOMPARE(first(), QStringLiteral("a"));
        sendKey(table, Qt::Key_Down, Qt::AltModifier);
        QCOMPARE(first(), QStringLiteral("c"));
    }

    void mdiKeysMaximizeAndFocusActivates()
    {
        QTuiMdiArea area;
        area.setGeometry(QRect(0, 0, 60, 20));
        QTuiMdiSubWindow *first = area.addSubWindow(new QTuiWidget);
        auto *button = new QTuiPushButton(QStringLiteral("OK"));
        QTuiMdiSubWindow *second = area.addSubWindow(button);
        area.setActiveSubWindow(first);

        // The focus on a widget in a window makes the window active
        button->setFocus();
        QCOMPARE(area.activeSubWindow(), second);
        QVERIFY(button->hasFocus());

        first->setFocus();
        QCOMPARE(area.activeSubWindow(), first);
        sendKey(*first, Qt::Key_M);
        QVERIFY(first->isMaximized());
        sendKey(*first, Qt::Key_M);
        QVERIFY(!first->isMaximized());
    }

    void clickInASubWindowPutsItOnTop()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 60, 20));
        app.setMainWindow(&mainWindow);
        auto *area = new QTuiMdiArea;
        mainWindow.setCentralWidget(area);
        mainWindow.show();
        // The label takes no focus, but a click on it activates its window
        QTuiMdiSubWindow *back = area->addSubWindow(new QTuiLabel(QStringLiteral("Back")));
        QTuiMdiSubWindow *front = area->addSubWindow(new QTuiWidget);
        back->setGeometry(QRect(0, 0, 20, 8));
        front->setGeometry(QRect(10, 0, 20, 8));
        back->widget()->setGeometry(QRect(1, 1, 18, 6));
        QCOMPARE(area->childWidgets().constLast(), front);

        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(2, 2));
        QCoreApplication::sendEvent(&mainWindow, &press);
        QCOMPARE(area->activeSubWindow(), back);
        QCOMPARE(area->childWidgets().constLast(), back);
    }

    void scrollAreaScrollsToTheFocusWidget()
    {
        QTuiScrollArea scrollArea;
        scrollArea.setGeometry(QRect(0, 0, 12, 4));
        auto *content = new QTuiWidget;
        content->setGeometry(QRect(0, 0, 10, 20));
        auto *button = new QTuiPushButton(QStringLiteral("Far"), content);
        button->setGeometry(QRect(0, 15, 8, 3));
        scrollArea.setWidget(content);
        QCOMPARE(scrollArea.verticalScrollBar()->value(), 0);

        // The bottom row of the button (17) is the last visible row
        button->setFocus();
        const int offset = scrollArea.verticalScrollBar()->value();
        QVERIFY(offset <= 15);
        QVERIFY(offset + scrollArea.geometry().height() - 1 >= 17);
    }

    void f10OpensTheFirstMenu()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 10));
        app.setMainWindow(&mainWindow);
        auto *menuBar = new QTuiMenuBar;
        QTuiMenu *file = menuBar->addMenu(QStringLiteral("File"));
        file->addAction(QStringLiteral("Quit"));
        mainWindow.setMenuBar(menuBar);
        mainWindow.setCentralWidget(new QTuiWidget);
        mainWindow.show();

        QVERIFY(!file->isVisible());
        sendKey(mainWindow, Qt::Key_F10);
        QVERIFY(file->isVisible());
        file->close();
    }

    void toolButtonMenuOpensWithDownAndDockClosesOnClick()
    {
        static int argc = 1;
        static char arg0[] = "test";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);
        QTuiMainWindow mainWindow;
        mainWindow.setGeometry(QRect(0, 0, 40, 10));
        app.setMainWindow(&mainWindow);
        auto *central = new QTuiWidget;
        mainWindow.setCentralWidget(central);
        auto *tool = new QTuiToolButton(QStringLiteral("Tool"), central);
        tool->setGeometry(QRect(0, 0, 10, 1));
        auto *menu = new QTuiMenu(QString(), central);
        menu->addAction(QStringLiteral("One"));
        tool->setMenu(menu);
        mainWindow.show();

        sendKey(*tool, Qt::Key_Down);
        QVERIFY(menu->isVisible());
        menu->close();

        // " Tool ▼": the "▼" is at column 6
        TuiMouseEvent click(TuiMouseEvent::Press, Qt::LeftButton, QPoint(6, 0));
        QCoreApplication::sendEvent(tool, &click);
        QVERIFY(menu->isVisible());
        menu->close();

        QTuiDockWidget dock(QStringLiteral("Dock"));
        dock.setGeometry(QRect(0, 2, 20, 5));
        dock.setVisible(true);
        // "[X]" is in the 3 columns before the top-right corner (19)
        TuiMouseEvent close(TuiMouseEvent::Press, Qt::LeftButton, QPoint(17, 2));
        QCoreApplication::sendEvent(&dock, &close);
        QVERIFY(!dock.isVisible());
    }
};

QTEST_MAIN(TestKeyboardAccess)
#include "test_keyboard_access.moc"
