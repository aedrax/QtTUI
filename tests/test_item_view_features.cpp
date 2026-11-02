// Tests for the item view features: item colors from model roles, the
// paging keys, the vim keys, the mouse wheel and the column resize modes.
// The models have more rows than the views are high.

#include <QtTest/QtTest>
#include <QtTUI/QtTUI>
#include <QStringListModel>
#include <QAbstractTableModel>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

// A typed letter, as InputParser sends it (an upper case letter has Shift)
static void typeChar(QTuiWidget &w, QChar ch)
{
    sendKey(w, ch.toUpper().unicode(), ch.isUpper() ? Qt::ShiftModifier : Qt::NoModifier,
            QString(ch));
}

static void wheel(QTuiWidget &w, int delta, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiMouseEvent ev(TuiMouseEvent::Wheel, Qt::NoButton, w.geometry().topLeft() + QPoint(1, 2),
                     mods, delta);
    QCoreApplication::sendEvent(&w, &ev);
}

static QStringList numberedRows(int count)
{
    QStringList rows;
    for (int i = 0; i < count; ++i)
        rows << QStringLiteral("row %1").arg(i);
    return rows;
}

// A table model with colors in Qt::ForegroundRole and Qt::BackgroundRole.
// Row 1 has a Color foreground, row 2 a 0xRRGGBB background, row 3 a
// "#RRGGBB" foreground.
class ColorModel : public QAbstractTableModel {
public:
    int rows = 50;
    int cols = 3;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : rows;
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : cols;
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::DisplayRole)
            return QStringLiteral("r%1c%2").arg(index.row()).arg(index.column());
        if (role == Qt::ForegroundRole && index.row() == 1)
            return QVariant::fromValue(Color(255, 0, 0));
        if (role == Qt::BackgroundRole && index.row() == 2)
            return 0x0000ff;
        if (role == Qt::ForegroundRole && index.row() == 3)
            return QStringLiteral("#00ff00");
        return {};
    }
    QVariant headerData(int section, Qt::Orientation, int role) const override
    {
        return role == Qt::DisplayRole ? QVariant(QStringLiteral("H%1").arg(section)) : QVariant();
    }
};

class TestItemViewFeatures : public QObject {
    Q_OBJECT

private slots:
    void cleanup()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::TrueColor);
    }

    // --- Item colors from model roles ---

    void roleColorsShowInTheCells()
    {
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        ScreenBuffer buffer(30, 6);
        view.paintEvent(buffer);
        // Line 0 is the header, so model row r is on line r + 1
        QCOMPARE(buffer.backCell(0, 2).foreground, Color(255, 0, 0));
        QCOMPARE(buffer.backCell(0, 3).background, Color(0, 0, 255));
        QCOMPARE(buffer.backCell(0, 4).foreground, Color(0, 255, 0));
        const ThemeProperties theme = view.themeProperties();
        QCOMPARE(buffer.backCell(0, 5).foreground, theme.foreground);
    }

    void currentItemKeepsTheFocusColors()
    {
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        view.setFocus();
        view.setCurrentIndex(model.index(1, 0));
        ScreenBuffer buffer(30, 6);
        view.paintEvent(buffer);
        const ThemeProperties theme = view.themeProperties();
        QCOMPARE(buffer.backCell(0, 2).foreground, theme.focusForeground);
        QCOMPARE(buffer.backCell(0, 2).background, theme.focusBackground);
        // The next cell in the row is not current and has the role color
        const QRect next = view.visualRect(model.index(1, 1));
        QCOMPARE(buffer.backCell(next.x(), 2).foreground, Color(255, 0, 0));
    }

    void noColorModeIgnoresRoleColors()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::NoColor);
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        ScreenBuffer buffer(30, 6);
        view.paintEvent(buffer);
        const ThemeProperties theme = view.themeProperties();
        QCOMPARE(buffer.backCell(0, 2).foreground, theme.foreground);
        QCOMPARE(buffer.backCell(0, 3).background, theme.background);
    }

    void roleColorsInTheOtherViews()
    {
        ColorModel model;
        model.cols = 1;
        QTuiListView list;
        QTuiTreeView tree;
        QTuiColumnView columns;
        for (QTuiAbstractItemView *view : {static_cast<QTuiAbstractItemView *>(&list),
                                           static_cast<QTuiAbstractItemView *>(&tree),
                                           static_cast<QTuiAbstractItemView *>(&columns)}) {
            view->setGeometry(QRect(0, 0, 20, 5));
            view->setModel(&model);
            ScreenBuffer buffer(20, 5);
            view->paintEvent(buffer);
            const QRect rect = view->visualRect(model.index(1, 0));
            QCOMPARE(buffer.backCell(rect.x(), rect.y()).foreground, Color(255, 0, 0));
        }
    }

    // An application can change the colors with its own rule
    void itemStyleIsVirtual()
    {
        class RedView : public QTuiListView {
        protected:
            Cell itemStyle(const QModelIndex &index, bool isCurrent,
                           const ThemeProperties &theme) const override
            {
                Cell style = QTuiListView::itemStyle(index, isCurrent, theme);
                style.background = Color(200, 0, 0);
                return style;
            }
        };
        QStringListModel model(numberedRows(20));
        RedView view;
        view.setGeometry(QRect(0, 0, 10, 4));
        view.setModel(&model);
        ScreenBuffer buffer(10, 4);
        view.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 3).background, Color(200, 0, 0));
    }

    // --- Page Up, Page Down, Home and End ---

    void pagingKeysInTheListView()
    {
        QStringListModel model(numberedRows(50));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 10)); // 10 rows: a page is 9 rows
        view.setModel(&model);
        QSignalSpy current(view.selectionModel(), &QItemSelectionModel::currentChanged);

        // At the first row, Page Up and Home do not move
        sendKey(view, Qt::Key_PageUp);
        sendKey(view, Qt::Key_Home);
        QCOMPARE(view.currentIndex().row(), 0);
        QCOMPARE(current.count(), 0);

        sendKey(view, Qt::Key_PageDown);
        QCOMPARE(view.currentIndex().row(), 9);
        QCOMPARE(current.count(), 1);
        QVERIFY(view.visualRect(view.currentIndex()).isValid());
        sendKey(view, Qt::Key_PageDown);
        QCOMPARE(view.currentIndex().row(), 18);
        QCOMPARE(view.scrollOffset(), 9);
        sendKey(view, Qt::Key_PageUp);
        QCOMPARE(view.currentIndex().row(), 9);

        sendKey(view, Qt::Key_End);
        QCOMPARE(view.currentIndex().row(), 49);
        QCOMPARE(view.scrollOffset(), 40);
        // At the last row, Page Down and End do not move
        const int changes = current.count();
        sendKey(view, Qt::Key_PageDown);
        sendKey(view, Qt::Key_End);
        QCOMPARE(view.currentIndex().row(), 49);
        QCOMPARE(current.count(), changes);
        // The current item is the selected item (SingleSelection)
        QCOMPARE(view.selectionModel()->selectedIndexes(), {view.currentIndex()});

        sendKey(view, Qt::Key_Home);
        QCOMPARE(view.currentIndex().row(), 0);
        QCOMPARE(view.scrollOffset(), 0);
    }

    void pagingKeysWithAnEmptyModel()
    {
        QStringListModel model;
        QTuiListView list;
        QTuiTableView table;
        QTuiTreeView tree;
        QTuiColumnView columns;
        for (QTuiAbstractItemView *view : {static_cast<QTuiAbstractItemView *>(&list),
                                           static_cast<QTuiAbstractItemView *>(&table),
                                           static_cast<QTuiAbstractItemView *>(&tree),
                                           static_cast<QTuiAbstractItemView *>(&columns)}) {
            view->setGeometry(QRect(0, 0, 10, 5));
            view->setModel(&model);
            for (int key : {Qt::Key_PageDown, Qt::Key_PageUp, Qt::Key_End, Qt::Key_Home})
                sendKey(*view, key);
            typeChar(*view, QLatin1Char('G'));
            QVERIFY(!view->currentIndex().isValid());
            QCOMPARE(view->scrollOffset(), 0);
        }
    }

    void shiftPagingExtendsTheSelection()
    {
        QStringListModel model(numberedRows(50));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 10));
        view.setModel(&model);
        view.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        sendKey(view, Qt::Key_Down);
        sendKey(view, Qt::Key_PageDown, Qt::ShiftModifier);
        QCOMPARE(view.currentIndex().row(), 10);
        QCOMPARE(view.selectionModel()->selectedIndexes().size(), 10); // rows 1 to 10
        sendKey(view, Qt::Key_End, Qt::ShiftModifier);
        QCOMPARE(view.selectionModel()->selectedIndexes().size(), 49); // rows 1 to 49
        sendKey(view, Qt::Key_Home);
        QCOMPARE(view.selectionModel()->selectedIndexes().size(), 1);
    }

    void pagingKeysInTheTableView()
    {
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 11)); // a header and 10 rows
        view.setModel(&model);
        sendKey(view, Qt::Key_Right);
        sendKey(view, Qt::Key_PageDown);
        QCOMPARE(view.currentIndex(), model.index(9, 1));
        sendKey(view, Qt::Key_End, Qt::ControlModifier);
        QCOMPARE(view.currentIndex(), model.index(49, 1));
        QCOMPARE(view.scrollOffset(), 40);
        sendKey(view, Qt::Key_Home, Qt::ControlModifier);
        QCOMPARE(view.currentIndex(), model.index(0, 1));
        sendKey(view, Qt::Key_End);
        QCOMPARE(view.currentIndex(), model.index(49, 1));
        sendKey(view, Qt::Key_PageUp);
        QCOMPARE(view.currentIndex(), model.index(40, 1));
        sendKey(view, Qt::Key_Home);
        QCOMPARE(view.currentIndex(), model.index(0, 1));
    }

    void pagingKeysInTheTreeAndColumnViews()
    {
        QStringListModel model(numberedRows(50));
        QTuiTreeView tree;
        QTuiColumnView columns;
        for (QTuiAbstractItemView *view : {static_cast<QTuiAbstractItemView *>(&tree),
                                           static_cast<QTuiAbstractItemView *>(&columns)}) {
            view->setGeometry(QRect(0, 0, 20, 10));
            view->setModel(&model);
            sendKey(*view, Qt::Key_PageDown);
            QCOMPARE(view->currentIndex().row(), 9);
            sendKey(*view, Qt::Key_End);
            QCOMPARE(view->currentIndex().row(), 49);
            // The last row shows on the last line
            QCOMPARE(view->visualRect(view->currentIndex()).y(), 9);
            sendKey(*view, Qt::Key_PageDown);
            QCOMPARE(view->currentIndex().row(), 49);
            sendKey(*view, Qt::Key_PageUp);
            QCOMPARE(view->currentIndex().row(), 40);
            sendKey(*view, Qt::Key_Home);
            QCOMPARE(view->currentIndex().row(), 0);
            QCOMPARE(view->visualRect(view->currentIndex()).y(), 0);
        }
    }

    // --- The vim keys ---

    void vimKeys()
    {
        QStringListModel model(numberedRows(50));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 10));
        view.setModel(&model);
        typeChar(view, QLatin1Char('j'));
        typeChar(view, QLatin1Char('j'));
        QCOMPARE(view.currentIndex().row(), 2);
        typeChar(view, QLatin1Char('k'));
        QCOMPARE(view.currentIndex().row(), 1);
        typeChar(view, QLatin1Char('G'));
        QCOMPARE(view.currentIndex().row(), 49);
        typeChar(view, QLatin1Char('g'));
        typeChar(view, QLatin1Char('g'));
        QCOMPARE(view.currentIndex().row(), 0);
        sendKey(view, Qt::Key_F, Qt::ControlModifier);
        QCOMPARE(view.currentIndex().row(), 9);
        sendKey(view, Qt::Key_B, Qt::ControlModifier);
        QCOMPARE(view.currentIndex().row(), 0);
        sendKey(view, Qt::Key_D, Qt::ControlModifier);
        QCOMPARE(view.currentIndex().row(), 5);
        sendKey(view, Qt::Key_U, Qt::ControlModifier);
        QCOMPARE(view.currentIndex().row(), 0);
    }

    void vimKeysSelect()
    {
        QStringListModel model(numberedRows(50));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 10));
        view.setModel(&model);
        view.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        // Ctrl+D moves as Page Down does, so the selection follows
        sendKey(view, Qt::Key_D, Qt::ControlModifier);
        QCOMPARE(view.selectionModel()->selectedIndexes(), {model.index(5, 0)});
        // J and K extend the selection, as Shift+Down and Shift+Up do
        typeChar(view, QLatin1Char('J'));
        typeChar(view, QLatin1Char('J'));
        QCOMPARE(view.selectionModel()->selectedIndexes().size(), 3);
    }

    void vimKeysInTheTableAndTreeViews()
    {
        ColorModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        typeChar(table, QLatin1Char('l'));
        typeChar(table, QLatin1Char('j'));
        QCOMPARE(table.currentIndex(), model.index(1, 1));
        typeChar(table, QLatin1Char('h'));
        QCOMPARE(table.currentIndex(), model.index(1, 0));

        // In the tree view, l expands and h collapses, as Right and Left do
        class Tree : public QAbstractItemModel {
        public:
            QModelIndex index(int row, int column, const QModelIndex &parent) const override
            {
                return createIndex(row, column, parent.isValid() ? quintptr(1) : quintptr(0));
            }
            QModelIndex parent(const QModelIndex &child) const override
            {
                return child.internalId() ? createIndex(0, 0, quintptr(0)) : QModelIndex();
            }
            int rowCount(const QModelIndex &parent) const override
            {
                if (!parent.isValid())
                    return 2;
                return parent.internalId() == 0 && parent.row() == 0 ? 3 : 0;
            }
            int columnCount(const QModelIndex &) const override { return 1; }
            QVariant data(const QModelIndex &index, int role) const override
            {
                return role == Qt::DisplayRole ? QVariant(index.row()) : QVariant();
            }
        } treeModel;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 10));
        tree.setModel(&treeModel);
        typeChar(tree, QLatin1Char('l'));
        QVERIFY(tree.isExpanded(treeModel.index(0, 0, QModelIndex())));
        typeChar(tree, QLatin1Char('h'));
        QVERIFY(!tree.isExpanded(treeModel.index(0, 0, QModelIndex())));
    }

    // With the AnyKeyPressed trigger, a letter opens the editor
    void editTriggerTakesTheLetters()
    {
        QStringListModel model(numberedRows(20));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 5));
        view.setModel(&model);
        view.setEditTriggers(QTuiAbstractItemView::AnyKeyPressed);
        typeChar(view, QLatin1Char('j'));
        QVERIFY(view.isEditing());
        QCOMPARE(view.currentIndex().row(), 0);
    }

    // --- The mouse wheel ---

    void wheelScrollsButDoesNotMoveTheCurrentItem()
    {
        QStringListModel model(numberedRows(50));
        QTuiListView list;
        QTuiTreeView tree;
        QTuiTableView table;
        for (QTuiAbstractItemView *view : {static_cast<QTuiAbstractItemView *>(&list),
                                           static_cast<QTuiAbstractItemView *>(&tree),
                                           static_cast<QTuiAbstractItemView *>(&table)}) {
            view->setGeometry(QRect(0, 0, 20, 10));
            view->setModel(&model);
            QSignalSpy current(view->selectionModel(), &QItemSelectionModel::currentChanged);
            const QModelIndex start = view->currentIndex();

            wheel(*view, -120); // toward the user: down
            QCOMPARE(view->scrollOffset(), 3);
            wheel(*view, -240); // two notches
            QCOMPARE(view->scrollOffset(), 9);
            wheel(*view, 120);
            QCOMPARE(view->scrollOffset(), 6);
            QCOMPARE(view->currentIndex(), start);
            QCOMPARE(current.count(), 0);

            // The limits are the same as for scrollToRow()
            wheel(*view, 1200);
            QCOMPARE(view->scrollOffset(), 0);
            for (int i = 0; i < 30; ++i)
                wheel(*view, -120);
            const int visibleRows = view == &table ? 9 : 10;
            QCOMPARE(view->scrollOffset(), 50 - visibleRows);
        }
    }

    void wheelScrollLinesIsAProperty()
    {
        QStringListModel model(numberedRows(50));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 10));
        view.setModel(&model);
        QCOMPARE(view.wheelScrollLines(), 3);
        QVERIFY(view.setProperty("wheelScrollLines", 5));
        wheel(view, -120);
        QCOMPARE(view.scrollOffset(), 5);
    }

    void shiftWheelScrollsTheTableHorizontally()
    {
        ColorModel model;
        model.cols = 10;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 20, 10));
        view.setModel(&model);
        wheel(view, -120, Qt::ShiftModifier);
        QCOMPARE(view.horizontalScrollOffset(), 1);
        QCOMPARE(view.scrollOffset(), 0);
        wheel(view, 120, Qt::ShiftModifier);
        wheel(view, 120, Qt::ShiftModifier);
        QCOMPARE(view.horizontalScrollOffset(), 0);
        QCOMPARE(view.currentIndex(), model.index(0, 0));
    }

    void wheelInTheColumnView()
    {
        QStringListModel model(numberedRows(50));
        QTuiColumnView view;
        view.setGeometry(QRect(0, 0, 20, 10));
        view.setModel(&model);
        wheel(view, -120);
        QCOMPARE(view.visualRect(model.index(3, 0)).y(), 0);
        QCOMPARE(view.currentIndex().row(), 0);
        QVERIFY(view.visualRect(view.currentIndex()).isEmpty());
        // A key shows the current item again, as in Qt
        sendKey(view, Qt::Key_Down);
        QCOMPARE(view.visualRect(model.index(1, 0)).y(), 0);
    }

    // --- The vim keys in the line edit ---

    void lineEditDeleteKeys()
    {
        QTuiLineEdit edit;
        edit.setText(QStringLiteral("one two  three"));
        edit.setCursorPosition(14);
        sendKey(edit, Qt::Key_W, Qt::ControlModifier);
        QCOMPARE(edit.text(), QStringLiteral("one two  "));
        sendKey(edit, Qt::Key_W, Qt::ControlModifier);
        QCOMPARE(edit.text(), QStringLiteral("one "));
        edit.undo();
        QCOMPARE(edit.text(), QStringLiteral("one two  "));
        edit.setCursorPosition(4);
        sendKey(edit, Qt::Key_U, Qt::ControlModifier);
        QCOMPARE(edit.text(), QStringLiteral("two  "));
        QCOMPARE(edit.cursorPosition(), 0);
    }
};

QTEST_MAIN(TestItemViewFeatures)
#include "test_item_view_features.moc"
