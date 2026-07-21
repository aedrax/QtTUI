// Tests for the Widget Gallery Demo. They build the real gallery with
// demos/GalleryBuilder.cpp (the same code as the demo), so the test cannot
// drift from the demo. Widgets are found by type with findChildren().

#include <QtTest/QtTest>
#include <rapidcheck.h>
#include "GalleryBuilder.h"
#include <QtTUI/QtTUI>
#include <QDate>

using namespace QtTUI;

class TestGallery : public QObject {
    Q_OBJECT

private:
    QTuiApplication *m_app = nullptr;
    QTuiMainWindow *m_window = nullptr;
    QTuiTabWidget *m_tabs = nullptr;
    QTuiStatusBar *m_statusBar = nullptr;

    // The first widget of type T on tab page `tab`
    template <typename T>
    T *find(int tab) const
    {
        const auto all = m_tabs->widget(tab)->findChildren<T *>();
        return all.isEmpty() ? nullptr : all.first();
    }

    void sendKey(int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        TuiKeyEvent ev(key, mods);
        QCoreApplication::sendEvent(m_window, &ev);
    }

private slots:
    void initTestCase()
    {
        static int argc = 1;
        static char arg0[] = "test_gallery";
        static char *argv[] = { arg0, nullptr };
        m_app = new QTuiApplication(argc, argv);
        m_window = new QTuiMainWindow;
        m_window->setGeometry(QRect(0, 0, 80, 30));
        m_app->setMainWindow(m_window);
        buildGallery(*m_window);
        m_window->show();

        m_tabs = qobject_cast<QTuiTabWidget *>(m_window->centralWidget());
        m_statusBar = m_window->statusBar();
        QVERIFY(m_tabs);
        QVERIFY(m_statusBar);
    }

    void cleanupTestCase()
    {
        delete m_window;
        delete m_app;
    }

    void tabsHaveTheirNames()
    {
        const QStringList names = {
            QStringLiteral("Buttons"), QStringLiteral("Text"), QStringLiteral("Numeric"),
            QStringLiteral("Selection"), QStringLiteral("Model/View"), QStringLiteral("Containers"),
            QStringLiteral("Panes"), QStringLiteral("Layouts"), QStringLiteral("Main Window"),
            QStringLiteral("Specialized")};
        QCOMPARE(m_tabs->count(), names.size());
        for (int i = 0; i < names.size(); ++i)
            QCOMPARE(m_tabs->tabBar()->tabText(i), names.at(i));
    }

    void fileMenuHasItsActions()
    {
        QTuiMenuBar *bar = m_window->menuBar();
        QVERIFY(bar);
        const auto menus = bar->findChildren<QTuiMenu *>();
        QVERIFY(!menus.isEmpty());
        QStringList shortcuts;
        for (QTuiAction *action : menus.first()->actions())
            shortcuts.append(action->shortcut());
        QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+O")));
        QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+N")));
        QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+Q")));
    }

    void statusBarShowsKeyHints()
    {
        QVERIFY(m_statusBar->currentMessage().contains(QStringLiteral("Ctrl+Q quit")));
    }

    void eachTabHasItsWidgets()
    {
        QVERIFY(find<QTuiPushButton>(0) && find<QTuiToolButton>(0) && find<QTuiCheckBox>(0)
                && find<QTuiRadioButton>(0) && find<QTuiButtonGroup>(0));
        QVERIFY(find<QTuiLineEdit>(1) && find<QTuiTextEdit>(1) && find<QTuiPlainTextEdit>(1)
                && find<QTuiDateTimeEdit>(1) && find<QTuiCompleter>(1));
        QVERIFY(find<QTuiSpinBox>(2) && find<QTuiDoubleSpinBox>(2) && find<QTuiSlider>(2)
                && find<QTuiDial>(2) && find<QTuiLCDNumber>(2));
        QVERIFY(find<QTuiComboBox>(3) && find<QTuiProgressBar>(3));
        QVERIFY(find<QTuiTableView>(4) && find<QTuiListView>(4) && find<QTuiTreeView>(4)
                && find<QTuiColumnView>(4));
        QVERIFY(find<QTuiGroupBox>(5) && find<QTuiCollapsible>(5) && find<QTuiTabWidget>(5)
                && find<QTuiToolBox>(5) && find<QTuiStackedWidget>(5));
        QVERIFY(find<QTuiSplitter>(6) && find<QTuiScrollArea>(6) && find<QTuiMdiArea>(6)
                && find<QTuiDockWidget>(6));
        QVERIFY(find<QTuiMenuBar>(8) && find<QTuiToolBar>(8) && find<QTuiStatusBar>(8)
                && find<QTuiDockWidget>(8));
        QVERIFY(find<QTuiCalendarWidget>(9) && find<QTuiUndoView>(9) && find<QTuiFocusFrame>(9)
                && find<QTuiSizeGrip>(9));
    }

    void layoutsTabUsesEachLayoutType()
    {
        bool form = false, grid = false, stacked = false;
        for (QTuiGroupBox *box : m_tabs->widget(7)->findChildren<QTuiGroupBox *>()) {
            form = form || qobject_cast<QTuiFormLayout *>(box->layout());
            grid = grid || qobject_cast<QTuiGridLayout *>(box->layout());
            stacked = stacked || qobject_cast<QTuiStackedLayout *>(box->layout());
        }
        QVERIFY(form && grid && stacked);
    }

    void sectionHeadersUseTheBarStyle()
    {
        int headers = 0;
        for (QTuiLabel *label : m_tabs->findChildren<QTuiLabel *>()) {
            if (label->text().startsWith(QStringLiteral("▍ ")))
                ++headers;
        }
        QVERIFY(headers >= 30);
    }

    void arrowKeysSwitchTabsAndShowHints()
    {
        m_tabs->setCurrentIndex(0);
        m_tabs->tabBar()->setFocus();
        m_statusBar->showMessage(QStringLiteral("something else"));
        sendKey(Qt::Key_Right);
        QCOMPARE(m_tabs->currentIndex(), 1);
        QVERIFY(m_statusBar->currentMessage().contains(QStringLiteral("Ctrl+Q quit")));
        sendKey(Qt::Key_Left);
        QCOMPARE(m_tabs->currentIndex(), 0);
    }

    void tabKeyMovesFocusForwardAndBack()
    {
        m_tabs->setCurrentIndex(0);
        m_tabs->tabBar()->setFocus();
        sendKey(Qt::Key_Tab);
        QTuiWidget *first = QTuiWidget::focusWidget();
        QVERIFY(first && first != m_tabs->tabBar());
        sendKey(Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(QTuiWidget::focusWidget(), m_tabs->tabBar());
    }

    void lineEditReportsTextLength()
    {
        QTuiLineEdit *edit = find<QTuiLineEdit>(1);
        edit->setText(QStringLiteral("abcd"));
        QCOMPARE(m_statusBar->currentMessage(), QStringLiteral("Text length: 4"));
    }

    void comboBoxReportsSelection()
    {
        QTuiComboBox *combo = find<QTuiComboBox>(3);
        combo->setCurrentIndex(2);
        QCOMPARE(m_statusBar->currentMessage(),
                 QStringLiteral("ComboBox: %1").arg(combo->itemText(2)));
    }

    void calendarReportsDate()
    {
        QTuiCalendarWidget *calendar = find<QTuiCalendarWidget>(9);
        const QDate date(2026, 3, 14);
        calendar->setSelectedDate(date);
        QCOMPARE(m_statusBar->currentMessage(),
                 QStringLiteral("Date: %1").arg(date.toString(Qt::ISODate)));
    }

    void tableIsSortable()
    {
        QTuiTableView *table = find<QTuiTableView>(4);
        QVERIFY(table->isSortingEnabled());
        table->sortByColumn(0, Qt::AscendingOrder);
        const QAbstractItemModel *model = table->model();
        for (int row = 1; row < model->rowCount(); ++row) {
            QVERIFY(model->index(row - 1, 0).data().toString()
                    <= model->index(row, 0).data().toString());
        }
    }

    // Property-based tests (RapidCheck)
    void propertyOnlyTheCurrentPageIsVisible()
    {
        rc::check("Tab switching shows only the active page", [this]() {
            const int i = *rc::gen::inRange(0, m_tabs->count());
            m_tabs->setCurrentIndex(i);
            for (int j = 0; j < m_tabs->count(); ++j)
                RC_ASSERT(m_tabs->widget(j)->isVisible() == (j == i));
        });
    }

    void propertySliderDrivesLcd()
    {
        QTuiSlider *slider = find<QTuiSlider>(2);
        QTuiLCDNumber *lcd = find<QTuiLCDNumber>(2);
        rc::check("Slider-LCD synchronization", [slider, lcd]() {
            const int v = *rc::gen::inRange(slider->minimum(), slider->maximum() + 1);
            slider->setValue(v);
            RC_ASSERT(lcd->intValue() == v);
        });
    }
};

QTEST_APPLESS_MAIN(TestGallery)
#include "test_gallery.moc"
