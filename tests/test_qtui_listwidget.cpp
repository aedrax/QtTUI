#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include <QtTUI/QTuiListWidget.h>
#include <QtTUI/QTuiListWidgetItem.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiListWidget : public QObject {
    Q_OBJECT

private slots:
    // Item addition, removal, retrieval (Req 21.1–21.4)
    void testAddItemByString();
    void testAddItemByPointer();
    void testInsertItem();
    void testTakeItem();
    void testCount();
    void testItemRetrieval();

    // Current item tracking and signals (Req 21.5–21.7)
    void testCurrentItemAndRow();
    void testSetCurrentItem();
    void testCurrentItemChangedSignal();
    void testItemClickedSignal();

    // Item data synchronization with internal model (Req 21.8–21.9)
    void testItemTextSyncsToModel();
    void testItemCustomData();
    void testItemFlags();
    void testItemCheckState();
    void testItemIcon();

    // Constructor with parent
    void testItemConstructorWithParent();
};

void TestQTuiListWidget::testAddItemByString()
{
    QTuiListWidget lw;
    lw.addItem("Alpha");
    lw.addItem("Beta");
    lw.addItem("Gamma");

    QCOMPARE(lw.count(), 3);
    QCOMPARE(lw.item(0)->text(), QStringLiteral("Alpha"));
    QCOMPARE(lw.item(1)->text(), QStringLiteral("Beta"));
    QCOMPARE(lw.item(2)->text(), QStringLiteral("Gamma"));
}

void TestQTuiListWidget::testAddItemByPointer()
{
    QTuiListWidget lw;
    auto *item = new QTuiListWidgetItem("Hello");
    lw.addItem(item);

    QCOMPARE(lw.count(), 1);
    QCOMPARE(lw.item(0), item);
    QCOMPARE(item->listWidget(), &lw);
}

void TestQTuiListWidget::testInsertItem()
{
    QTuiListWidget lw;
    lw.addItem("First");
    lw.addItem("Third");

    auto *mid = new QTuiListWidgetItem("Second");
    lw.insertItem(1, mid);

    QCOMPARE(lw.count(), 3);
    QCOMPARE(lw.item(0)->text(), QStringLiteral("First"));
    QCOMPARE(lw.item(1)->text(), QStringLiteral("Second"));
    QCOMPARE(lw.item(2)->text(), QStringLiteral("Third"));
    QCOMPARE(mid->listWidget(), &lw);
}

void TestQTuiListWidget::testTakeItem()
{
    QTuiListWidget lw;
    lw.addItem("A");
    lw.addItem("B");
    lw.addItem("C");

    QTuiListWidgetItem *taken = lw.takeItem(1);
    QVERIFY(taken != nullptr);
    QCOMPARE(taken->text(), QStringLiteral("B"));
    QCOMPARE(taken->listWidget(), nullptr);
    QCOMPARE(lw.count(), 2);
    QCOMPARE(lw.item(0)->text(), QStringLiteral("A"));
    QCOMPARE(lw.item(1)->text(), QStringLiteral("C"));

    // Out-of-range returns nullptr
    QVERIFY(lw.takeItem(-1) == nullptr);
    QVERIFY(lw.takeItem(100) == nullptr);

    delete taken;
}

void TestQTuiListWidget::testCount()
{
    QTuiListWidget lw;
    QCOMPARE(lw.count(), 0);

    lw.addItem("One");
    QCOMPARE(lw.count(), 1);

    lw.addItem("Two");
    QCOMPARE(lw.count(), 2);

    delete lw.takeItem(0);
    QCOMPARE(lw.count(), 1);
}

void TestQTuiListWidget::testItemRetrieval()
{
    QTuiListWidget lw;
    QVERIFY(lw.item(-1) == nullptr);
    QVERIFY(lw.item(0) == nullptr);

    lw.addItem("Test");
    QVERIFY(lw.item(0) != nullptr);
    QVERIFY(lw.item(1) == nullptr);
}

void TestQTuiListWidget::testCurrentItemAndRow()
{
    QTuiListWidget lw;
    lw.setGeometry(QRect(0, 0, 20, 10));
    lw.addItem("A");
    lw.addItem("B");
    lw.addItem("C");

    // Initially currentRow depends on base class (row 0 after model set)
    lw.setCurrentRow(1);
    QCOMPARE(lw.currentRow(), 1);
    QCOMPARE(lw.currentItem(), lw.item(1));

    lw.setCurrentRow(2);
    QCOMPARE(lw.currentRow(), 2);
    QCOMPARE(lw.currentItem()->text(), QStringLiteral("C"));

    // Out-of-range setCurrentRow should be ignored
    lw.setCurrentRow(-1);
    QCOMPARE(lw.currentRow(), 2); // unchanged

    lw.setCurrentRow(100);
    QCOMPARE(lw.currentRow(), 2); // unchanged
}

void TestQTuiListWidget::testSetCurrentItem()
{
    QTuiListWidget lw;
    lw.setGeometry(QRect(0, 0, 20, 10));
    lw.addItem("X");
    lw.addItem("Y");
    lw.addItem("Z");

    lw.setCurrentItem(lw.item(2));
    QCOMPARE(lw.currentRow(), 2);
    QCOMPARE(lw.currentItem(), lw.item(2));
}

void TestQTuiListWidget::testCurrentItemChangedSignal()
{
    QTuiListWidget lw;
    lw.setGeometry(QRect(0, 0, 20, 10));
    lw.addItem("A");
    lw.addItem("B");
    lw.addItem("C");

    QSignalSpy spy(&lw, &QTuiListWidget::currentItemChanged);

    lw.setCurrentRow(1);
    QCOMPARE(spy.count(), 1);

    auto args = spy.last();
    QTuiListWidgetItem *current = args.at(0).value<QTuiListWidgetItem *>();
    QCOMPARE(current, lw.item(1));

    // Navigate via key press
    spy.clear();
    sendKey(lw, Qt::Key_Down);
    QCOMPARE(spy.count(), 1);
    current = spy.last().at(0).value<QTuiListWidgetItem *>();
    QCOMPARE(current, lw.item(2));
}

void TestQTuiListWidget::testItemClickedSignal()
{
    QTuiListWidget lw;
    lw.setGeometry(QRect(0, 0, 20, 10));
    lw.addItem("A");
    lw.addItem("B");

    lw.setCurrentRow(0);

    QSignalSpy spy(&lw, &QTuiListWidget::itemClicked);

    sendKey(lw, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);
    QTuiListWidgetItem *clicked = spy.last().at(0).value<QTuiListWidgetItem *>();
    QCOMPARE(clicked, lw.item(0));

    // Navigate down and press Enter again
    sendKey(lw, Qt::Key_Down);
    sendKey(lw, Qt::Key_Enter);
    QCOMPARE(spy.count(), 2);
    clicked = spy.last().at(0).value<QTuiListWidgetItem *>();
    QCOMPARE(clicked, lw.item(1));
}

void TestQTuiListWidget::testItemTextSyncsToModel()
{
    QTuiListWidget lw;
    lw.addItem("Original");

    // Verify model has the data
    QAbstractItemModel *model = lw.model();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0), Qt::DisplayRole).toString(),
             QStringLiteral("Original"));

    // Change item text, should sync to model
    lw.item(0)->setText("Updated");
    QCOMPARE(model->data(model->index(0, 0), Qt::DisplayRole).toString(),
             QStringLiteral("Updated"));
}

void TestQTuiListWidget::testItemCustomData()
{
    QTuiListWidgetItem item("Test");
    item.setData(Qt::UserRole, 42);
    item.setData(Qt::UserRole + 1, QStringLiteral("custom"));

    QCOMPARE(item.data(Qt::UserRole).toInt(), 42);
    QCOMPARE(item.data(Qt::UserRole + 1).toString(), QStringLiteral("custom"));
    QCOMPARE(item.text(), QStringLiteral("Test"));

    // Non-existent role returns invalid QVariant
    QVERIFY(!item.data(Qt::UserRole + 99).isValid());
}

void TestQTuiListWidget::testItemFlags()
{
    QTuiListWidgetItem item("Test");

    // Default flags
    QVERIFY(item.flags().testFlag(Qt::ItemIsSelectable));
    QVERIFY(item.flags().testFlag(Qt::ItemIsEnabled));

    // Set custom flags
    item.setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable);
    QVERIFY(item.flags().testFlag(Qt::ItemIsEditable));
    QVERIFY(!item.flags().testFlag(Qt::ItemIsEnabled));
}

void TestQTuiListWidget::testItemCheckState()
{
    QTuiListWidgetItem item("Check");
    QCOMPARE(item.checkState(), 0); // Unchecked

    item.setCheckState(2); // Checked
    QCOMPARE(item.checkState(), 2);
}

void TestQTuiListWidget::testItemIcon()
{
    QTuiListWidgetItem item("Icon");
    QVERIFY(item.icon().isEmpty());

    item.setIcon("★");
    QCOMPARE(item.icon(), QStringLiteral("★"));
}

void TestQTuiListWidget::testItemConstructorWithParent()
{
    QTuiListWidget lw;
    // Creating item with parent should auto-add to the list widget
    auto *item = new QTuiListWidgetItem("Auto", &lw);

    QCOMPARE(lw.count(), 1);
    QCOMPARE(lw.item(0), item);
    QCOMPARE(item->listWidget(), &lw);
}

QTEST_MAIN(TestQTuiListWidget)
#include "test_qtui_listwidget.moc"
