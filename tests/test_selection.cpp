// Tests for the selection of the item views (QItemSelectionModel,
// selectionMode, selectionBehavior) and of the item widgets.

#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

static void click(QTuiWidget &w, int x, int y, Qt::KeyboardModifiers mods = Qt::NoModifier,
                  Qt::MouseButton button = Qt::LeftButton)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, button, QPoint(x, y), mods);
    QCoreApplication::sendEvent(&w, &ev);
}

static QList<int> selectedRows(const QTuiAbstractItemView &view)
{
    QList<int> rows;
    for (const QModelIndex &index : view.selectionModel()->selectedIndexes())
        rows.append(index.row());
    std::sort(rows.begin(), rows.end());
    return rows;
}

class TestSelection : public QObject {
    Q_OBJECT

private:
    QStringListModel m_model{QStringList{"a", "b", "c", "d", "e"}};

    void setUpList(QTuiListView &view, QTuiAbstractItemView::SelectionMode mode)
    {
        view.setGeometry(QRect(0, 0, 10, 5));
        view.setModel(&m_model);
        view.setSelectionMode(mode);
    }

private slots:
    void setModelMakesASelectionModel()
    {
        QTuiListView view;
        QVERIFY(!view.selectionModel());
        view.setModel(&m_model);
        QVERIFY(view.selectionModel());
        QCOMPARE(view.selectionModel()->model(), &m_model);

        QStringListModel other;
        QPointer<QItemSelectionModel> old = view.selectionModel();
        view.setModel(&other);
        QCOMPARE(view.selectionModel()->model(), &other);
        QVERIFY(old.isNull()); // the view owned it
    }

    void setSelectionModelRejectsOtherModel()
    {
        QTuiListView view;
        view.setModel(&m_model);
        QStringListModel other;
        QItemSelectionModel wrong(&other);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("different model"));
        view.setSelectionModel(&wrong);
        QVERIFY(view.selectionModel() != &wrong);
    }

    void singleSelectionFollowsTheCurrentItem()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::SingleSelection);
        view.setCurrentIndex(m_model.index(2, 0));
        QCOMPARE(selectedRows(view), QList<int>{2});
        sendKey(view, Qt::Key_Down);
        QCOMPARE(selectedRows(view), QList<int>{3});
        // Shift does not extend in single selection
        sendKey(view, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(selectedRows(view), QList<int>{4});
        sendKey(view, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(selectedRows(view), QList<int>{4});
    }

    void noSelectionSelectsNothing()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::NoSelection);
        sendKey(view, Qt::Key_Down);
        click(view, 0, 3);
        sendKey(view, Qt::Key_Space);
        QVERIFY(selectedRows(view).isEmpty());
        QCOMPARE(view.currentIndex().row(), 3);
    }

    void multiSelectionToggles()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::MultiSelection);
        click(view, 0, 1);
        click(view, 0, 3);
        QCOMPARE(selectedRows(view), (QList<int>{1, 3}));
        click(view, 0, 1);
        QCOMPARE(selectedRows(view), QList<int>{3});
        // The arrow keys move the current item only; Space toggles
        sendKey(view, Qt::Key_Down);
        QCOMPARE(selectedRows(view), QList<int>{3});
        sendKey(view, Qt::Key_Space);
        QCOMPARE(selectedRows(view), (QList<int>{2, 3}));
    }

    void extendedSelectionRangesAndToggles()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::ExtendedSelection);
        click(view, 0, 1);
        sendKey(view, Qt::Key_Down, Qt::ShiftModifier);
        sendKey(view, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2, 3}));
        // Shift+Up shrinks the range from the same anchor
        sendKey(view, Qt::Key_Up, Qt::ShiftModifier);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2}));
        // Ctrl+click toggles one item
        click(view, 0, 4, Qt::ControlModifier);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2, 4}));
        // Ctrl+arrow moves only; Ctrl+Space toggles
        sendKey(view, Qt::Key_Up, Qt::ControlModifier);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2, 4}));
        sendKey(view, Qt::Key_Space, Qt::ControlModifier);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2, 3, 4}));
        // Shift+click selects from the anchor (the toggled item)
        click(view, 0, 0, Qt::ShiftModifier);
        QCOMPARE(selectedRows(view), (QList<int>{0, 1, 2, 3}));
        // A plain click selects one item
        click(view, 0, 2);
        QCOMPARE(selectedRows(view), QList<int>{2});
        sendKey(view, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(selectedRows(view), (QList<int>{0, 1, 2, 3, 4}));
    }

    void firstShiftMoveStartsAtTheFirstItem()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::ExtendedSelection);
        sendKey(view, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(selectedRows(view), (QList<int>{0, 1}));
    }

    void contiguousSelectionIgnoresCtrl()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::ContiguousSelection);
        click(view, 0, 1);
        click(view, 0, 3, Qt::ControlModifier);
        QCOMPARE(selectedRows(view), QList<int>{3});
    }

    void rightClickKeepsTheSelection()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::ExtendedSelection);
        click(view, 0, 1);
        click(view, 0, 2, Qt::ShiftModifier);
        click(view, 0, 1, Qt::NoModifier, Qt::RightButton);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2}));
        // On an item outside the selection, it selects that item
        click(view, 0, 4, Qt::NoModifier, Qt::RightButton);
        QCOMPARE(selectedRows(view), QList<int>{4});
    }

    void dragSelectsARange()
    {
        // Some terminals keep Shift+click for their own text selection
        auto drag = [](QTuiWidget &w, int x, int y) {
            TuiMouseEvent ev(TuiMouseEvent::Move, Qt::LeftButton, QPoint(x, y));
            QCoreApplication::sendEvent(&w, &ev);
        };
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::ExtendedSelection);
        QSignalSpy activated(&view, &QTuiListView::activated);
        click(view, 0, 3);
        drag(view, 0, 2);
        drag(view, 0, 1);
        QCOMPARE(selectedRows(view), (QList<int>{1, 2, 3}));
        QCOMPARE(view.currentIndex().row(), 1);
        QCOMPARE(activated.count(), 1);

        QTuiTableWidget table(4, 2);
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        table.setSelectionBehavior(QTuiAbstractItemView::SelectRows);
        table.setSortingEnabled(true);
        click(table, 1, 1);
        drag(table, 1, 3);
        QCOMPARE(table.selectionModel()->selectedRows().size(), 3);
        // A drag over the header does not sort
        drag(table, 1, 0);
        QCOMPARE(table.sortColumn(), -1);
        QCOMPARE(table.selectionModel()->selectedRows().size(), 3);
    }

    void selectionModelMovesTheCurrentItem()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::ExtendedSelection);
        view.selectionModel()->select(m_model.index(0, 0), QItemSelectionModel::Select);
        view.selectionModel()->setCurrentIndex(m_model.index(3, 0), QItemSelectionModel::NoUpdate);
        QCOMPARE(view.currentIndex().row(), 3);
        // The view follows without a change to the selection
        QCOMPARE(selectedRows(view), QList<int>{0});
    }

    void currentItemFollowsRowChanges()
    {
        QStringListModel model(QStringList{"c", "a", "b"});
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 5));
        view.setModel(&model);
        view.setCurrentIndex(model.index(2, 0)); // "b"
        model.insertRows(0, 1);
        QCOMPARE(view.currentIndex(), view.selectionModel()->currentIndex());
        QCOMPARE(view.currentIndex().data().toString(), QStringLiteral("b"));
        model.sort(0); // "", "a", "b", "c"
        QCOMPARE(view.currentIndex(), view.selectionModel()->currentIndex());
        QCOMPARE(view.currentIndex().data().toString(), QStringLiteral("b"));

        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 10, 5));
        table.setModel(&model);
        table.setCurrentIndex(model.index(1, 0)); // "a"
        model.removeRows(0, 1);
        QCOMPARE(table.currentIndex(), table.selectionModel()->currentIndex());
        QCOMPARE(table.currentIndex().data().toString(), QStringLiteral("a"));
    }

    void treeCurrentItemFollowsRowChanges()
    {
        QStringListModel model(QStringList{"x", "y", "z"});
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 10, 5));
        tree.setModel(&model);
        tree.setCurrentIndex(model.index(2, 0));
        model.insertRows(0, 2);
        QCOMPARE(tree.currentIndex(), tree.selectionModel()->currentIndex());
        QCOMPARE(tree.currentIndex().data().toString(), QStringLiteral("z"));
    }

    void selectedItemsAreHighlightedWithoutFocus()
    {
        QTuiListView view;
        setUpList(view, QTuiAbstractItemView::MultiSelection);
        click(view, 0, 2);
        ScreenBuffer buffer(10, 5);
        view.paintEvent(buffer);
        // A dimmer color and bold text, not the focus look
        const ThemeProperties theme = view.themeProperties();
        QVERIFY(buffer.backCell(0, 2).background != theme.focusBackground);
        QVERIFY(buffer.backCell(0, 2).background != theme.background);
        QVERIFY(buffer.backCell(0, 2).bold);
        QCOMPARE(buffer.backCell(0, 1).background, theme.background);
    }

    void tableSelectRowsSelectsTheWholeRow()
    {
        QTuiTableWidget table(3, 3);
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        table.setSelectionBehavior(QTuiAbstractItemView::SelectRows);
        table.setItem(1, 0, new QTuiTableWidgetItem("x"));
        table.setItem(1, 2, new QTuiTableWidgetItem("y"));
        table.setCurrentIndex(table.model()->index(1, 1));
        QCOMPARE(table.selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table.selectedItems().size(), 2);
        sendKey(table, Qt::Key_Down, Qt::ShiftModifier);
        const auto ranges = table.selectedRanges();
        QCOMPARE(ranges.size(), 1);
        QCOMPARE(ranges[0].topRow(), 1);
        QCOMPARE(ranges[0].bottomRow(), 2);
        QCOMPARE(ranges[0].leftColumn(), 0);
        QCOMPARE(ranges[0].rightColumn(), 2);
    }

    void treeRangeCrossesLevels()
    {
        QTuiTreeWidget tree;
        tree.setGeometry(QRect(0, 0, 20, 10));
        tree.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        auto *a = new QTuiTreeWidgetItem({"a"});
        auto *b = new QTuiTreeWidgetItem({"b"});
        a->addChild(new QTuiTreeWidgetItem({"a1"}));
        tree.addTopLevelItem(a);
        tree.addTopLevelItem(b);
        tree.expand(tree.model()->index(0, 0));
        // Rows: a, a1, b
        click(tree, 0, 0);
        click(tree, 0, 2, Qt::ShiftModifier);
        QCOMPARE(tree.selectedItems().size(), 3);
        sendKey(tree, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(tree.selectedItems().size(), 3);
    }

    void listWidgetSelectedItems()
    {
        QTuiListWidget list;
        list.setGeometry(QRect(0, 0, 10, 5));
        for (const char *text : {"x", "y", "z"})
            list.addItem(QString::fromLatin1(text));
        list.setSelectionMode(QTuiAbstractItemView::MultiSelection);
        click(list, 0, 0);
        click(list, 0, 2);
        const auto items = list.selectedItems();
        QCOMPARE(items.size(), 2);
    }

    void uiLoaderSetsTheSelectionProperties()
    {
        UI_Loader loader;
        QScopedPointer<QTuiWidget> widget(loader.loadFromData(
            R"({"type": "QTuiListView", "selectionMode": "ExtendedSelection",
                "selectionBehavior": "SelectRows"})"));
        auto *view = qobject_cast<QTuiListView *>(widget.data());
        QVERIFY(view);
        QCOMPARE(view->selectionMode(), QTuiAbstractItemView::ExtendedSelection);
        QCOMPARE(view->selectionBehavior(), QTuiAbstractItemView::SelectRows);
    }

    void ctrlSpaceIsParsed()
    {
        InputParser parser(nullptr);
        QList<QPair<int, Qt::KeyboardModifiers>> keys;
        connect(&parser, &InputParser::keyEventParsed, this,
                [&keys](int key, Qt::KeyboardModifiers mods, const QString &) {
                    keys.append({key, mods});
                });
        parser.processInput(QByteArray(1, '\0'));
        QCOMPARE(keys.size(), 1);
        QCOMPARE(keys[0].first, int(Qt::Key_Space));
        QCOMPARE(keys[0].second, Qt::KeyboardModifiers(Qt::ControlModifier));
    }

    // Property-based test (RapidCheck): in extended selection, a Shift
    // move from an anchor selects exactly the rows between the anchor
    // and the current row.
    void propertyShiftRangeIsContiguous()
    {
        rc::check("Shift+arrow selects anchor..current", [this]() {
            QTuiListView view;
            setUpList(view, QTuiAbstractItemView::ExtendedSelection);
            const int anchor = *rc::gen::inRange(0, 5);
            click(view, 0, anchor);
            const auto moves = *rc::gen::container<std::vector<bool>>(rc::gen::arbitrary<bool>());
            for (bool down : moves)
                sendKey(view, down ? Qt::Key_Down : Qt::Key_Up, Qt::ShiftModifier);
            const int current = view.currentIndex().row();
            QList<int> expected;
            for (int r = qMin(anchor, current); r <= qMax(anchor, current); ++r)
                expected.append(r);
            RC_ASSERT(selectedRows(view) == expected);
        });
    }
};

QTEST_MAIN(TestSelection)
#include "test_selection.moc"
