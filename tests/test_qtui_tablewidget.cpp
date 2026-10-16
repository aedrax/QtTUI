#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include <QtTUI/QTuiTableWidget.h>
#include <QtTUI/QTuiTableWidgetItem.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiTableWidget : public QObject {
    Q_OBJECT

private slots:
    // Row/column count management (Req 22.1, 22.3)
    void testInitialDimensions();
    void testSortingSortsTheRows();
    void testSetRowCount();
    void testSetColumnCount();
    void testConstructorWithDimensions();

    // Item set/get (Req 22.4)
    void testSetAndGetItem();
    void testTakeItem();
    void testSetItemOutOfRange();
    void testReplaceItem();

    // Header labels (Req 22.5)
    void testHorizontalHeaderLabels();
    void testVerticalHeaderLabels();

    // Selection ranges (Req 22.9, 22.10)
    void testSelectedRanges();

    // Signals (Req 22.6, 22.7)
    void testCurrentCellChangedSignal();
    void testCellClickedSignal();

    // Item data (Req 22.8)
    void testItemTextAndData();
    void testItemFlags();
    void testItemRowColumn();

    // SelectionRange struct (Req 22.9)
    void testSelectionRangeStruct();

    // Model sync (Req 22.2)
    void testItemSyncsToModel();
    void testItemSetTextEmitsDataChanged();

    // Resize shrinks and cleans up items
    void testShrinkDeletesItems();
};

void TestQTuiTableWidget::testInitialDimensions()
{
    QTuiTableWidget tw;
    QCOMPARE(tw.rowCount(), 0);
    QCOMPARE(tw.columnCount(), 0);
}

void TestQTuiTableWidget::testSetRowCount()
{
    QTuiTableWidget tw;
    tw.setColumnCount(2);
    tw.setRowCount(3);
    QCOMPARE(tw.rowCount(), 3);
    QCOMPARE(tw.columnCount(), 2);

    tw.setRowCount(5);
    QCOMPARE(tw.rowCount(), 5);

    tw.setRowCount(1);
    QCOMPARE(tw.rowCount(), 1);
}

void TestQTuiTableWidget::testSetColumnCount()
{
    QTuiTableWidget tw;
    tw.setRowCount(2);
    tw.setColumnCount(4);
    QCOMPARE(tw.columnCount(), 4);

    tw.setColumnCount(1);
    QCOMPARE(tw.columnCount(), 1);
}

void TestQTuiTableWidget::testConstructorWithDimensions()
{
    QTuiTableWidget tw(3, 4);
    QCOMPARE(tw.rowCount(), 3);
    QCOMPARE(tw.columnCount(), 4);
}

void TestQTuiTableWidget::testSetAndGetItem()
{
    QTuiTableWidget tw(3, 3);
    auto *item = new QTuiTableWidgetItem("Hello");
    tw.setItem(1, 2, item);

    QCOMPARE(tw.item(1, 2), item);
    QCOMPARE(item->tableWidget(), &tw);
    QCOMPARE(item->row(), 1);
    QCOMPARE(item->column(), 2);

    // Null cells return nullptr
    QVERIFY(tw.item(0, 0) == nullptr);
}

void TestQTuiTableWidget::testTakeItem()
{
    QTuiTableWidget tw(2, 2);
    auto *item = new QTuiTableWidgetItem("Take me");
    tw.setItem(0, 1, item);

    QTuiTableWidgetItem *taken = tw.takeItem(0, 1);
    QCOMPARE(taken, item);
    QVERIFY(taken->tableWidget() == nullptr);
    QCOMPARE(taken->row(), -1);
    QCOMPARE(taken->column(), -1);
    QVERIFY(tw.item(0, 1) == nullptr);

    // Out-of-range returns nullptr
    QVERIFY(tw.takeItem(-1, 0) == nullptr);
    QVERIFY(tw.takeItem(0, 100) == nullptr);

    delete taken;
}

void TestQTuiTableWidget::testSetItemOutOfRange()
{
    QTuiTableWidget tw(2, 2);
    auto *item = new QTuiTableWidgetItem("OOB");
    tw.setItem(5, 5, item); // out of range, should be ignored
    QVERIFY(tw.item(5, 5) == nullptr);
    delete item; // not owned by widget
}

void TestQTuiTableWidget::testReplaceItem()
{
    QTuiTableWidget tw(2, 2);
    auto *first = new QTuiTableWidgetItem("First");
    tw.setItem(0, 0, first);
    QCOMPARE(tw.item(0, 0)->text(), QStringLiteral("First"));

    auto *second = new QTuiTableWidgetItem("Second");
    tw.setItem(0, 0, second);
    QCOMPARE(tw.item(0, 0)->text(), QStringLiteral("Second"));
    QCOMPARE(tw.item(0, 0), second);
    // first was deleted by setItem
}

void TestQTuiTableWidget::testHorizontalHeaderLabels()
{
    QTuiTableWidget tw(2, 3);
    tw.setHorizontalHeaderLabels({"Name", "Age", "City"});

    QAbstractItemModel *m = tw.model();
    QCOMPARE(m->headerData(0, Qt::Horizontal).toString(), QStringLiteral("Name"));
    QCOMPARE(m->headerData(1, Qt::Horizontal).toString(), QStringLiteral("Age"));
    QCOMPARE(m->headerData(2, Qt::Horizontal).toString(), QStringLiteral("City"));
}

void TestQTuiTableWidget::testVerticalHeaderLabels()
{
    QTuiTableWidget tw(3, 2);
    tw.setVerticalHeaderLabels({"Row1", "Row2", "Row3"});

    QAbstractItemModel *m = tw.model();
    QCOMPARE(m->headerData(0, Qt::Vertical).toString(), QStringLiteral("Row1"));
    QCOMPARE(m->headerData(1, Qt::Vertical).toString(), QStringLiteral("Row2"));
    QCOMPARE(m->headerData(2, Qt::Vertical).toString(), QStringLiteral("Row3"));
}

void TestQTuiTableWidget::testSelectedRanges()
{
    QTuiTableWidget tw(3, 3);
    tw.setGeometry(QRect(0, 0, 30, 10));
    tw.setItem(1, 1, new QTuiTableWidgetItem("Center"));

    // Navigate to row 1, col 1
    tw.setCurrentIndex(tw.model()->index(1, 1));

    auto ranges = tw.selectedRanges();
    QCOMPARE(ranges.size(), 1);
    QCOMPARE(ranges[0].topRow(), 1);
    QCOMPARE(ranges[0].bottomRow(), 1);
    QCOMPARE(ranges[0].leftColumn(), 1);
    QCOMPARE(ranges[0].rightColumn(), 1);
}

void TestQTuiTableWidget::testCurrentCellChangedSignal()
{
    QTuiTableWidget tw(3, 3);
    tw.setGeometry(QRect(0, 0, 30, 10));

    QSignalSpy spy(&tw, &QTuiTableWidget::currentCellChanged);

    // Navigate down from (0,0)
    sendKey(tw, Qt::Key_Down);
    QCOMPARE(spy.count(), 1);
    auto args = spy.last();
    QCOMPARE(args.at(0).toInt(), 1); // new row
    QCOMPARE(args.at(1).toInt(), 0); // new col
    QCOMPARE(args.at(2).toInt(), 0); // prev row
    QCOMPARE(args.at(3).toInt(), 0); // prev col

    // Navigate right
    spy.clear();
    sendKey(tw, Qt::Key_Right);
    QCOMPARE(spy.count(), 1);
    args = spy.last();
    QCOMPARE(args.at(0).toInt(), 1); // new row
    QCOMPARE(args.at(1).toInt(), 1); // new col
}

void TestQTuiTableWidget::testCellClickedSignal()
{
    QTuiTableWidget tw(3, 3);
    tw.setGeometry(QRect(0, 0, 30, 10));

    QSignalSpy spy(&tw, &QTuiTableWidget::cellClicked);

    sendKey(tw, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).toInt(), 0); // row
    QCOMPARE(spy.last().at(1).toInt(), 0); // col

    sendKey(tw, Qt::Key_Down);
    sendKey(tw, Qt::Key_Right);
    sendKey(tw, Qt::Key_Enter);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.last().at(0).toInt(), 1);
    QCOMPARE(spy.last().at(1).toInt(), 1);
    // A mouse click on a cell below the header emits it too
    TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(0, 3));
    QCoreApplication::sendEvent(&tw, &press);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.last().at(0).toInt(), 2);
    QCOMPARE(spy.last().at(1).toInt(), 0);
}

void TestQTuiTableWidget::testItemTextAndData()
{
    QTuiTableWidgetItem item("Hello");
    QCOMPARE(item.text(), QStringLiteral("Hello"));

    item.setText("World");
    QCOMPARE(item.text(), QStringLiteral("World"));

    item.setData(Qt::UserRole, 42);
    QCOMPARE(item.data(Qt::UserRole).toInt(), 42);

    // Non-existent role
    QVERIFY(!item.data(Qt::UserRole + 99).isValid());
}

void TestQTuiTableWidget::testItemFlags()
{
    QTuiTableWidgetItem item;
    QVERIFY(item.flags().testFlag(Qt::ItemIsSelectable));
    QVERIFY(item.flags().testFlag(Qt::ItemIsEnabled));

    item.setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable);
    QVERIFY(item.flags().testFlag(Qt::ItemIsEditable));
    QVERIFY(!item.flags().testFlag(Qt::ItemIsEnabled));
}

void TestQTuiTableWidget::testItemRowColumn()
{
    QTuiTableWidgetItem item("Standalone");
    QCOMPARE(item.row(), -1);
    QCOMPARE(item.column(), -1);
    QVERIFY(item.tableWidget() == nullptr);
}

void TestQTuiTableWidget::testSelectionRangeStruct()
{
    QTuiTableWidgetSelectionRange range;
    range.m_topRow = 1;
    range.m_leftColumn = 2;
    range.m_bottomRow = 3;
    range.m_rightColumn = 5;

    QCOMPARE(range.topRow(), 1);
    QCOMPARE(range.leftColumn(), 2);
    QCOMPARE(range.bottomRow(), 3);
    QCOMPARE(range.rightColumn(), 5);
    QCOMPARE(range.rowCount(), 3);
    QCOMPARE(range.columnCount(), 4);
}

void TestQTuiTableWidget::testItemSyncsToModel()
{
    QTuiTableWidget tw(2, 2);
    auto *item = new QTuiTableWidgetItem("Synced");
    tw.setItem(0, 0, item);

    QAbstractItemModel *m = tw.model();
    QCOMPARE(m->data(m->index(0, 0), Qt::DisplayRole).toString(),
             QStringLiteral("Synced"));
}

// Regression: setText/setData on an item already placed in the table must
// notify the internal model (dataChanged) so the view repaints.
void TestQTuiTableWidget::testItemSetTextEmitsDataChanged()
{
    QTuiTableWidget tw(2, 2);
    auto *item = new QTuiTableWidgetItem("Before");
    tw.setItem(1, 0, item);

    QSignalSpy spy(tw.model(), &QAbstractItemModel::dataChanged);

    item->setText("After");
    QCOMPARE(spy.count(), 1);
    QModelIndex changed = spy.last().at(0).value<QModelIndex>();
    QCOMPARE(changed.row(), 1);
    QCOMPARE(changed.column(), 0);
    QCOMPARE(tw.model()->data(tw.model()->index(1, 0), Qt::DisplayRole).toString(),
             QStringLiteral("After"));

    // setData must notify as well
    spy.clear();
    item->setData(Qt::UserRole, 42);
    QCOMPARE(spy.count(), 1);

    // A standalone item (not in a table) must not crash
    QTuiTableWidgetItem standalone("Loose");
    standalone.setText("Still loose");
    QCOMPARE(standalone.text(), QStringLiteral("Still loose"));
}

void TestQTuiTableWidget::testShrinkDeletesItems()
{
    QTuiTableWidget tw(3, 3);
    tw.setItem(2, 2, new QTuiTableWidgetItem("Corner"));
    tw.setItem(0, 0, new QTuiTableWidgetItem("Origin"));

    // Shrink rows - item at (2,2) should be cleaned up
    tw.setRowCount(2);
    QCOMPARE(tw.rowCount(), 2);
    QVERIFY(tw.item(0, 0) != nullptr);
    QCOMPARE(tw.item(0, 0)->text(), QStringLiteral("Origin"));

    // Shrink columns
    tw.setColumnCount(1);
    QCOMPARE(tw.columnCount(), 1);
    QVERIFY(tw.item(0, 0) != nullptr);
}

void TestQTuiTableWidget::testSortingSortsTheRows()
{
    QTuiTableWidget tw(3, 2);
    tw.setGeometry(QRect(0, 0, 30, 10));
    const QStringList names{QStringLiteral("Carol"), QStringLiteral("Alice"), QStringLiteral("Bob")};
    for (int row = 0; row < 3; ++row) {
        tw.setItem(row, 0, new QTuiTableWidgetItem(names.at(row)));
        tw.setItem(row, 1, new QTuiTableWidgetItem(QString::number(row)));
    }
    QTuiTableWidgetItem *alice = tw.item(1, 0);
    tw.setCurrentIndex(tw.model()->index(1, 0));
    tw.setSortingEnabled(true);

    tw.sortByColumn(0, Qt::AscendingOrder);
    QCOMPARE(tw.item(0, 0)->text(), QStringLiteral("Alice"));
    QCOMPARE(tw.item(0, 1)->text(), QStringLiteral("1"));
    QCOMPARE(tw.item(2, 0)->text(), QStringLiteral("Carol"));
    QCOMPARE(alice->row(), 0);
    // The current cell stays on the same item
    QCOMPARE(tw.currentIndex().row(), 0);

    tw.sortByColumn(0, Qt::DescendingOrder);
    QCOMPARE(tw.item(0, 0)->text(), QStringLiteral("Carol"));
}

QTEST_MAIN(TestQTuiTableWidget)
#include "test_qtui_tablewidget.moc"
