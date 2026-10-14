#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/QTuiColumnView.h>
#include <QtTUI/QTuiUndoView.h>
#include <QtTUI/QTuiUndoStack.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QSignalSpy>

using namespace QtTUI;

// ---- Simple tree model (Qt6::Core only, no QStandardItemModel) ----

class SimpleTreeNode {
public:
    explicit SimpleTreeNode(const QString &text, SimpleTreeNode *parent = nullptr)
        : m_text(text), m_parent(parent)
    {
        if (m_parent)
            m_parent->m_children.append(this);
    }
    ~SimpleTreeNode() { qDeleteAll(m_children); }

    QString text() const { return m_text; }
    void setText(const QString &t) { m_text = t; }
    SimpleTreeNode *parent() const { return m_parent; }
    SimpleTreeNode *child(int row) const { return m_children.value(row); }
    int childCount() const { return m_children.size(); }
    int row() const {
        if (m_parent)
            return m_parent->m_children.indexOf(const_cast<SimpleTreeNode*>(this));
        return 0;
    }
    void appendChild(SimpleTreeNode *child) {
        child->m_parent = this;
        m_children.append(child);
    }
    void removeChildAt(int row) {
        if (row >= 0 && row < m_children.size())
            delete m_children.takeAt(row);
    }
    QList<SimpleTreeNode*> &children() { return m_children; }

private:
    QString m_text;
    SimpleTreeNode *m_parent = nullptr;
    QList<SimpleTreeNode*> m_children;
};

class SimpleTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit SimpleTreeModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent), m_root(new SimpleTreeNode(QString())) {}
    ~SimpleTreeModel() override { delete m_root; }

    SimpleTreeNode *rootNode() const { return m_root; }

    // Convenience: add a top-level item, returns the node
    SimpleTreeNode *addTopLevel(const QString &text) {
        int row = m_root->childCount();
        beginInsertRows(QModelIndex(), row, row);
        auto *node = new SimpleTreeNode(text, m_root);
        endInsertRows();
        return node;
    }

    // Convenience: add a child to an existing node
    SimpleTreeNode *addChild(SimpleTreeNode *parent, const QString &text) {
        QModelIndex parentIdx = indexForNode(parent);
        int row = parent->childCount();
        beginInsertRows(parentIdx, row, row);
        auto *node = new SimpleTreeNode(text, parent);
        endInsertRows();
        return node;
    }

    void removeTopLevel(int row) {
        if (row < 0 || row >= m_root->childCount()) return;
        beginRemoveRows(QModelIndex(), row, row);
        m_root->removeChildAt(row);
        endRemoveRows();
    }

    void clearAll() {
        beginResetModel();
        delete m_root;
        m_root = new SimpleTreeNode(QString());
        endResetModel();
    }

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override {
        if (!hasIndex(row, column, parent)) return QModelIndex();
        SimpleTreeNode *parentNode = parent.isValid()
            ? static_cast<SimpleTreeNode*>(parent.internalPointer()) : m_root;
        SimpleTreeNode *child = parentNode->child(row);
        return child ? createIndex(row, column, child) : QModelIndex();
    }

    QModelIndex parent(const QModelIndex &idx) const override {
        if (!idx.isValid()) return QModelIndex();
        auto *child = static_cast<SimpleTreeNode*>(idx.internalPointer());
        SimpleTreeNode *parentNode = child->parent();
        if (!parentNode || parentNode == m_root) return QModelIndex();
        return createIndex(parentNode->row(), 0, parentNode);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        SimpleTreeNode *node = parent.isValid()
            ? static_cast<SimpleTreeNode*>(parent.internalPointer()) : m_root;
        return node->childCount();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return 1; }

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override {
        if (!idx.isValid() || role != Qt::DisplayRole) return {};
        return static_cast<SimpleTreeNode*>(idx.internalPointer())->text();
    }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override {
        SimpleTreeNode *node = parent.isValid()
            ? static_cast<SimpleTreeNode*>(parent.internalPointer()) : m_root;
        return node->childCount() > 0;
    }

    QModelIndex indexForNode(SimpleTreeNode *node) const {
        if (!node || node == m_root) return QModelIndex();
        return createIndex(node->row(), 0, node);
    }

private:
    SimpleTreeNode *m_root;
};

// Helper: send a key event to a widget
static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

// Helper: build a simple two-level tree model
// Root0 -> Child0_0, Child0_1
// Root1 -> Child1_0
// Root2 (leaf)
static SimpleTreeModel *buildTreeModel(QObject *parent = nullptr)
{
    auto *model = new SimpleTreeModel(parent);
    auto *root0 = model->addTopLevel("Root0");
    model->addChild(root0, "Child0_0");
    model->addChild(root0, "Child0_1");

    auto *root1 = model->addTopLevel("Root1");
    model->addChild(root1, "Child1_0");

    model->addTopLevel("Root2");
    return model;
}

// Simple undo command for testing
class TestCommand : public QTuiUndoCommand {
public:
    TestCommand(const QString &text) : QTuiUndoCommand(text) {}
    void undo() override {}
    void redo() override {}
};

class TestTreeColumnUndoViews : public QObject {
    Q_OBJECT
private slots:

    // ========== QTuiTreeView Tests ==========

    void treeViewExpandCollapse()
    {
        rc::check("Expanding a node adds children to flat list; collapsing removes them", []() {
            SimpleTreeModel model;
            const int rootCount = *rc::gen::inRange(1, 6);
            const int childCount = *rc::gen::inRange(1, 5);

            for (int r = 0; r < rootCount; ++r) {
                auto *root = model.addTopLevel(QStringLiteral("Root%1").arg(r));
                for (int c = 0; c < childCount; ++c)
                    model.addChild(root, QStringLiteral("Child%1_%2").arg(r).arg(c));
            }

            QTuiTreeView tv;
            tv.setGeometry(QRect(0, 0, 40, 30));
            tv.setModel(&model);

            const int expandIdx = *rc::gen::inRange(0, rootCount);
            QModelIndex rootIdx = model.index(expandIdx, 0);

            RC_ASSERT(!tv.isExpanded(rootIdx));
            tv.expand(rootIdx);
            RC_ASSERT(tv.isExpanded(rootIdx));

            tv.setCurrentIndex(rootIdx);
            sendKey(tv, Qt::Key_Down);
            QModelIndex cur = tv.currentIndex();
            RC_ASSERT(cur.isValid());
            RC_ASSERT(cur.parent() == rootIdx);

            tv.collapse(rootIdx);
            RC_ASSERT(!tv.isExpanded(rootIdx));

            cur = tv.currentIndex();
            RC_ASSERT(cur.isValid());
        });
    }

    void treeViewModelSignalHandling()
    {
        SimpleTreeModel model;
        model.addTopLevel("Item0");
        model.addTopLevel("Item1");

        QTuiTreeView tv;
        tv.setGeometry(QRect(0, 0, 30, 20));
        tv.setModel(&model);

        model.addTopLevel("Item2");
        QModelIndex cur = tv.currentIndex();
        QVERIFY(cur.isValid());

        model.removeTopLevel(0);
        cur = tv.currentIndex();
        if (model.rowCount() > 0)
            QVERIFY(cur.isValid());

        model.clearAll();
        cur = tv.currentIndex();
        QVERIFY(!cur.isValid());
    }

    void treeViewKeyboardNavigation()
    {
        rc::check("Up/Down navigate, Right expands, Left collapses", []() {
            auto *model = buildTreeModel();
            QTuiTreeView tv;
            tv.setGeometry(QRect(0, 0, 40, 20));
            tv.setModel(model);

            QModelIndex root0 = model->index(0, 0);
            tv.setCurrentIndex(root0);
            RC_ASSERT(tv.currentIndex() == root0);

            sendKey(tv, Qt::Key_Right);
            RC_ASSERT(tv.isExpanded(root0));

            sendKey(tv, Qt::Key_Down);
            QModelIndex cur = tv.currentIndex();
            RC_ASSERT(cur.parent() == root0);
            RC_ASSERT(cur.row() == 0);

            sendKey(tv, Qt::Key_Left);
            RC_ASSERT(tv.currentIndex() == root0);

            sendKey(tv, Qt::Key_Left);
            RC_ASSERT(!tv.isExpanded(root0));

            sendKey(tv, Qt::Key_Down);
            RC_ASSERT(tv.currentIndex() == model->index(1, 0));

            sendKey(tv, Qt::Key_Up);
            RC_ASSERT(tv.currentIndex() == root0);

            delete model;
        });
    }

    void treeViewActivatedSignal()
    {
        rc::check("Enter emits activated with correct QModelIndex", []() {
            auto *model = buildTreeModel();
            QTuiTreeView tv;
            tv.setGeometry(QRect(0, 0, 40, 20));
            tv.setModel(model);

            const int row = *rc::gen::inRange(0, model->rowCount());
            QModelIndex idx = model->index(row, 0);
            tv.setCurrentIndex(idx);

            QSignalSpy spy(&tv, &QTuiTreeView::activated);
            sendKey(tv, Qt::Key_Return);

            RC_ASSERT(spy.count() == 1);
            QModelIndex emitted = spy.at(0).at(0).value<QModelIndex>();
            RC_ASSERT(emitted == idx);

            delete model;
        });
    }

    void treeViewScrolling()
    {
        rc::check("When items exceed widget height, scrolling keeps current visible", []() {
            SimpleTreeModel model;
            const int totalItems = *rc::gen::inRange(10, 25);
            for (int i = 0; i < totalItems; ++i)
                model.addTopLevel(QStringLiteral("Item%1").arg(i));

            const int viewHeight = *rc::gen::inRange(3, 8);
            QTuiTreeView tv;
            tv.setGeometry(QRect(0, 0, 30, viewHeight));
            tv.setModel(&model);

            for (int i = 0; i < totalItems; ++i) {
                tv.setCurrentIndex(model.index(i, 0));
                ScreenBuffer buf(30, viewHeight);
                tv.paintEvent(buf);
                RC_ASSERT(tv.currentIndex().row() == i);
            }
        });
    }

    // ========== QTuiColumnView Tests ==========

    void columnViewCascadingColumns()
    {
        rc::check("Selecting item with children creates child column", []() {
            auto *model = buildTreeModel();
            QTuiColumnView cv;
            cv.setGeometry(QRect(0, 0, 80, 20));
            cv.setModel(model);

            QModelIndex root0 = model->index(0, 0);

            sendKey(cv, Qt::Key_Right);
            QModelIndex cur = cv.currentIndex();
            RC_ASSERT(cur.isValid());
            RC_ASSERT(cur.parent() == root0);

            sendKey(cv, Qt::Key_Left);
            sendKey(cv, Qt::Key_Down);
            sendKey(cv, Qt::Key_Down);

            QModelIndex root2 = model->index(2, 0);
            RC_ASSERT(cv.currentIndex() == root2);
            sendKey(cv, Qt::Key_Right);
            RC_ASSERT(cv.currentIndex() == root2);

            delete model;
        });
    }

    void columnViewNavigation()
    {
        auto *model = buildTreeModel();
        QTuiColumnView cv;
        cv.setGeometry(QRect(0, 0, 80, 20));
        cv.setModel(model);

        QModelIndex root0 = model->index(0, 0);
        QCOMPARE(cv.currentIndex(), root0);

        sendKey(cv, Qt::Key_Right);
        QModelIndex cur = cv.currentIndex();
        QVERIFY(cur.isValid());
        QCOMPARE(cur.parent(), root0);

        sendKey(cv, Qt::Key_Left);
        cur = cv.currentIndex();
        QVERIFY(cur.isValid());
        QCOMPARE(cur, root0);

        sendKey(cv, Qt::Key_Down);
        cur = cv.currentIndex();
        QVERIFY(cur.isValid());
        QCOMPARE(cur, model->index(1, 0));

        sendKey(cv, Qt::Key_Up);
        cur = cv.currentIndex();
        QCOMPARE(cur, root0);

        delete model;
    }

    void columnViewClickMakesTheItemCurrent()
    {
        auto *model = buildTreeModel();
        QTuiColumnView cv;
        cv.setGeometry(QRect(0, 0, 80, 20));
        cv.setModel(model);
        auto click = [&cv](int x, int y) {
            TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, QPoint(x, y));
            QCoreApplication::sendEvent(&cv, &ev);
        };

        // The second column starts after the first column (20) and a separator
        click(22, 1);
        QCOMPARE(cv.currentIndex(), model->index(1, 0, model->index(0, 0)));

        // A click in the first column shows the children of that item
        click(1, 1);
        QCOMPARE(cv.currentIndex(), model->index(1, 0));
        QCOMPARE(cv.visualRect(model->index(0, 0, model->index(1, 0))), QRect(21, 0, 20, 1));

        // A click below the last item does nothing
        click(1, 5);
        QCOMPARE(cv.currentIndex(), model->index(1, 0));
        delete model;
    }

    void columnViewModelReset()
    {
        SimpleTreeModel model;
        auto *root = model.addTopLevel("Root");
        model.addChild(root, "Child");

        QTuiColumnView cv;
        cv.setGeometry(QRect(0, 0, 60, 10));
        cv.setModel(&model);

        model.clearAll();
        QModelIndex cur = cv.currentIndex();
        QVERIFY(!cur.isValid());
    }

    void columnViewActivatedSignal()
    {
        rc::check("Enter emits activated with correct QModelIndex", []() {
            auto *model = buildTreeModel();
            QTuiColumnView cv;
            cv.setGeometry(QRect(0, 0, 80, 20));
            cv.setModel(model);

            const int row = *rc::gen::inRange(0, model->rowCount());
            QModelIndex idx = model->index(row, 0);
            cv.setCurrentIndex(idx);

            QSignalSpy spy(&cv, &QTuiColumnView::activated);
            sendKey(cv, Qt::Key_Return);

            RC_ASSERT(spy.count() == 1);
            QModelIndex emitted = spy.at(0).at(0).value<QModelIndex>();
            RC_ASSERT(emitted == idx);

            delete model;
        });
    }

    // ========== QTuiUndoView Tests ==========

    void undoViewStackChanges()
    {
        rc::check("Pushing commands updates the view; stack changes keep index valid", []() {
            QTuiUndoStack stack;
            QTuiUndoView uv;
            uv.setGeometry(QRect(0, 0, 30, 10));
            uv.setStack(&stack);

            const int cmdCount = *rc::gen::inRange(1, 8);
            for (int i = 0; i < cmdCount; ++i)
                stack.push(new TestCommand(QStringLiteral("Cmd%1").arg(i)));

            ScreenBuffer buf(30, 10);
            uv.paintEvent(buf);
            RC_ASSERT(stack.count() == cmdCount);
        });
    }

    void undoViewVisualDistinction()
    {
        QTuiUndoStack stack;
        stack.push(new TestCommand("Cmd0"));
        stack.push(new TestCommand("Cmd1"));
        stack.push(new TestCommand("Cmd2"));

        stack.undo();

        QTuiUndoView uv;
        uv.setGeometry(QRect(0, 0, 30, 5));
        uv.setStack(&stack);

        ScreenBuffer buf(30, 5);
        uv.paintEvent(buf);

        QCOMPARE(stack.index(), 2);
        QCOMPARE(stack.count(), 3);

        Cell &activeCell = buf.backCell(0, 0);
        Cell &undoneCell = buf.backCell(0, 2);
        QVERIFY(activeCell.foreground != undoneCell.foreground
                || activeCell.background != undoneCell.background);
    }

    void undoViewSelectCommand()
    {
        QTuiUndoStack stack;
        stack.push(new TestCommand("Cmd0"));
        stack.push(new TestCommand("Cmd1"));
        stack.push(new TestCommand("Cmd2"));

        QTuiUndoView uv;
        uv.setGeometry(QRect(0, 0, 30, 5));
        uv.setStack(&stack);

        sendKey(uv, Qt::Key_Up);
        sendKey(uv, Qt::Key_Up);

        sendKey(uv, Qt::Key_Return);
        QCOMPARE(stack.index(), 1);
    }

    void undoViewEmptyRowUndoesAllAndFollowsTheStack()
    {
        QTuiUndoStack stack;
        stack.push(nullptr); // ignored
        stack.push(new TestCommand("Cmd0"));
        stack.push(new TestCommand("Cmd1"));
        QCOMPARE(stack.count(), 2);

        QTuiUndoView uv;
        uv.setGeometry(QRect(0, 0, 30, 5));
        uv.setStack(&stack);

        // Row 0 is "<empty>": Enter on it undoes all commands
        sendKey(uv, Qt::Key_Up);
        sendKey(uv, Qt::Key_Up);
        sendKey(uv, Qt::Key_Return);
        QCOMPARE(stack.index(), 0);

        // The current row follows the stack: Down from row 1 goes to row 2
        stack.redo();
        sendKey(uv, Qt::Key_Down);
        sendKey(uv, Qt::Key_Return);
        QCOMPARE(stack.index(), 2);

        ScreenBuffer buf(30, 5);
        uv.paintEvent(buf);
        QCOMPARE(buf.backCell(0, 0).ch, U'<');
    }

    void undoViewNavigation()
    {
        rc::check("Up/Down navigate through command list", []() {
            QTuiUndoStack stack;
            const int cmdCount = *rc::gen::inRange(2, 8);
            for (int i = 0; i < cmdCount; ++i)
                stack.push(new TestCommand(QStringLiteral("Cmd%1").arg(i)));

            QTuiUndoView uv;
            uv.setGeometry(QRect(0, 0, 30, cmdCount + 2));
            uv.setStack(&stack);

            for (int i = 0; i < cmdCount - 1; ++i)
                sendKey(uv, Qt::Key_Up);

            for (int i = 0; i < cmdCount - 1; ++i)
                sendKey(uv, Qt::Key_Down);

            sendKey(uv, Qt::Key_Return);
            RC_ASSERT(stack.index() == cmdCount);
        });
    }
};

QTEST_MAIN(TestTreeColumnUndoViews)
#include "test_qtui_treeview_columnview_undoview.moc"
