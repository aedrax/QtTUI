#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/RenderScheduler.h>
#include <QAbstractTableModel>
#include <QAbstractListModel>
#include <QAbstractItemModel>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// Simple table model (Qt6::Core only, no QStandardItemModel)
// ---------------------------------------------------------------------------
class TestTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    TestTableModel(int rows, int cols, QObject *parent = nullptr)
        : QAbstractTableModel(parent), m_cols(cols)
    {
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r) {
            m_data[r].resize(cols);
            for (int c = 0; c < cols; ++c)
                m_data[r][c] = QStringLiteral("R%1C%2").arg(r).arg(c);
        }
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = QModelIndex()) const override { return m_cols; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || !index.isValid())
            return {};
        if (index.row() < 0 || index.row() >= rowCount()
            || index.column() < 0 || index.column() >= columnCount())
            return {};
        return m_data[index.row()][index.column()];
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};
        return QStringLiteral("H%1").arg(section);
    }

    void emitDataChanged(int topRow, int bottomRow, int leftCol, int rightCol)
    {
        emit dataChanged(index(topRow, leftCol), index(bottomRow, rightCol));
    }

    void insertRowsAt(int position, int count)
    {
        position = qBound(0, position, rowCount());
        beginInsertRows(QModelIndex(), position, position + count - 1);
        for (int i = 0; i < count; ++i) {
            QVector<QString> row(m_cols);
            for (int c = 0; c < m_cols; ++c)
                row[c] = QStringLiteral("New%1C%2").arg(position + i).arg(c);
            m_data.insert(position + i, row);
        }
        endInsertRows();
    }

    void removeRowsAt(int position, int count)
    {
        count = qMin(count, rowCount() - position);
        if (count <= 0)
            return;
        beginRemoveRows(QModelIndex(), position, position + count - 1);
        m_data.remove(position, count);
        endRemoveRows();
    }

    void doReset()
    {
        beginResetModel();
        m_data.clear();
        endResetModel();
    }

private:
    QVector<QVector<QString>> m_data;
    int m_cols;
};

// ---------------------------------------------------------------------------
// Simple list model (Qt6::Core only)
// ---------------------------------------------------------------------------
class TestListModel : public QAbstractListModel {
    Q_OBJECT
public:
    TestListModel(int rows, QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r)
            m_data[r] = QStringLiteral("Item%1").arg(r);
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_data.size(); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || !index.isValid())
            return {};
        if (index.row() < 0 || index.row() >= rowCount())
            return {};
        return m_data[index.row()];
    }

    void emitDataChanged(int topRow, int bottomRow)
    {
        emit dataChanged(index(topRow, 0), index(bottomRow, 0));
    }

    void insertRowsAt(int position, int count)
    {
        position = qBound(0, position, rowCount());
        beginInsertRows(QModelIndex(), position, position + count - 1);
        for (int i = 0; i < count; ++i)
            m_data.insert(position + i, QStringLiteral("New%1").arg(position + i));
        endInsertRows();
    }

    void removeRowsAt(int position, int count)
    {
        count = qMin(count, rowCount() - position);
        if (count <= 0)
            return;
        beginRemoveRows(QModelIndex(), position, position + count - 1);
        m_data.remove(position, count);
        endRemoveRows();
    }

    void doReset()
    {
        beginResetModel();
        m_data.clear();
        endResetModel();
    }

private:
    QVector<QString> m_data;
};

// ---------------------------------------------------------------------------
// Simple tree model (Qt6::Core only) — flat tree (all items at root level)
// ---------------------------------------------------------------------------
class TreeNode {
public:
    explicit TreeNode(const QString &text, TreeNode *parent = nullptr)
        : m_text(text), m_parent(parent)
    {
        if (m_parent)
            m_parent->m_children.append(this);
    }
    ~TreeNode() { qDeleteAll(m_children); }

    QString text() const { return m_text; }
    TreeNode *parent() const { return m_parent; }
    TreeNode *child(int row) const { return (row >= 0 && row < m_children.size()) ? m_children.at(row) : nullptr; }
    int childCount() const { return m_children.size(); }
    int row() const {
        if (m_parent)
            return m_parent->m_children.indexOf(const_cast<TreeNode*>(this));
        return 0;
    }
    void removeChildAt(int row) {
        if (row >= 0 && row < m_children.size())
            delete m_children.takeAt(row);
    }

private:
    QString m_text;
    TreeNode *m_parent = nullptr;
    QList<TreeNode*> m_children;
};

class TestTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit TestTreeModel(int topLevelCount, QObject *parent = nullptr)
        : QAbstractItemModel(parent), m_root(new TreeNode(QString()))
    {
        for (int i = 0; i < topLevelCount; ++i)
            new TreeNode(QStringLiteral("Node%1").arg(i), m_root);
    }
    ~TestTreeModel() override { delete m_root; }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();
        TreeNode *parentNode = parent.isValid()
            ? static_cast<TreeNode*>(parent.internalPointer()) : m_root;
        TreeNode *child = parentNode->child(row);
        return child ? createIndex(row, column, child) : QModelIndex();
    }

    QModelIndex parent(const QModelIndex &idx) const override
    {
        if (!idx.isValid())
            return QModelIndex();
        auto *node = static_cast<TreeNode*>(idx.internalPointer());
        TreeNode *parentNode = node->parent();
        if (!parentNode || parentNode == m_root)
            return QModelIndex();
        return createIndex(parentNode->row(), 0, parentNode);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        TreeNode *node = parent.isValid()
            ? static_cast<TreeNode*>(parent.internalPointer()) : m_root;
        return node->childCount();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return 1; }

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override
    {
        if (!idx.isValid() || role != Qt::DisplayRole)
            return {};
        return static_cast<TreeNode*>(idx.internalPointer())->text();
    }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override
    {
        TreeNode *node = parent.isValid()
            ? static_cast<TreeNode*>(parent.internalPointer()) : m_root;
        return node->childCount() > 0;
    }

    void insertTopLevelAt(int position)
    {
        position = qBound(0, position, m_root->childCount());
        beginInsertRows(QModelIndex(), position, position);
        new TreeNode(QStringLiteral("Inserted%1").arg(position), m_root);
        endInsertRows();
    }

    void removeTopLevelAt(int position)
    {
        if (position < 0 || position >= m_root->childCount())
            return;
        beginRemoveRows(QModelIndex(), position, position);
        m_root->removeChildAt(position);
        endRemoveRows();
    }

    void emitDataChanged(int topRow, int bottomRow)
    {
        emit dataChanged(index(topRow, 0), index(bottomRow, 0));
    }

    void doReset()
    {
        beginResetModel();
        delete m_root;
        m_root = new TreeNode(QString());
        endResetModel();
    }

private:
    TreeNode *m_root;
};

// ---------------------------------------------------------------------------
// Test helper: create a QTuiApplication + MainWindow environment
// ---------------------------------------------------------------------------
struct TestEnv {
    int argc = 1;
    char arg0[5] = "test";
    char *argv[2] = { arg0, nullptr };
    QTuiApplication app;
    QTuiMainWindow mainWindow;

    TestEnv() : app(argc, argv)
    {
        mainWindow.setGeometry(QRect(0, 0, 80, 24));
        mainWindow.setVisible(true);
        app.setMainWindow(&mainWindow);
        drain();
    }

    void drain()
    {
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }
};

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestModelViewDirty : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 9: Model dataChanged Precise Dirtying
    // -----------------------------------------------------------------------
    void dataChangedPreciseDirtyingTableView()
    {
        rc::check("TableView dataChanged marks only visible changed rows as dirty",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 51);
                const int colCount = *rc::gen::inRange(1, 6);

                TestTableModel model(rowCount, colCount);

                QTuiTableView tableView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 20);
                tableView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                // Generate random changed range
                const int changedTop = *rc::gen::inRange(0, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom, 0, colCount - 1);

                // Determine expected behavior
                const int scrollOffset = tableView.scrollOffset();
                const int visibleDataRows = viewHeight - 1; // header takes 1 row
                const int firstVisibleRow = scrollOffset;
                const int lastVisibleRow = scrollOffset + visibleDataRows - 1;

                if (changedTop > lastVisibleRow || changedBottom < firstVisibleRow) {
                    // Entirely outside viewport: no dirty marking
                    RC_ASSERT(!tableView.isDirty());
                } else {
                    // At least partially visible: should be dirty
                    RC_ASSERT(tableView.isDirty());

                    // Verify dirty region is within the widget's geometry
                    QRect dirtyRegion = tableView.dirtyRegion();
                    QRect geo = tableView.geometry();
                    RC_ASSERT(geo.contains(dirtyRegion));
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 9: Model dataChanged for ListView
    // -----------------------------------------------------------------------
    void dataChangedPreciseDirtyingListView()
    {
        rc::check("ListView dataChanged marks only visible changed rows as dirty",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 51);
                TestListModel model(rowCount);

                QTuiListView listView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 20);
                listView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                listView.setModel(&model);

                env.drain();
                listView.clearDirtyRegions();
                env.drain();

                // Generate random changed range
                const int changedTop = *rc::gen::inRange(0, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom);

                const int scrollOffset = listView.scrollOffset();
                const int firstVisibleRow = scrollOffset;
                const int lastVisibleRow = scrollOffset + viewHeight - 1;

                if (changedTop > lastVisibleRow || changedBottom < firstVisibleRow) {
                    RC_ASSERT(!listView.isDirty());
                } else {
                    RC_ASSERT(listView.isDirty());
                    QRect dirtyRegion = listView.dirtyRegion();
                    QRect geo = listView.geometry();
                    RC_ASSERT(geo.contains(dirtyRegion));
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 9: Model dataChanged for TreeView
    // -----------------------------------------------------------------------
    void dataChangedPreciseDirtyingTreeView()
    {
        rc::check("TreeView dataChanged marks only visible changed rows as dirty",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 30);
                TestTreeModel model(rowCount);

                QTuiTreeView treeView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 15);
                treeView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                treeView.setModel(&model);

                env.drain();
                treeView.clearDirtyRegions();
                env.drain();

                // Generate random changed range within root items
                const int changedTop = *rc::gen::inRange(0, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom);

                // For a flat tree (no scroll), flat rows == model rows
                const int firstVisibleRow = 0;
                const int lastVisibleRow = viewHeight - 1;

                if (changedTop > lastVisibleRow || changedBottom < firstVisibleRow) {
                    RC_ASSERT(!treeView.isDirty());
                } else {
                    RC_ASSERT(treeView.isDirty());
                    QRect dirtyRegion = treeView.dirtyRegion();
                    QRect geo = treeView.geometry();
                    RC_ASSERT(geo.contains(dirtyRegion));
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 10: Model Row Insert/Remove Dirtying
    // -----------------------------------------------------------------------
    void rowInsertRemoveDirtyingTableView()
    {
        rc::check("TableView rowsInserted/rowsRemoved marks from affected position to bottom",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 40);
                TestTableModel model(rowCount, 2);

                QTuiTableView tableView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 15);
                tableView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                const bool doInsert = *rc::gen::arbitrary<bool>();
                const int position = *rc::gen::inRange(0, rowCount);

                if (doInsert) {
                    model.insertRowsAt(position, 1);
                } else {
                    model.removeRowsAt(position, 1);
                }

                const int scrollOffset = tableView.scrollOffset();
                const int visibleDataRows = viewHeight - 1; // header row
                const int lastVisibleRow = scrollOffset + visibleDataRows - 1;

                if (position > lastVisibleRow) {
                    // Entirely below viewport: no dirty marking
                    RC_ASSERT(!tableView.isDirty());
                } else {
                    // Should be dirty from affected position to bottom
                    RC_ASSERT(tableView.isDirty());
                    QRect dirtyRegion = tableView.dirtyRegion();
                    QRect geo = tableView.geometry();

                    // Dirty region should extend to the bottom of the view area
                    RC_ASSERT(dirtyRegion.bottom() >= geo.bottom() - 1);
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 10: Row Insert/Remove for ListView
    // -----------------------------------------------------------------------
    void rowInsertRemoveDirtyingListView()
    {
        rc::check("ListView rowsInserted/rowsRemoved marks from affected position to bottom",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 40);
                TestListModel model(rowCount);

                QTuiListView listView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 15);
                listView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                listView.setModel(&model);

                env.drain();
                listView.clearDirtyRegions();
                env.drain();

                const bool doInsert = *rc::gen::arbitrary<bool>();
                const int position = *rc::gen::inRange(0, rowCount);

                if (doInsert) {
                    model.insertRowsAt(position, 1);
                } else {
                    model.removeRowsAt(position, 1);
                }

                const int scrollOffset = listView.scrollOffset();
                const int lastVisibleRow = scrollOffset + viewHeight - 1;

                if (position > lastVisibleRow) {
                    RC_ASSERT(!listView.isDirty());
                } else {
                    RC_ASSERT(listView.isDirty());
                    QRect dirtyRegion = listView.dirtyRegion();
                    QRect geo = listView.geometry();
                    // Dirty region should extend to the bottom of the view
                    RC_ASSERT(dirtyRegion.bottom() >= geo.bottom() - 1);
                }
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 11: Model Reset Full Dirty
    // -----------------------------------------------------------------------
    void modelResetFullDirtyTableView()
    {
        rc::check("TableView modelReset marks entire widget geometry as dirty",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(1, 30);
                TestTableModel model(rowCount, 2);

                QTuiTableView tableView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(10, 60);
                const int viewHeight = *rc::gen::inRange(5, 20);
                QRect geo(0, 0, viewWidth, viewHeight);
                tableView.setGeometry(geo);
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                model.doReset();

                RC_ASSERT(tableView.isDirty());
                QRect dirtyRegion = tableView.dirtyRegion();
                RC_ASSERT(dirtyRegion.contains(geo));
            });
    }

    void modelResetFullDirtyListView()
    {
        rc::check("ListView modelReset marks entire widget geometry as dirty",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(1, 30);
                TestListModel model(rowCount);

                QTuiListView listView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(10, 60);
                const int viewHeight = *rc::gen::inRange(5, 20);
                QRect geo(0, 0, viewWidth, viewHeight);
                listView.setGeometry(geo);
                listView.setModel(&model);

                env.drain();
                listView.clearDirtyRegions();
                env.drain();

                model.doReset();

                RC_ASSERT(listView.isDirty());
                QRect dirtyRegion = listView.dirtyRegion();
                RC_ASSERT(dirtyRegion.contains(geo));
            });
    }

    void modelResetFullDirtyTreeView()
    {
        rc::check("TreeView modelReset marks entire widget geometry as dirty",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(1, 20);
                TestTreeModel model(rowCount);

                QTuiTreeView treeView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(10, 60);
                const int viewHeight = *rc::gen::inRange(5, 20);
                QRect geo(0, 0, viewWidth, viewHeight);
                treeView.setGeometry(geo);
                treeView.setModel(&model);

                env.drain();
                treeView.clearDirtyRegions();
                env.drain();

                model.doReset();

                RC_ASSERT(treeView.isDirty());
                QRect dirtyRegion = treeView.dirtyRegion();
                RC_ASSERT(dirtyRegion.contains(geo));
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 12: Model Signal Viewport Filtering
    // -----------------------------------------------------------------------
    void viewportFilteringTableView()
    {
        rc::check("TableView signals for indices outside viewport produce no dirty regions",
            []() {
                TestEnv env;

                // Create a model with enough rows to have out-of-viewport indices
                const int viewHeight = *rc::gen::inRange(5, 10);
                const int rowCount = viewHeight + *rc::gen::inRange(5, 30);
                TestTableModel model(rowCount, 2);

                QTuiTableView tableView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                tableView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                // The visible viewport is rows [scrollOffset, scrollOffset + viewHeight - 2]
                const int scrollOffset = tableView.scrollOffset();
                const int visibleDataRows = viewHeight - 1;
                const int lastVisibleRow = scrollOffset + visibleDataRows - 1;

                // Generate a dataChanged signal entirely BELOW the viewport
                RC_PRE(lastVisibleRow + 1 < rowCount);
                const int changedTop = *rc::gen::inRange(lastVisibleRow + 1, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom, 0, 1);

                RC_ASSERT(!tableView.isDirty());
            });
    }

    void viewportFilteringListView()
    {
        rc::check("ListView signals for indices outside viewport produce no dirty regions",
            []() {
                TestEnv env;

                const int viewHeight = *rc::gen::inRange(5, 10);
                const int rowCount = viewHeight + *rc::gen::inRange(5, 30);
                TestListModel model(rowCount);

                QTuiListView listView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                listView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                listView.setModel(&model);

                env.drain();
                listView.clearDirtyRegions();
                env.drain();

                const int scrollOffset = listView.scrollOffset();
                const int lastVisibleRow = scrollOffset + viewHeight - 1;

                // Generate a dataChanged signal entirely BELOW the viewport
                RC_PRE(lastVisibleRow + 1 < rowCount);
                const int changedTop = *rc::gen::inRange(lastVisibleRow + 1, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom);

                RC_ASSERT(!listView.isDirty());
            });
    }

    void viewportFilteringTreeView()
    {
        rc::check("TreeView signals for indices outside viewport produce no dirty regions",
            []() {
                TestEnv env;

                const int viewHeight = *rc::gen::inRange(5, 10);
                const int rowCount = viewHeight + *rc::gen::inRange(5, 20);
                TestTreeModel model(rowCount);

                QTuiTreeView treeView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                treeView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                treeView.setModel(&model);

                env.drain();
                treeView.clearDirtyRegions();
                env.drain();

                // For a flat tree with no scroll, visible rows are [0, viewHeight-1]
                // Generate a dataChanged signal entirely BELOW the viewport
                const int lastVisibleRow = viewHeight - 1;
                RC_PRE(lastVisibleRow + 1 < rowCount);
                const int changedTop = *rc::gen::inRange(lastVisibleRow + 1, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom);

                RC_ASSERT(!treeView.isDirty());
            });
    }

    // -----------------------------------------------------------------------
    // Unit test: TableView insert below viewport does not dirty
    // -----------------------------------------------------------------------
    void tableViewInsertBelowViewport()
    {
        TestEnv env;

        TestTableModel model(20, 2);

        QTuiTableView tableView(&env.mainWindow);
        tableView.setGeometry(QRect(0, 0, 40, 6)); // 5 visible data rows (1 header)
        tableView.setModel(&model);

        env.drain();
        tableView.clearDirtyRegions();
        env.drain();

        // Insert at row 10, which is below the visible viewport (rows 0-4)
        model.insertRowsAt(10, 1);

        QVERIFY(!tableView.isDirty());
    }

    // -----------------------------------------------------------------------
    // Unit test: ListView modelReset marks full geometry dirty
    // -----------------------------------------------------------------------
    void listViewModelResetFullDirty()
    {
        TestEnv env;

        TestListModel model(10);

        QTuiListView listView(&env.mainWindow);
        listView.setGeometry(QRect(5, 3, 30, 8));
        listView.setModel(&model);

        env.drain();
        listView.clearDirtyRegions();
        env.drain();

        model.doReset();

        QVERIFY(listView.isDirty());
        QRect dirtyRegion = listView.dirtyRegion();
        QVERIFY(dirtyRegion.contains(listView.geometry()));
    }

    // -----------------------------------------------------------------------
    // Unit test: TreeView onRowsInserted marks dirty
    // -----------------------------------------------------------------------
    void treeViewRowsInsertedDirty()
    {
        TestEnv env;

        TestTreeModel model(10);

        QTuiTreeView treeView(&env.mainWindow);
        treeView.setGeometry(QRect(0, 0, 40, 8));
        treeView.setModel(&model);

        env.drain();
        treeView.clearDirtyRegions();
        env.drain();

        // Insert at position 2 (within viewport)
        model.insertTopLevelAt(2);

        QVERIFY(treeView.isDirty());
    }

    // -----------------------------------------------------------------------
    // Unit test: TreeView onRowsRemoved marks dirty
    // -----------------------------------------------------------------------
    void treeViewRowsRemovedDirty()
    {
        TestEnv env;

        TestTreeModel model(10);

        QTuiTreeView treeView(&env.mainWindow);
        treeView.setGeometry(QRect(0, 0, 40, 8));
        treeView.setModel(&model);

        env.drain();
        treeView.clearDirtyRegions();
        env.drain();

        // Remove row 3 (within viewport)
        model.removeTopLevelAt(3);

        QVERIFY(treeView.isDirty());
    }
};

QTEST_APPLESS_MAIN(TestModelViewDirty)
#include "test_model_view_dirty.moc"
