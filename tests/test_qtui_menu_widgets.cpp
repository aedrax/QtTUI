#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiMenuWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiMenu tests
    // ---------------------------------------------------------------

    void menu_defaultConstruction()
    {
        QTuiMenu menu;
        QCOMPARE(menu.title(), QString());
        QCOMPARE(menu.actions().size(), 0);
        QCOMPARE(menu.isVisible(), false);
    }

    void menu_constructionWithTitle()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QCOMPARE(menu.title(), QStringLiteral("File"));
    }

    void menu_setTitle()
    {
        QTuiMenu menu;
        menu.setTitle(QStringLiteral("Edit"));
        QCOMPARE(menu.title(), QStringLiteral("Edit"));
    }

    void menu_addAction()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QTuiAction *act = menu.addAction(QStringLiteral("Open"));
        QVERIFY(act != nullptr);
        QCOMPARE(act->text(), QStringLiteral("Open"));
        QCOMPARE(menu.actions().size(), 1);
        QCOMPARE(menu.actions().at(0), act);
    }

    void menu_addActionWithShortcut()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QTuiAction *act = menu.addAction(QStringLiteral("Save"), QStringLiteral("Ctrl+S"));
        QVERIFY(act != nullptr);
        QCOMPARE(act->text(), QStringLiteral("Save"));
        QCOMPARE(act->shortcut(), QStringLiteral("Ctrl+S"));
    }

    void menu_addMultipleActions()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addAction(QStringLiteral("Open"));
        menu.addAction(QStringLiteral("Save"));
        QCOMPARE(menu.actions().size(), 3);
    }

    void menu_addSeparator()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addSeparator();
        menu.addAction(QStringLiteral("Quit"));
        // Separator doesn't have an action, so actions() returns 2
        QCOMPARE(menu.actions().size(), 2);
    }

    void menu_addSubmenu()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QTuiMenu *sub = menu.addMenu(QStringLiteral("Recent"));
        QVERIFY(sub != nullptr);
        QCOMPARE(sub->title(), QStringLiteral("Recent"));
        // The submenu entry adds an action for the title
        QCOMPARE(menu.actions().size(), 1);
    }

    void menu_addExistingSubmenu()
    {
        QTuiMenu menu(QStringLiteral("File"));
        auto *sub = new QTuiMenu(QStringLiteral("Export"), &menu);
        menu.addMenu(sub);
        QCOMPARE(menu.actions().size(), 1);
    }

    void menu_popupAndClose()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("Open"));
        menu.addAction(QStringLiteral("Save"));

        QSignalSpy showSpy(&menu, &QTuiMenu::aboutToShow);
        QSignalSpy hideSpy(&menu, &QTuiMenu::aboutToHide);

        QCOMPARE(menu.isVisible(), false);

        menu.popup(QPoint(5, 3));
        QCOMPARE(menu.isVisible(), true);
        QCOMPARE(showSpy.count(), 1);

        menu.close();
        QCOMPARE(menu.isVisible(), false);
        QCOMPARE(hideSpy.count(), 1);
    }

    void menu_closeWhenNotVisible()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QSignalSpy hideSpy(&menu, &QTuiMenu::aboutToHide);
        menu.close();
        QCOMPARE(hideSpy.count(), 0);
    }

    void menu_keyNavigationUpDown()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addAction(QStringLiteral("Open"));
        menu.addAction(QStringLiteral("Save"));
        menu.popup(QPoint(0, 0));

        sendKey(menu, Qt::Key_Down);
        sendKey(menu, Qt::Key_Down);
        sendKey(menu, Qt::Key_Up);

        // Menu should still be visible (no trigger)
        QCOMPARE(menu.isVisible(), true);
    }

    void menu_keyEnterTriggersAction()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QTuiAction *act1 = menu.addAction(QStringLiteral("New"));
        QTuiAction *act2 = menu.addAction(QStringLiteral("Open"));

        QSignalSpy spy1(act1, &QTuiAction::triggered);
        QSignalSpy spy2(act2, &QTuiAction::triggered);

        menu.popup(QPoint(0, 0));

        // Navigate to second item and trigger
        sendKey(menu, Qt::Key_Down);
        sendKey(menu, Qt::Key_Return);

        QCOMPARE(spy1.count(), 0);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(menu.isVisible(), false);
    }

    void menu_keyEscapeCloses()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.popup(QPoint(0, 0));
        QCOMPARE(menu.isVisible(), true);

        sendKey(menu, Qt::Key_Escape);
        QCOMPARE(menu.isVisible(), false);
    }

    void menu_skipsSeparatorsOnNavigation()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addSeparator();
        QTuiAction *act2 = menu.addAction(QStringLiteral("Quit"));

        QSignalSpy spy2(act2, &QTuiAction::triggered);

        menu.popup(QPoint(0, 0));

        // Down should skip separator and land on "Quit"
        sendKey(menu, Qt::Key_Down);
        sendKey(menu, Qt::Key_Return);

        QCOMPARE(spy2.count(), 1);
    }

    void menu_submenuOpensOnEnter()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QTuiMenu *sub = menu.addMenu(QStringLiteral("Recent"));
        sub->addAction(QStringLiteral("file1.txt"));

        menu.popup(QPoint(0, 0));

        sendKey(menu, Qt::Key_Return);
        QCOMPARE(sub->isVisible(), true);
        QCOMPARE(menu.isVisible(), true);
    }

    void menu_submenuOpensOnRight()
    {
        QTuiMenu menu(QStringLiteral("File"));
        QTuiMenu *sub = menu.addMenu(QStringLiteral("Recent"));
        sub->addAction(QStringLiteral("file1.txt"));

        menu.popup(QPoint(0, 0));

        sendKey(menu, Qt::Key_Right);
        QCOMPARE(sub->isVisible(), true);
    }

    void menu_minimumSize()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addAction(QStringLiteral("Open File"), QStringLiteral("Ctrl+O"));
        menu.addSeparator();
        menu.addAction(QStringLiteral("Quit"));

        QSize ms = menu.minimumSize();
        QVERIFY(ms.width() >= 10);
        // Height: 2 borders + 4 items (including separator)
        QCOMPARE(ms.height(), 6);
    }

    void menu_paintEvent()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addAction(QStringLiteral("Open"));
        menu.popup(QPoint(0, 0));

        ScreenBuffer buffer(20, 6);
        menu.paintEvent(buffer);

        QString row1;
        for (int col = 0; col < 20; ++col)
            row1 += buffer.backCell(col, 1).text();
        QVERIFY(row1.contains(QStringLiteral("New")));

        QString row2;
        for (int col = 0; col < 20; ++col)
            row2 += buffer.backCell(col, 2).text();
        QVERIFY(row2.contains(QStringLiteral("Open")));
    }

    void menu_highlightedItemRenderedDifferently()
    {
        QTuiMenu menu(QStringLiteral("File"));
        menu.addAction(QStringLiteral("New"));
        menu.addAction(QStringLiteral("Open"));
        menu.popup(QPoint(0, 0));

        ScreenBuffer buffer(20, 6);
        menu.paintEvent(buffer);

        // First item (highlighted) should have bold
        QCOMPARE(buffer.backCell(1, 1).bold, true);
        // Second item should not be bold
        QCOMPARE(buffer.backCell(1, 2).bold, false);
    }

    // ---------------------------------------------------------------
    // QTuiMenuBar tests
    // ---------------------------------------------------------------

    void menuBar_defaultConstruction()
    {
        QTuiMenuBar bar;
        QCOMPARE(bar.menus().size(), 0);
    }

    void menuBar_addMenuByTitle()
    {
        QTuiMenuBar bar;
        QTuiMenu *menu = bar.addMenu(QStringLiteral("File"));
        QVERIFY(menu != nullptr);
        QCOMPARE(menu->title(), QStringLiteral("File"));
        QCOMPARE(bar.menus().size(), 1);
    }

    void menuBar_addExistingMenu()
    {
        QTuiMenuBar bar;
        auto *menu = new QTuiMenu(QStringLiteral("Edit"), &bar);
        bar.addMenu(menu);
        QCOMPARE(bar.menus().size(), 1);
        QCOMPARE(bar.menus().at(0), menu);
    }

    void menuBar_addMultipleMenus()
    {
        QTuiMenuBar bar;
        bar.addMenu(QStringLiteral("File"));
        bar.addMenu(QStringLiteral("Edit"));
        bar.addMenu(QStringLiteral("View"));
        QCOMPARE(bar.menus().size(), 3);
    }

    void menuBar_keyLeftRight()
    {
        QTuiMenuBar bar;
        bar.addMenu(QStringLiteral("File"));
        bar.addMenu(QStringLiteral("Edit"));
        bar.addMenu(QStringLiteral("View"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        sendKey(bar, Qt::Key_Right);
        sendKey(bar, Qt::Key_Right);
        sendKey(bar, Qt::Key_Left);

        // No menus opened by just navigating
        QCOMPARE(bar.menus().at(0)->isVisible(), false);
        QCOMPARE(bar.menus().at(1)->isVisible(), false);
        QCOMPARE(bar.menus().at(2)->isVisible(), false);
    }

    void menuBar_enterOpensMenu()
    {
        QTuiMenuBar bar;
        QTuiMenu *fileMenu = bar.addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        sendKey(bar, Qt::Key_Return);
        QCOMPARE(fileMenu->isVisible(), true);
    }

    void menuBar_downOpensMenu()
    {
        QTuiMenuBar bar;
        QTuiMenu *fileMenu = bar.addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        sendKey(bar, Qt::Key_Down);
        QCOMPARE(fileMenu->isVisible(), true);
    }

    void menuBar_escapeClosesMenu()
    {
        QTuiMenuBar bar;
        QTuiMenu *fileMenu = bar.addMenu(QStringLiteral("File"));
        fileMenu->addAction(QStringLiteral("New"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        sendKey(bar, Qt::Key_Return);
        QCOMPARE(fileMenu->isVisible(), true);

        sendKey(bar, Qt::Key_Escape);
        QCOMPARE(fileMenu->isVisible(), false);
    }

    void menuBar_lateralNavigationWithOpenMenu()
    {
        QTuiMenuBar bar;
        QTuiMenu *fileMenu = bar.addMenu(QStringLiteral("File"));
        QTuiMenu *editMenu = bar.addMenu(QStringLiteral("Edit"));
        fileMenu->addAction(QStringLiteral("New"));
        editMenu->addAction(QStringLiteral("Undo"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        // Open File menu
        sendKey(bar, Qt::Key_Return);
        QCOMPARE(fileMenu->isVisible(), true);
        QCOMPARE(editMenu->isVisible(), false);

        // Navigate right should close File and open Edit
        sendKey(bar, Qt::Key_Right);
        QCOMPARE(fileMenu->isVisible(), false);
        QCOMPARE(editMenu->isVisible(), true);

        // Navigate left should close Edit and open File
        sendKey(bar, Qt::Key_Left);
        QCOMPARE(fileMenu->isVisible(), true);
        QCOMPARE(editMenu->isVisible(), false);
    }

    void menuBar_lateralNavigationWraps()
    {
        QTuiMenuBar bar;
        QTuiMenu *fileMenu = bar.addMenu(QStringLiteral("File"));
        QTuiMenu *editMenu = bar.addMenu(QStringLiteral("Edit"));
        fileMenu->addAction(QStringLiteral("New"));
        editMenu->addAction(QStringLiteral("Undo"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        // Navigate left from first item should wrap to last
        sendKey(bar, Qt::Key_Left);
        sendKey(bar, Qt::Key_Return);
        QCOMPARE(editMenu->isVisible(), true);
    }

    void menuBar_minimumSize()
    {
        QTuiMenuBar bar;
        bar.addMenu(QStringLiteral("File"));
        bar.addMenu(QStringLiteral("Edit"));
        QSize ms = bar.minimumSize();
        // "File" = 4 + 2 padding = 6, "Edit" = 4 + 2 padding = 6, total = 12
        QCOMPARE(ms.width(), 12);
        QCOMPARE(ms.height(), 1);
    }

    void menuBar_paintEvent()
    {
        QTuiMenuBar bar;
        bar.addMenu(QStringLiteral("File"));
        bar.addMenu(QStringLiteral("Edit"));
        bar.setGeometry(QRect(0, 0, 20, 1));

        ScreenBuffer buffer(20, 1);
        bar.paintEvent(buffer);

        QString row;
        for (int col = 0; col < 20; ++col)
            row += buffer.backCell(col, 0).text();

        QVERIFY(row.contains(QStringLiteral("File")));
        QVERIFY(row.contains(QStringLiteral("Edit")));
    }

    void menuBar_activeMenuHighlighted()
    {
        QTuiMenuBar bar;
        bar.addMenu(QStringLiteral("File"));
        bar.addMenu(QStringLiteral("Edit"));
        bar.setGeometry(QRect(0, 0, 20, 1));

        ScreenBuffer buffer(20, 1);
        bar.paintEvent(buffer);

        // First menu title (active) should be bold at position 1 (after leading space)
        QCOMPARE(buffer.backCell(1, 0).bold, true);
    }

    void menuBar_mnemonicActivation()
    {
        QTuiMenuBar bar;
        QTuiMenu *fileMenu = bar.addMenu(QStringLiteral("&File"));
        QTuiMenu *editMenu = bar.addMenu(QStringLiteral("&Edit"));
        fileMenu->addAction(QStringLiteral("New"));
        editMenu->addAction(QStringLiteral("Undo"));
        bar.setGeometry(QRect(0, 0, 30, 1));

        // Alt+E should open Edit menu
        sendKey(bar, Qt::Key_E, Qt::AltModifier, QStringLiteral("e"));
        QCOMPARE(editMenu->isVisible(), true);
        QCOMPARE(fileMenu->isVisible(), false);
    }
};

QTEST_MAIN(TestQTuiMenuWidgets)
#include "test_qtui_menu_widgets.moc"
