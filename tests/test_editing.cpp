// Tests for the in-place editing of the item views (edit triggers,
// edit(), the editor lifetime) and for scrollTo() and visualRect().

#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QtTUI>
#include <QStringListModel>
#include <QJsonDocument>
#include <QJsonObject>

using namespace QtTUI;

static void sendKey(QTuiWidget *w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(w, &ev);
}

static void sendText(QTuiWidget *w, const QString &text)
{
    for (const QChar ch : text)
        sendKey(w, ch.unicode(), Qt::NoModifier, QString(ch));
}

static void doubleClick(QTuiWidget &w, int x, int y)
{
    for (auto type : {TuiMouseEvent::Press, TuiMouseEvent::Release, TuiMouseEvent::DoubleClick}) {
        TuiMouseEvent ev(type, Qt::LeftButton, QPoint(x, y));
        QCoreApplication::sendEvent(&w, &ev);
    }
}

// A table with a text column and a number column
class NumberModel : public QAbstractTableModel {
public:
    QStringList names{"one", "two", "three"};
    QList<int> values{1, 2, 3};

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : names.size();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 2;
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role != Qt::DisplayRole && role != Qt::EditRole)
            return {};
        if (index.column() == 0)
            return names.at(index.row());
        return values.at(index.row());
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role != Qt::EditRole)
            return false;
        if (index.column() == 0)
            names[index.row()] = value.toString();
        else
            values[index.row()] = value.toInt();
        emit dataChanged(index, index);
        return true;
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        // The first row is read-only
        const Qt::ItemFlags base = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return index.row() == 0 ? base : base | Qt::ItemIsEditable;
    }
};

class TestEditing : public QObject {
    Q_OBJECT

private:
    QStringListModel m_model;

    void setUpList(QTuiListView &view)
    {
        m_model.setStringList({"a", "b", "c", "d", "e"});
        view.setGeometry(QRect(2, 1, 10, 5));
        view.setModel(&m_model);
        view.setFocus();
    }

private slots:
    void f2OpensALineEditOnTheItem()
    {
        QTuiListView view;
        setUpList(view);
        view.setCurrentIndex(m_model.index(2, 0));
        sendKey(&view, Qt::Key_F2);
        QVERIFY(view.isEditing());
        auto *lineEdit = qobject_cast<QTuiLineEdit *>(view.editor());
        QVERIFY(lineEdit);
        QCOMPARE(lineEdit->text(), QStringLiteral("c"));
        QCOMPARE(lineEdit->geometry(), QRect(2, 3, 10, 1));
        QCOMPARE(lineEdit->geometry(), view.visualRect(m_model.index(2, 0)));
        QVERIFY(lineEdit->hasFocus());
    }

    void enterWritesTheValue()
    {
        QTuiListView view;
        setUpList(view);
        view.setCurrentIndex(m_model.index(1, 0));
        sendKey(&view, Qt::Key_F2);
        QTuiWidget *editor = view.editor();
        // The text is selected, so typing replaces it
        sendText(editor, QStringLiteral("xy"));
        sendKey(editor, Qt::Key_Return);
        QVERIFY(!view.isEditing());
        QCOMPARE(m_model.index(1, 0).data().toString(), QStringLiteral("xy"));
        QVERIFY(view.hasFocus());
    }

    void escapeKeepsTheValue()
    {
        QTuiListView view;
        setUpList(view);
        sendKey(&view, Qt::Key_F2);
        sendText(view.editor(), QStringLiteral("zzz"));
        sendKey(view.editor(), Qt::Key_Escape);
        QVERIFY(!view.isEditing());
        QCOMPARE(m_model.index(0, 0).data().toString(), QStringLiteral("a"));
        QVERIFY(view.hasFocus());
    }

    void focusLossWritesTheValue()
    {
        QTuiWidget root;
        QTuiListView view(&root);
        setUpList(view);
        QTuiLineEdit other(&root);
        sendKey(&view, Qt::Key_F2);
        sendText(view.editor(), QStringLiteral("q"));
        other.setFocus();
        QVERIFY(!view.isEditing());
        QCOMPARE(m_model.index(0, 0).data().toString(), QStringLiteral("q"));
        QVERIFY(other.hasFocus());
    }

    void typingOpensTheEditorOnlyWithAnyKeyPressed()
    {
        QTuiListView view;
        setUpList(view);
        sendKey(&view, Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
        QVERIFY(!view.isEditing());

        view.setEditTriggers(QTuiAbstractItemView::AnyKeyPressed);
        sendKey(&view, Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
        QVERIFY(view.isEditing());
        sendKey(view.editor(), Qt::Key_Return);
        QCOMPARE(m_model.index(0, 0).data().toString(), QStringLiteral("x"));
        // F2 is not in the triggers now
        sendKey(&view, Qt::Key_F2);
        QVERIFY(!view.isEditing());
    }

    void doubleClickOpensTheEditor()
    {
        QTuiListView view;
        setUpList(view);
        doubleClick(view, 3, 4); // row 3
        QVERIFY(view.isEditing());
        QCOMPARE(qobject_cast<QTuiLineEdit *>(view.editor())->text(), QStringLiteral("d"));
        view.closeEditor(false);

        view.setEditTriggers(QTuiAbstractItemView::NoEditTriggers);
        doubleClick(view, 3, 4);
        QVERIFY(!view.isEditing());
    }

    void editorFollowsAndClosesWithItsRow()
    {
        QTuiListView view;
        setUpList(view);
        view.setCurrentIndex(m_model.index(2, 0));
        sendKey(&view, Qt::Key_F2);
        m_model.insertRows(0, 1);
        QVERIFY(view.isEditing());
        QCOMPARE(view.editor()->geometry(), view.visualRect(m_model.index(3, 0)));
        sendText(view.editor(), QStringLiteral("gone"));
        m_model.removeRows(3, 1);
        QVERIFY(!view.isEditing());
        QVERIFY(!m_model.stringList().contains(QStringLiteral("gone")));
    }

    void tableEditsNumbersWithASpinBox()
    {
        NumberModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        table.setFocus();
        const QModelIndex cell = model.index(1, 1);
        QVERIFY(table.edit(cell));
        auto *spin = qobject_cast<QTuiSpinBox *>(table.editor());
        QVERIFY(spin);
        QCOMPARE(spin->value(), 2);
        QCOMPARE(spin->geometry(), table.visualRect(cell));
        QCOMPARE(spin->geometry().y(), 2); // after the header and row 0
        QVERIFY(spin->geometry().x() > 0);
        sendKey(spin, Qt::Key_Up);
        sendKey(spin, Qt::Key_Enter);
        QCOMPARE(model.values.at(1), 3);
    }

    void readOnlyItemsDoNotEdit()
    {
        NumberModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        QVERIFY(!table.edit(model.index(0, 0)));
        sendKey(&table, Qt::Key_F2); // the current cell is row 0
        QVERIFY(!table.isEditing());
    }

    void treeEditsAfterTheIndent()
    {
        QTuiTreeWidget tree;
        tree.setGeometry(QRect(0, 0, 20, 5));
        auto *parent = new QTuiTreeWidgetItem({"p"});
        parent->addChild(new QTuiTreeWidgetItem({"c"}));
        tree.addTopLevelItem(parent);
        const QModelIndex child = tree.model()->index(0, 0, tree.model()->index(0, 0));
        // scrollTo() expands the parent
        tree.scrollTo(child);
        QVERIFY(tree.isExpanded(tree.model()->index(0, 0)));
        const QRect rect = tree.visualRect(child);
        QCOMPARE(rect, QRect(4, 1, 16, 1));
    }

    void scrollToHints()
    {
        QStringList rows;
        for (int i = 0; i < 20; ++i)
            rows.append(QString::number(i));
        QStringListModel model(rows);
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 10, 5));
        view.setModel(&model);

        view.scrollTo(model.index(12, 0));
        QCOMPARE(view.scrollOffset(), 8); // row 12 is the last line
        view.scrollTo(model.index(10, 0));
        QCOMPARE(view.scrollOffset(), 8); // already visible
        view.scrollTo(model.index(10, 0), QTuiAbstractItemView::PositionAtTop);
        QCOMPARE(view.scrollOffset(), 10);
        view.scrollTo(model.index(10, 0), QTuiAbstractItemView::PositionAtCenter);
        QCOMPARE(view.scrollOffset(), 8);
        view.scrollTo(model.index(19, 0), QTuiAbstractItemView::PositionAtTop);
        QCOMPARE(view.scrollOffset(), 15); // not past the end
        QVERIFY(view.visualRect(model.index(2, 0)).isEmpty());
        // edit() scrolls to the item
        QVERIFY(view.edit(model.index(1, 0)));
        QCOMPARE(view.scrollOffset(), 1);
    }

    void uiLoaderReadsAndWritesEditTriggers()
    {
        UI_Loader loader;
        QScopedPointer<QTuiWidget> widget(loader.loadFromData(
            R"({"type": "QTuiTableView", "editTriggers": "DoubleClicked|AnyKeyPressed"})"));
        auto *view = qobject_cast<QTuiTableView *>(widget.data());
        QVERIFY(view);
        QCOMPARE(view->editTriggers(),
                 QTuiAbstractItemView::DoubleClicked | QTuiAbstractItemView::AnyKeyPressed);
        const QJsonObject saved = QJsonDocument::fromJson(loader.serialize(view)).object();
        QCOMPARE(saved.value("editTriggers").toString(),
                 QStringLiteral("DoubleClicked|AnyKeyPressed"));
    }

    // Property-based test (RapidCheck): after scrollTo(), the item is visible
    // and its rectangle is the line at its row
    void propertyScrollToShowsTheItem()
    {
        rc::check("scrollTo makes visualRect non-empty", []() {
            const int count = *rc::gen::inRange(1, 60);
            const int height = *rc::gen::inRange(1, 12);
            const int row = *rc::gen::inRange(0, count);
            const auto hint = static_cast<QTuiAbstractItemView::ScrollHint>(*rc::gen::inRange(0, 4));
            QStringListModel model(QStringList(count, QStringLiteral("x")));
            QTuiListView view;
            view.setGeometry(QRect(0, 0, 8, height));
            view.setModel(&model);
            view.scrollTo(model.index(row, 0), hint);
            const QRect rect = view.visualRect(model.index(row, 0));
            RC_ASSERT(!rect.isEmpty());
            RC_ASSERT(rect.y() == row - view.scrollOffset());
        });
    }
};

QTEST_MAIN(TestEditing)
#include "test_editing.moc"
