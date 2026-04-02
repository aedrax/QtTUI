#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include <QtTUI/QTuiTreeWidget.h>
#include <QtTUI/QTuiTreeWidgetItem.h>
#include <QtTUI/QTuiTreeWidgetItemIterator.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiTreeWidget : public QObject {
    Q_OBJECT

private slots:
    // Top-level item management (Req 23.1, 23.2, 23.3)
    void testAddTopLevelItem();
    void testTopLevelItemRetrieval();
    void testTakeTopLevelItem();
    void testTopLevelItemCount();

    // Child hierarchy (Req 23.7)
    void testAddChild();
    void testChildRetrieval();
    void testTakeChild();
    void testNestedChildren();

    // Multi-column text (Req 23.8)
    void testMultiColumnText();
    void testItemConstructorWithTexts();

    // Per-column check state (Req 23.9)
    void testPerColumnCheckState();

    // Data and flags
    void testItemData();
    void testItemFlags();

    // Header labels (Req 23.4)
    void testSetHeaderLabels();
    void testColumnCount();

    // Signals (Req 23.5, 23.6)
    void testCurrentItemChangedSignal();
    void testItemClickedSignal();

    // Current item tracking
    void testCurrentItem();
    void testSetCurrentItem();

    // Item-to-model synchronization
    void testItemSyncsToModel();
    void testChildSyncsToModel();

    // Iterator (Req 23.10, 23.11)
    void testIteratorAll();
    void testIteratorDepthFirst();
    void testIteratorCheckedFilter();
    void testIteratorUncheckedFilter();
    void testIteratorFromItem();
    void testIteratorAtEnd();
    void testIteratorPostIncrement();
};

void TestQTuiTreeWidget::testAddTopLevelItem()
{
    QTuiTreeWidget tw;
    auto *item = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(item);

    QCOMPARE(tw.topLevelItemCount(), 1);
    QCOMPARE(tw.topLevelItem(0), item);
    QCOMPARE(item->treeWidget(), &tw);
}

void TestQTuiTreeWidget::testTopLevelItemRetrieval()
{
    QTuiTreeWidget tw;
    QVERIFY(tw.topLevelItem(-1) == nullptr);
    QVERIFY(tw.topLevelItem(0) == nullptr);

    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    QVERIFY(tw.topLevelItem(0) != nullptr);
    QVERIFY(tw.topLevelItem(1) == nullptr);
}

void TestQTuiTreeWidget::testTakeTopLevelItem()
{
    QTuiTreeWidget tw;
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("C")}));

    QTuiTreeWidgetItem *taken = tw.takeTopLevelItem(1);
    QVERIFY(taken != nullptr);
    QCOMPARE(taken->text(0), QStringLiteral("B"));
    QCOMPARE(taken->treeWidget(), nullptr);
    QCOMPARE(tw.topLevelItemCount(), 2);
    QCOMPARE(tw.topLevelItem(0)->text(0), QStringLiteral("A"));
    QCOMPARE(tw.topLevelItem(1)->text(0), QStringLiteral("C"));

    QVERIFY(tw.takeTopLevelItem(-1) == nullptr);
    QVERIFY(tw.takeTopLevelItem(100) == nullptr);

    delete taken;
}

void TestQTuiTreeWidget::testTopLevelItemCount()
{
    QTuiTreeWidget tw;
    QCOMPARE(tw.topLevelItemCount(), 0);

    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("One")}));
    QCOMPARE(tw.topLevelItemCount(), 1);

    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("Two")}));
    QCOMPARE(tw.topLevelItemCount(), 2);

    delete tw.takeTopLevelItem(0);
    QCOMPARE(tw.topLevelItemCount(), 1);
}

void TestQTuiTreeWidget::testAddChild()
{
    QTuiTreeWidget tw;
    auto *root = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(root);

    auto *child = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child")});
    root->addChild(child);

    QCOMPARE(root->childCount(), 1);
    QCOMPARE(root->child(0), child);
    QCOMPARE(child->parent(), root);
    QCOMPARE(child->treeWidget(), &tw);
}

void TestQTuiTreeWidget::testChildRetrieval()
{
    QTuiTreeWidgetItem root(QStringList{QStringLiteral("Root")});
    QVERIFY(root.child(-1) == nullptr);
    QVERIFY(root.child(0) == nullptr);

    auto *c = new QTuiTreeWidgetItem(QStringList{QStringLiteral("C")});
    root.addChild(c);
    QCOMPARE(root.child(0), c);
    QVERIFY(root.child(1) == nullptr);
}

void TestQTuiTreeWidget::testTakeChild()
{
    QTuiTreeWidget tw;
    auto *root = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(root);

    root->addChild(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    root->addChild(new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")}));

    QTuiTreeWidgetItem *taken = root->takeChild(0);
    QVERIFY(taken != nullptr);
    QCOMPARE(taken->text(0), QStringLiteral("A"));
    QCOMPARE(taken->parent(), nullptr);
    QCOMPARE(taken->treeWidget(), nullptr);
    QCOMPARE(root->childCount(), 1);
    QCOMPARE(root->child(0)->text(0), QStringLiteral("B"));

    QVERIFY(root->takeChild(-1) == nullptr);
    QVERIFY(root->takeChild(100) == nullptr);

    delete taken;
}

void TestQTuiTreeWidget::testNestedChildren()
{
    QTuiTreeWidget tw;
    auto *root = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(root);

    auto *child = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child")});
    root->addChild(child);

    auto *grandchild = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Grandchild")});
    child->addChild(grandchild);

    QCOMPARE(grandchild->parent(), child);
    QCOMPARE(grandchild->treeWidget(), &tw);
    QCOMPARE(child->childCount(), 1);
    QCOMPARE(child->child(0), grandchild);
}

void TestQTuiTreeWidget::testMultiColumnText()
{
    QTuiTreeWidgetItem item;
    item.setText(0, QStringLiteral("Col0"));
    item.setText(1, QStringLiteral("Col1"));
    item.setText(2, QStringLiteral("Col2"));

    QCOMPARE(item.text(0), QStringLiteral("Col0"));
    QCOMPARE(item.text(1), QStringLiteral("Col1"));
    QCOMPARE(item.text(2), QStringLiteral("Col2"));
    QCOMPARE(item.columnCount(), 3);

    // Non-existent column returns empty
    QCOMPARE(item.text(5), QString());
}

void TestQTuiTreeWidget::testItemConstructorWithTexts()
{
    QTuiTreeWidgetItem item(QStringList{QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")});
    QCOMPARE(item.text(0), QStringLiteral("A"));
    QCOMPARE(item.text(1), QStringLiteral("B"));
    QCOMPARE(item.text(2), QStringLiteral("C"));
    QCOMPARE(item.columnCount(), 3);
}

void TestQTuiTreeWidget::testPerColumnCheckState()
{
    QTuiTreeWidgetItem item;
    QCOMPARE(item.checkState(0), 0); // default unchecked

    item.setCheckState(0, 2); // Checked
    QCOMPARE(item.checkState(0), 2);

    item.setCheckState(1, 1); // PartiallyChecked
    QCOMPARE(item.checkState(1), 1);
    // Column 0 unchanged
    QCOMPARE(item.checkState(0), 2);
}

void TestQTuiTreeWidget::testItemData()
{
    QTuiTreeWidgetItem item;
    item.setData(0, Qt::UserRole, 42);
    item.setData(0, Qt::UserRole + 1, QStringLiteral("custom"));
    item.setData(1, Qt::UserRole, QStringLiteral("col1data"));

    QCOMPARE(item.data(0, Qt::UserRole).toInt(), 42);
    QCOMPARE(item.data(0, Qt::UserRole + 1).toString(), QStringLiteral("custom"));
    QCOMPARE(item.data(1, Qt::UserRole).toString(), QStringLiteral("col1data"));

    // Non-existent returns invalid
    QVERIFY(!item.data(5, Qt::UserRole).isValid());
}

void TestQTuiTreeWidget::testItemFlags()
{
    QTuiTreeWidgetItem item;
    QVERIFY(item.flags().testFlag(Qt::ItemIsSelectable));
    QVERIFY(item.flags().testFlag(Qt::ItemIsEnabled));

    item.setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable);
    QVERIFY(item.flags().testFlag(Qt::ItemIsEditable));
    QVERIFY(!item.flags().testFlag(Qt::ItemIsEnabled));
}

void TestQTuiTreeWidget::testSetHeaderLabels()
{
    QTuiTreeWidget tw;
    tw.setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});

    QCOMPARE(tw.columnCount(), 2);

    // Verify model header data
    QAbstractItemModel *model = tw.model();
    QCOMPARE(model->headerData(0, Qt::Horizontal).toString(), QStringLiteral("Name"));
    QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), QStringLiteral("Value"));
}

void TestQTuiTreeWidget::testColumnCount()
{
    QTuiTreeWidget tw;
    QCOMPARE(tw.columnCount(), 1); // default

    tw.setColumnCount(3);
    QCOMPARE(tw.columnCount(), 3);

    // Minimum is 1
    tw.setColumnCount(0);
    QCOMPARE(tw.columnCount(), 1);
}

void TestQTuiTreeWidget::testCurrentItemChangedSignal()
{
    QTuiTreeWidget tw;
    tw.setGeometry(QRect(0, 0, 40, 10));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("C")}));

    QSignalSpy spy(&tw, &QTuiTreeWidget::currentItemChanged);

    sendKey(tw, Qt::Key_Down);
    QCOMPARE(spy.count(), 1);

    auto args = spy.last();
    auto *current = args.at(0).value<QTuiTreeWidgetItem *>();
    QCOMPARE(current, tw.topLevelItem(1));
}

void TestQTuiTreeWidget::testItemClickedSignal()
{
    QTuiTreeWidget tw;
    tw.setGeometry(QRect(0, 0, 40, 10));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));

    QSignalSpy spy(&tw, &QTuiTreeWidget::itemClicked);

    sendKey(tw, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);

    auto *clicked = spy.last().at(0).value<QTuiTreeWidgetItem *>();
    QCOMPARE(clicked, tw.topLevelItem(0));
}

void TestQTuiTreeWidget::testCurrentItem()
{
    QTuiTreeWidget tw;
    tw.setGeometry(QRect(0, 0, 40, 10));
    QVERIFY(tw.currentItem() == nullptr);

    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")}));

    // After adding items, current should be first item (row 0)
    QCOMPARE(tw.currentItem(), tw.topLevelItem(0));
}

void TestQTuiTreeWidget::testSetCurrentItem()
{
    QTuiTreeWidget tw;
    tw.setGeometry(QRect(0, 0, 40, 10));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("C")}));

    tw.setCurrentItem(tw.topLevelItem(2));
    QCOMPARE(tw.currentItem(), tw.topLevelItem(2));
}

void TestQTuiTreeWidget::testItemSyncsToModel()
{
    QTuiTreeWidget tw;
    auto *item = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Original")});
    tw.addTopLevelItem(item);

    QAbstractItemModel *model = tw.model();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0), Qt::DisplayRole).toString(),
             QStringLiteral("Original"));

    // Change item text, should sync to model
    item->setText(0, QStringLiteral("Updated"));
    QCOMPARE(model->data(model->index(0, 0), Qt::DisplayRole).toString(),
             QStringLiteral("Updated"));
}

void TestQTuiTreeWidget::testChildSyncsToModel()
{
    QTuiTreeWidget tw;
    auto *root = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(root);

    auto *child = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child")});
    root->addChild(child);

    QAbstractItemModel *model = tw.model();
    QModelIndex rootIdx = model->index(0, 0);
    QCOMPARE(model->rowCount(rootIdx), 1);
    QModelIndex childIdx = model->index(0, 0, rootIdx);
    QCOMPARE(model->data(childIdx, Qt::DisplayRole).toString(),
             QStringLiteral("Child"));
}

void TestQTuiTreeWidget::testIteratorAll()
{
    QTuiTreeWidget tw;
    auto *a = new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")});
    auto *b = new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")});
    auto *c = new QTuiTreeWidgetItem(QStringList{QStringLiteral("C")});
    tw.addTopLevelItem(a);
    tw.addTopLevelItem(b);
    tw.addTopLevelItem(c);

    QStringList visited;
    QTuiTreeWidgetItemIterator it(&tw);
    while (!it.atEnd()) {
        visited.append((*it)->text(0));
        ++it;
    }

    QCOMPARE(visited, (QStringList{QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")}));
}

void TestQTuiTreeWidget::testIteratorDepthFirst()
{
    QTuiTreeWidget tw;
    auto *root = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(root);

    auto *child1 = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child1")});
    auto *child2 = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child2")});
    root->addChild(child1);
    root->addChild(child2);

    auto *grandchild = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Grandchild")});
    child1->addChild(grandchild);

    QStringList visited;
    QTuiTreeWidgetItemIterator it(&tw);
    while (!it.atEnd()) {
        visited.append((*it)->text(0));
        ++it;
    }

    // Depth-first: Root -> Child1 -> Grandchild -> Child2
    QCOMPARE(visited, (QStringList{
        QStringLiteral("Root"),
        QStringLiteral("Child1"),
        QStringLiteral("Grandchild"),
        QStringLiteral("Child2")
    }));
}

void TestQTuiTreeWidget::testIteratorCheckedFilter()
{
    QTuiTreeWidget tw;
    auto *a = new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")});
    auto *b = new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")});
    auto *c = new QTuiTreeWidgetItem(QStringList{QStringLiteral("C")});
    a->setCheckState(0, 2); // Checked
    b->setCheckState(0, 0); // Unchecked
    c->setCheckState(0, 2); // Checked
    tw.addTopLevelItem(a);
    tw.addTopLevelItem(b);
    tw.addTopLevelItem(c);

    QStringList visited;
    QTuiTreeWidgetItemIterator it(&tw, QTuiTreeWidgetItemIterator::Checked);
    while (!it.atEnd()) {
        visited.append((*it)->text(0));
        ++it;
    }

    QCOMPARE(visited, (QStringList{QStringLiteral("A"), QStringLiteral("C")}));
}

void TestQTuiTreeWidget::testIteratorUncheckedFilter()
{
    QTuiTreeWidget tw;
    auto *a = new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")});
    auto *b = new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")});
    a->setCheckState(0, 2); // Checked
    b->setCheckState(0, 0); // Unchecked
    tw.addTopLevelItem(a);
    tw.addTopLevelItem(b);

    QStringList visited;
    QTuiTreeWidgetItemIterator it(&tw, QTuiTreeWidgetItemIterator::Unchecked);
    while (!it.atEnd()) {
        visited.append((*it)->text(0));
        ++it;
    }

    QCOMPARE(visited, (QStringList{QStringLiteral("B")}));
}

void TestQTuiTreeWidget::testIteratorFromItem()
{
    QTuiTreeWidget tw;
    auto *root = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Root")});
    tw.addTopLevelItem(root);

    auto *child1 = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child1")});
    auto *child2 = new QTuiTreeWidgetItem(QStringList{QStringLiteral("Child2")});
    root->addChild(child1);
    root->addChild(child2);

    // Iterate starting from child1
    QStringList visited;
    QTuiTreeWidgetItemIterator it(child1);
    while (!it.atEnd()) {
        visited.append((*it)->text(0));
        ++it;
    }

    // Should visit child1 only (no children of its own)
    QCOMPARE(visited, (QStringList{QStringLiteral("Child1")}));
}

void TestQTuiTreeWidget::testIteratorAtEnd()
{
    QTuiTreeWidget tw;
    QTuiTreeWidgetItemIterator it(&tw);
    QVERIFY(it.atEnd());
    QVERIFY(*it == nullptr);
}

void TestQTuiTreeWidget::testIteratorPostIncrement()
{
    QTuiTreeWidget tw;
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("A")}));
    tw.addTopLevelItem(new QTuiTreeWidgetItem(QStringList{QStringLiteral("B")}));

    QTuiTreeWidgetItemIterator it(&tw);
    QTuiTreeWidgetItemIterator prev = it++;

    QCOMPARE((*prev)->text(0), QStringLiteral("A"));
    QCOMPARE((*it)->text(0), QStringLiteral("B"));
}

QTEST_MAIN(TestQTuiTreeWidget)
#include "test_qtui_treewidget.moc"
