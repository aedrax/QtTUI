#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiTabBar.h>
#include <QtTUI/QTuiTabWidget.h>
#include <QtTUI/QTuiToolBox.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// Simple test child widget
class TabTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit TabTestWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        for (int row = 0; row < geo.height(); ++row) {
            for (int col = 0; col < geo.width(); ++col) {
                int px = geo.x() + col;
                int py = geo.y() + row;
                if (px < buffer.width() && py < buffer.height()) {
                    buffer.backCell(px, py).character = QLatin1Char('X');
                }
            }
        }
    }
    QSize minimumSize() const override { return QSize(3, 1); }
};

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiTabWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiTabBar tests
    // ---------------------------------------------------------------

    void tabBar_defaultConstruction()
    {
        QTuiTabBar bar;
        QCOMPARE(bar.count(), 0);
        QCOMPARE(bar.currentIndex(), -1);
        QCOMPARE(bar.tabPosition(), QTuiTabBar::North);
    }

    void tabBar_addTab()
    {
        QTuiTabBar bar;
        int idx = bar.addTab(QStringLiteral("First"));
        QCOMPARE(idx, 0);
        QCOMPARE(bar.count(), 1);
        QCOMPARE(bar.currentIndex(), 0);
        QCOMPARE(bar.tabText(0), QStringLiteral("First"));
    }

    void tabBar_addMultipleTabs()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));
        bar.addTab(QStringLiteral("C"));
        QCOMPARE(bar.count(), 3);
        // Only first tab becomes current
        QCOMPARE(bar.currentIndex(), 0);
    }

    void tabBar_removeTab()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));
        bar.addTab(QStringLiteral("C"));
        bar.setCurrentIndex(2);

        bar.removeTab(0);
        QCOMPARE(bar.count(), 2);
        // currentIndex was 2, removing index 0 shifts it to 1
        QCOMPARE(bar.currentIndex(), 1);
        QCOMPARE(bar.tabText(0), QStringLiteral("B"));
        QCOMPARE(bar.tabText(1), QStringLiteral("C"));
    }

    void tabBar_removeCurrentTab()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));
        bar.addTab(QStringLiteral("C"));
        bar.setCurrentIndex(1);

        bar.removeTab(1);
        QCOMPARE(bar.count(), 2);
        // After removing current tab at index 1, index stays at 1 (now "C")
        QCOMPARE(bar.currentIndex(), 1);
    }

    void tabBar_removeAllTabs()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));

        bar.removeTab(1);
        bar.removeTab(0);
        QCOMPARE(bar.count(), 0);
        QCOMPARE(bar.currentIndex(), -1);
    }

    void tabBar_tabText()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("Hello"));
        bar.addTab(QStringLiteral("World"));
        QCOMPARE(bar.tabText(0), QStringLiteral("Hello"));
        QCOMPARE(bar.tabText(1), QStringLiteral("World"));
        // Out of range returns empty
        QCOMPARE(bar.tabText(5), QString());
    }

    void tabBar_setTabText()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("Old"));
        bar.setTabText(0, QStringLiteral("New"));
        QCOMPARE(bar.tabText(0), QStringLiteral("New"));
    }

    void tabBar_setCurrentIndex()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));
        bar.addTab(QStringLiteral("C"));

        QSignalSpy spy(&bar, &QTuiTabBar::currentChanged);
        bar.setCurrentIndex(2);
        QCOMPARE(bar.currentIndex(), 2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 2);
    }

    void tabBar_setCurrentIndexOutOfRange()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));

        QSignalSpy spy(&bar, &QTuiTabBar::currentChanged);
        bar.setCurrentIndex(10);
        QCOMPARE(bar.currentIndex(), 0); // unchanged
        QCOMPARE(spy.count(), 0);
    }

    void tabBar_currentChangedSignal()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));

        QSignalSpy spy(&bar, &QTuiTabBar::currentChanged);
        bar.setCurrentIndex(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);

        // Setting same index again should not emit
        bar.setCurrentIndex(1);
        QCOMPARE(spy.count(), 1);
    }

    void tabBar_tabPosition()
    {
        QTuiTabBar bar;
        QCOMPARE(bar.tabPosition(), QTuiTabBar::North);
        bar.setTabPosition(QTuiTabBar::South);
        QCOMPARE(bar.tabPosition(), QTuiTabBar::South);
        bar.setTabPosition(QTuiTabBar::North);
        QCOMPARE(bar.tabPosition(), QTuiTabBar::North);
    }

    void tabBar_keyLeftRight()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));
        bar.addTab(QStringLiteral("C"));
        QCOMPARE(bar.currentIndex(), 0);

        QSignalSpy spy(&bar, &QTuiTabBar::currentChanged);

        sendKey(bar, Qt::Key_Right);
        QCOMPARE(bar.currentIndex(), 1);

        sendKey(bar, Qt::Key_Right);
        QCOMPARE(bar.currentIndex(), 2);

        // Right at end does nothing
        sendKey(bar, Qt::Key_Right);
        QCOMPARE(bar.currentIndex(), 2);

        sendKey(bar, Qt::Key_Left);
        QCOMPARE(bar.currentIndex(), 1);

        // Left at beginning does nothing
        bar.setCurrentIndex(0);
        sendKey(bar, Qt::Key_Left);
        QCOMPARE(bar.currentIndex(), 0);
    }

    void tabBar_keyEnterEmitsSignal()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("A"));
        bar.addTab(QStringLiteral("B"));
        bar.setCurrentIndex(1);

        QSignalSpy spy(&bar, &QTuiTabBar::currentChanged);
        sendKey(bar, Qt::Key_Return);
        // Enter re-emits currentChanged for the current tab
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void tabBar_minimumSize()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("AB"));
        bar.addTab(QStringLiteral("CD"));
        // "AB | CD" = 2 + 3 + 2 = 7
        QSize ms = bar.minimumSize();
        QCOMPARE(ms.width(), 7);
        QCOMPARE(ms.height(), 1);
    }

    void tabBar_paintEvent()
    {
        QTuiTabBar bar;
        bar.addTab(QStringLiteral("Tab1"));
        bar.addTab(QStringLiteral("Tab2"));
        bar.setCurrentIndex(0);
        bar.setGeometry(QRect(0, 0, 20, 1));

        ScreenBuffer buffer(20, 1);
        bar.paintEvent(buffer);

        // Collect the rendered row
        QString row;
        for (int col = 0; col < 20; ++col)
            row += buffer.backCell(col, 0).character;

        QVERIFY(row.contains(QStringLiteral("Tab1")));
        QVERIFY(row.contains(QStringLiteral("Tab2")));
    }

    // ---------------------------------------------------------------
    // QTuiTabWidget tests
    // ---------------------------------------------------------------

    void tabWidget_defaultConstruction()
    {
        QTuiTabWidget tw;
        QCOMPARE(tw.count(), 0);
        QCOMPARE(tw.currentIndex(), -1);
        QVERIFY(tw.currentWidget() == nullptr);
        QVERIFY(tw.tabBar() != nullptr);
    }

    void tabWidget_addTab()
    {
        QTuiTabWidget tw;
        auto *page = new TabTestWidget;
        int idx = tw.addTab(page, QStringLiteral("Page1"));
        QCOMPARE(idx, 0);
        QCOMPARE(tw.count(), 1);
        QCOMPARE(tw.currentIndex(), 0);
        QCOMPARE(tw.currentWidget(), page);
        QCOMPARE(page->isVisible(), true);
    }

    void tabWidget_addMultipleTabs()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        auto *page2 = new TabTestWidget;
        auto *page3 = new TabTestWidget;

        tw.addTab(page1, QStringLiteral("P1"));
        tw.addTab(page2, QStringLiteral("P2"));
        tw.addTab(page3, QStringLiteral("P3"));

        QCOMPARE(tw.count(), 3);
        QCOMPARE(tw.currentIndex(), 0);
        // Only current page should be visible
        QCOMPARE(page1->isVisible(), true);
        QCOMPARE(page2->isVisible(), false);
        QCOMPARE(page3->isVisible(), false);
    }

    void tabWidget_removeTab()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        auto *page2 = new TabTestWidget;
        tw.addTab(page1, QStringLiteral("P1"));
        tw.addTab(page2, QStringLiteral("P2"));

        tw.removeTab(0);
        QCOMPARE(tw.count(), 1);
        QCOMPARE(tw.widget(0), page2);
    }

    void tabWidget_widgetAndIndexOf()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        auto *page2 = new TabTestWidget;
        tw.addTab(page1, QStringLiteral("P1"));
        tw.addTab(page2, QStringLiteral("P2"));

        QCOMPARE(tw.widget(0), page1);
        QCOMPARE(tw.widget(1), page2);
        QCOMPARE(tw.widget(5), nullptr);
        QCOMPARE(tw.indexOf(page1), 0);
        QCOMPARE(tw.indexOf(page2), 1);
    }

    void tabWidget_setCurrentIndex()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        auto *page2 = new TabTestWidget;
        tw.addTab(page1, QStringLiteral("P1"));
        tw.addTab(page2, QStringLiteral("P2"));

        QSignalSpy spy(&tw, &QTuiTabWidget::currentChanged);
        tw.setCurrentIndex(1);
        QCOMPARE(tw.currentIndex(), 1);
        QCOMPARE(tw.currentWidget(), page2);
        QCOMPARE(page1->isVisible(), false);
        QCOMPARE(page2->isVisible(), true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void tabWidget_currentChangedSignal()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        auto *page2 = new TabTestWidget;
        tw.addTab(page1, QStringLiteral("P1"));
        tw.addTab(page2, QStringLiteral("P2"));

        QSignalSpy spy(&tw, &QTuiTabWidget::currentChanged);
        tw.setCurrentIndex(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void tabWidget_tabBarAccessor()
    {
        QTuiTabWidget tw;
        QTuiTabBar *bar = tw.tabBar();
        QVERIFY(bar != nullptr);
        // Tab bar should be a child of the tab widget
        QCOMPARE(bar->parent(), &tw);
    }

    void tabWidget_pageVisibilityMatchesCurrentTab()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        auto *page2 = new TabTestWidget;
        auto *page3 = new TabTestWidget;
        tw.addTab(page1, QStringLiteral("P1"));
        tw.addTab(page2, QStringLiteral("P2"));
        tw.addTab(page3, QStringLiteral("P3"));

        // Initially page1 is current
        QCOMPARE(page1->isVisible(), true);
        QCOMPARE(page2->isVisible(), false);
        QCOMPARE(page3->isVisible(), false);

        // Switch to page2
        tw.setCurrentIndex(1);
        QCOMPARE(page1->isVisible(), false);
        QCOMPARE(page2->isVisible(), true);
        QCOMPARE(page3->isVisible(), false);

        // Switch to page3
        tw.setCurrentIndex(2);
        QCOMPARE(page1->isVisible(), false);
        QCOMPARE(page2->isVisible(), false);
        QCOMPARE(page3->isVisible(), true);
    }

    void tabWidget_paintEvent()
    {
        QTuiTabWidget tw;
        auto *page1 = new TabTestWidget;
        tw.addTab(page1, QStringLiteral("Tab1"));
        tw.setGeometry(QRect(0, 0, 20, 10));

        ScreenBuffer buffer(20, 10);
        tw.paintEvent(buffer);

        // Tab bar row should contain "Tab1"
        QString topRow;
        for (int col = 0; col < 20; ++col)
            topRow += buffer.backCell(col, 0).character;
        QVERIFY(topRow.contains(QStringLiteral("Tab1")));
    }

    // ---------------------------------------------------------------
    // QTuiToolBox tests
    // ---------------------------------------------------------------

    void toolBox_defaultConstruction()
    {
        QTuiToolBox tb;
        QCOMPARE(tb.count(), 0);
        QCOMPARE(tb.currentIndex(), -1);
    }

    void toolBox_addItem()
    {
        QTuiToolBox tb;
        auto *w = new TabTestWidget;
        int idx = tb.addItem(w, QStringLiteral("Item1"));
        QCOMPARE(idx, 0);
        QCOMPARE(tb.count(), 1);
        QCOMPARE(tb.currentIndex(), 0);
        QCOMPARE(w->isVisible(), true);
    }

    void toolBox_addMultipleItems()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        auto *w3 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));
        tb.addItem(w3, QStringLiteral("I3"));

        QCOMPARE(tb.count(), 3);
        QCOMPARE(tb.currentIndex(), 0);
        // Only current item's widget visible
        QCOMPARE(w1->isVisible(), true);
        QCOMPARE(w2->isVisible(), false);
        QCOMPARE(w3->isVisible(), false);
    }

    void toolBox_removeItem()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        auto *w3 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));
        tb.addItem(w3, QStringLiteral("I3"));

        tb.removeItem(0);
        QCOMPARE(tb.count(), 2);
        QCOMPARE(tb.widget(0), w2);
        QCOMPARE(tb.widget(1), w3);
    }

    void toolBox_widgetAndItemText()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("First"));
        tb.addItem(w2, QStringLiteral("Second"));

        QCOMPARE(tb.widget(0), w1);
        QCOMPARE(tb.widget(1), w2);
        QCOMPARE(tb.widget(5), nullptr);
        QCOMPARE(tb.itemText(0), QStringLiteral("First"));
        QCOMPARE(tb.itemText(1), QStringLiteral("Second"));
        QCOMPARE(tb.itemText(5), QString());
    }

    void toolBox_setCurrentIndex()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));

        QSignalSpy spy(&tb, &QTuiToolBox::currentChanged);
        tb.setCurrentIndex(1);
        QCOMPARE(tb.currentIndex(), 1);
        QCOMPARE(w1->isVisible(), false);
        QCOMPARE(w2->isVisible(), true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void toolBox_currentChangedSignal()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));

        QSignalSpy spy(&tb, &QTuiToolBox::currentChanged);
        tb.setCurrentIndex(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);

        // Setting same index again should not emit
        tb.setCurrentIndex(1);
        QCOMPARE(spy.count(), 1);
    }

    void toolBox_singleItemVisible()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        auto *w3 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));
        tb.addItem(w3, QStringLiteral("I3"));

        // Switch through all items, verify only one visible at a time
        for (int i = 0; i < 3; ++i) {
            tb.setCurrentIndex(i);
            int visibleCount = 0;
            for (int j = 0; j < 3; ++j) {
                if (tb.widget(j)->isVisible())
                    ++visibleCount;
            }
            QCOMPARE(visibleCount, 1);
            QCOMPARE(tb.widget(i)->isVisible(), true);
        }
    }

    void toolBox_keyUpDown()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        auto *w3 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));
        tb.addItem(w3, QStringLiteral("I3"));

        // Down moves focused header
        sendKey(tb, Qt::Key_Down);
        sendKey(tb, Qt::Key_Down);
        // Activate with Enter
        sendKey(tb, Qt::Key_Return);
        QCOMPARE(tb.currentIndex(), 2);

        // Up moves back
        sendKey(tb, Qt::Key_Up);
        sendKey(tb, Qt::Key_Return);
        QCOMPARE(tb.currentIndex(), 1);
    }

    void toolBox_keyEnterActivates()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("I1"));
        tb.addItem(w2, QStringLiteral("I2"));

        QSignalSpy spy(&tb, &QTuiToolBox::currentChanged);

        // Navigate to second header and activate with Space
        sendKey(tb, Qt::Key_Down);
        sendKey(tb, Qt::Key_Space);
        QCOMPARE(tb.currentIndex(), 1);
        QCOMPARE(spy.count(), 1);
    }

    void toolBox_paintEvent()
    {
        QTuiToolBox tb;
        auto *w1 = new TabTestWidget;
        auto *w2 = new TabTestWidget;
        tb.addItem(w1, QStringLiteral("Section1"));
        tb.addItem(w2, QStringLiteral("Section2"));
        tb.setGeometry(QRect(0, 0, 20, 10));

        ScreenBuffer buffer(20, 10);
        tb.paintEvent(buffer);

        // First header row should contain the expand indicator and "Section1"
        QString row0;
        for (int col = 0; col < 20; ++col)
            row0 += buffer.backCell(col, 0).character;
        QVERIFY(row0.contains(QStringLiteral("Section1")));
        // Current item uses ▼ indicator
        QVERIFY(row0.contains(QChar(0x25BC)));
    }
};

QTEST_MAIN(TestQTuiTabWidgets)
#include "test_qtui_tab_widgets.moc"
