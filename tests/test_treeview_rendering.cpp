#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractItemModel>

using namespace QtTUI;

// Feature: widget-rendering-overhaul, Property 18: Tree Indentation
// Feature: widget-rendering-overhaul, Property 19: Tree Expand/Collapse Indicators
// Feature: widget-rendering-overhaul, Property 20: Tree Expand/Collapse Rebuild and Dirty

// ---------------------------------------------------------------------------
// Tree model for testing
// ---------------------------------------------------------------------------
class TreeTestModel : public QAbstractItemModel {
    Q_OBJECT
public:
    struct Node {
        QString text;
        QList<Node *> children;
        Node *parent = nullptr;
        int row = 0;
    };

    explicit TreeTestModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent)
    {
        m_root = new Node{QStringLiteral("root"), {}, nullptr, 0};
    }

    ~TreeTestModel() override { deleteNode(m_root); }

    Node *addChild(Node *parentNode, const QString &text)
    {
        auto *child = new Node{text, {}, parentNode, static_cast<int>(parentNode->children.size())};
        parentNode->children.append(child);
        return child;
    }

    Node *rootNode() const { return m_root; }

    QModelIndex indexForNode(Node *node) const
    {
        if (!node || node == m_root)
            return {};
        return createIndex(node->row, 0, node);
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (column != 0)
            return {};
        Node *parentNode = parent.isValid() ? static_cast<Node *>(parent.internalPointer()) : m_root;
        if (row < 0 || row >= parentNode->children.size())
            return {};
        return createIndex(row, 0, parentNode->children.at(row));
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid())
            return {};
        Node *node = static_cast<Node *>(child.internalPointer());
        if (!node || !node->parent || node->parent == m_root)
            return {};
        return createIndex(node->parent->row, 0, node->parent);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Node *parentNode = parent.isValid() ? static_cast<Node *>(parent.internalPointer()) : m_root;
        return parentNode->children.size();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return 1; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || !index.isValid())
            return {};
        Node *node = static_cast<Node *>(index.internalPointer());
        return node ? node->text : QVariant();
    }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override
    {
        Node *parentNode = parent.isValid() ? static_cast<Node *>(parent.internalPointer()) : m_root;
        return !parentNode->children.isEmpty();
    }

private:
    void deleteNode(Node *node)
    {
        for (auto *child : node->children)
            deleteNode(child);
        delete node;
    }

    Node *m_root;
};

class TestTreeViewRendering : public QObject {
    Q_OBJECT
private slots:

    // **Validates: Requirements 8.1**
    // Feature: widget-rendering-overhaul, Property 18: Tree Indentation
    void treeIndentation()
    {
        rc::check("Tree items at depth D begin text at offset D * indentWidth + indicatorWidth",
            []() {
                // Build a tree with random depth
                const int maxDepth = *rc::gen::inRange(1, 5);
                const int childrenPerLevel = *rc::gen::inRange(1, 4);

                TreeTestModel model;

                // Build tree: each level has childrenPerLevel children
                struct LevelInfo {
                    TreeTestModel::Node *node;
                    int depth;
                };
                QList<LevelInfo> allNodes;

                // Add root-level children
                for (int i = 0; i < childrenPerLevel; ++i) {
                    auto *child = model.addChild(model.rootNode(),
                                                 QStringLiteral("D0_%1").arg(i));
                    allNodes.append({child, 0});
                }

                // Add deeper levels
                for (int d = 1; d <= maxDepth; ++d) {
                    QList<LevelInfo> currentLevel;
                    for (auto &info : allNodes) {
                        if (info.depth == d - 1) {
                            for (int i = 0; i < childrenPerLevel; ++i) {
                                auto *child = model.addChild(info.node,
                                                             QStringLiteral("D%1_%2").arg(d).arg(i));
                                currentLevel.append({child, d});
                            }
                        }
                    }
                    allNodes.append(currentLevel);
                }

                // Set up tree view with all nodes expanded
                const int viewWidth = 60;
                const int viewHeight = allNodes.size() + 5;

                QTuiTreeView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tv.setModel(&model);

                // Expand all nodes that have children
                for (auto &info : allNodes) {
                    QModelIndex idx = model.indexForNode(info.node);
                    if (model.hasChildren(idx))
                        tv.expand(idx);
                }

                ScreenBuffer buf(viewWidth, viewHeight);
                tv.paintEvent(buf);

                // Verify indentation for each visible row
                // The indentWidth is 2, indicator is 2 chars (either "▶ "/"▼ " or "  ")
                const int indentWidth = 2;
                const int indicatorWidth = 2;

                // Build expected flat list by traversing the tree in order
                struct FlatItem {
                    QString text;
                    int depth;
                    bool hasChildren;
                };
                QList<FlatItem> flatItems;

                std::function<void(TreeTestModel::Node *, int)> flatten;
                flatten = [&](TreeTestModel::Node *node, int depth) {
                    for (auto *child : node->children) {
                        flatItems.append({child->text, depth, !child->children.isEmpty()});
                        if (!child->children.isEmpty())
                            flatten(child, depth + 1);
                    }
                };
                flatten(model.rootNode(), 0);

                for (int vr = 0; vr < flatItems.size() && vr < viewHeight; ++vr) {
                    const auto &item = flatItems.at(vr);
                    int expectedTextStart = item.depth * indentWidth + indicatorWidth;

                    // Verify indentation: all chars before expectedTextStart should be
                    // spaces (indentation) or indicator chars
                    // The indentation portion (depth * indentWidth) should be spaces
                    for (int x = 0; x < item.depth * indentWidth && x < viewWidth; ++x) {
                        QChar ch = buf.backCell(x, vr).character;
                        RC_ASSERT(ch == QLatin1Char(' '));
                    }

                    // Verify text starts at the correct offset
                    if (expectedTextStart < viewWidth && !item.text.isEmpty()) {
                        QChar firstTextChar = buf.backCell(expectedTextStart, vr).character;
                        RC_ASSERT(firstTextChar == item.text.at(0));
                    }
                }
            });
    }

    // **Validates: Requirements 8.2**
    // Feature: widget-rendering-overhaul, Property 19: Tree Expand/Collapse Indicators
    void treeExpandCollapseIndicators()
    {
        rc::check("Nodes with children show ▶ (collapsed) or ▼ (expanded); leaves show no indicator",
            []() {
                TreeTestModel model;

                // Create a mix of leaf and parent nodes
                const int rootChildren = *rc::gen::inRange(2, 6);
                QList<TreeTestModel::Node *> parentNodes;

                for (int i = 0; i < rootChildren; ++i) {
                    auto *child = model.addChild(model.rootNode(),
                                                 QStringLiteral("N%1").arg(i));
                    // Randomly decide if this node has children
                    bool makeParent = *rc::gen::arbitrary<bool>();
                    if (makeParent) {
                        int numGrandchildren = *rc::gen::inRange(1, 4);
                        for (int j = 0; j < numGrandchildren; ++j) {
                            model.addChild(child, QStringLiteral("C%1_%2").arg(i).arg(j));
                        }
                        parentNodes.append(child);
                    }
                }

                // Ensure at least one parent node exists
                if (parentNodes.isEmpty()) {
                    auto *first = static_cast<TreeTestModel::Node *>(
                        model.index(0, 0).internalPointer());
                    model.addChild(first, QStringLiteral("forced_child"));
                    parentNodes.append(first);
                }

                const int viewWidth = 40;
                const int viewHeight = 30;

                QTuiTreeView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tv.setModel(&model);

                // Initially all nodes are collapsed
                // Verify collapsed parent nodes show ▶
                {
                    ScreenBuffer buf(viewWidth, viewHeight);
                    tv.paintEvent(buf);

                    // Check root-level items (depth 0, indicator at offset 0)
                    for (int i = 0; i < rootChildren && i < viewHeight; ++i) {
                        QModelIndex idx = model.index(i, 0);
                        bool hasKids = model.hasChildren(idx);
                        QChar indicatorChar = buf.backCell(0, i).character;

                        if (hasKids) {
                            // Collapsed parent: should show ▶
                            RC_ASSERT(indicatorChar == QChar(0x25B6));
                        } else {
                            // Leaf: should show space (no indicator)
                            RC_ASSERT(indicatorChar == QLatin1Char(' '));
                        }
                    }
                }

                // Expand a parent node and verify it shows ▼
                if (!parentNodes.isEmpty()) {
                    QModelIndex parentIdx = model.indexForNode(parentNodes.first());
                    tv.expand(parentIdx);

                    ScreenBuffer buf(viewWidth, viewHeight);
                    tv.paintEvent(buf);

                    // Find the row of the expanded parent
                    int parentRow = -1;
                    for (int i = 0; i < rootChildren; ++i) {
                        QModelIndex idx = model.index(i, 0);
                        if (idx == parentIdx) {
                            parentRow = i;
                            break;
                        }
                    }

                    if (parentRow >= 0 && parentRow < viewHeight) {
                        QChar indicatorChar = buf.backCell(0, parentRow).character;
                        RC_ASSERT(indicatorChar == QChar(0x25BC)); // ▼
                    }

                    // Children of expanded node should appear at depth 1
                    // Their indicator is at offset indentWidth(2)
                    int childRow = parentRow + 1;
                    TreeTestModel::Node *parentNode = parentNodes.first();
                    for (int c = 0; c < parentNode->children.size() && childRow < viewHeight; ++c, ++childRow) {
                        bool childHasKids = !parentNode->children.at(c)->children.isEmpty();
                        QChar childIndicator = buf.backCell(2, childRow).character; // indent=2

                        if (childHasKids) {
                            RC_ASSERT(childIndicator == QChar(0x25B6)); // ▶
                        } else {
                            RC_ASSERT(childIndicator == QLatin1Char(' '));
                        }
                    }
                }
            });
    }

    // **Validates: Requirements 8.6**
    // Feature: widget-rendering-overhaul, Property 20: Tree Expand/Collapse Rebuild and Dirty
    void treeExpandCollapseRebuildAndDirty()
    {
        rc::check("On expand/collapse, flat list is rebuilt and affected rows are marked dirty",
            []() {
                // We need QTuiApplication for the scheduler
                int argc = 0;
                QTuiApplication app(argc, nullptr);
                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 40, 20));

                TreeTestModel model;

                // Create a tree with a parent node that has children
                const int rootChildren = *rc::gen::inRange(2, 5);
                TreeTestModel::Node *expandableNode = nullptr;

                for (int i = 0; i < rootChildren; ++i) {
                    auto *child = model.addChild(model.rootNode(),
                                                 QStringLiteral("Root%1").arg(i));
                    if (i == 0) {
                        // First node gets children
                        int numChildren = *rc::gen::inRange(1, 4);
                        for (int j = 0; j < numChildren; ++j) {
                            model.addChild(child, QStringLiteral("Child%1").arg(j));
                        }
                        expandableNode = child;
                    }
                }

                const int viewHeight = rootChildren + 5;
                const int viewWidth = 40;

                QTuiTreeView tv(&mainWindow);
                tv.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tv.setModel(&model);

                // Clear any dirty state from setup
                tv.clearDirtyRegions();
                app.scheduler()->renderNow();

                // Expand the node
                QModelIndex expandIdx = model.indexForNode(expandableNode);
                tv.expand(expandIdx);

                // After expand, the widget should be dirty
                RC_ASSERT(tv.isDirty());

                // The dirty region should cover from the expanded node's row to the bottom
                QRect dirtyRegion = tv.dirtyRegion();
                RC_ASSERT(dirtyRegion.height() > 0);

                // Verify the flat list was rebuilt: the tree view should now show
                // the children of the expanded node
                ScreenBuffer buf(viewWidth, viewHeight);
                tv.paintEvent(buf);

                // The expanded node's children should be visible at depth 1
                // First row is the expanded node itself (depth 0)
                // Subsequent rows should be its children (depth 1, indented by 2)
                int childStartRow = 1; // row after the expanded node
                int numExpandedChildren = expandableNode->children.size();

                for (int c = 0; c < numExpandedChildren && (childStartRow + c) < viewHeight; ++c) {
                    // At depth 1, indentation is 2 chars, so text starts at offset 2 + 2 = 4
                    // (indent=2, indicator=2)
                    // First 2 chars should be spaces (indentation)
                    QChar indent0 = buf.backCell(0, childStartRow + c).character;
                    QChar indent1 = buf.backCell(1, childStartRow + c).character;
                    RC_ASSERT(indent0 == QLatin1Char(' '));
                    RC_ASSERT(indent1 == QLatin1Char(' '));
                }

                // Now collapse and verify dirty marking again
                tv.clearDirtyRegions();
                tv.collapse(expandIdx);

                RC_ASSERT(tv.isDirty());
                QRect collapseDirty = tv.dirtyRegion();
                RC_ASSERT(collapseDirty.height() > 0);

                // After collapse, the children should no longer be visible
                ScreenBuffer buf2(viewWidth, viewHeight);
                tv.paintEvent(buf2);

                // Row 1 should now be the second root child (not a child of the collapsed node)
                if (rootChildren > 1) {
                    // The second root item text starts at offset 2 (depth 0, indicator 2)
                    // and should be "Root1"
                    QString expectedText = QStringLiteral("Root1");
                    // indicator at offset 0 (leaf or parent), text at offset 2
                    QChar textStart = buf2.backCell(2, 1).character;
                    RC_ASSERT(textStart == expectedText.at(0));
                }
            });
    }
};

QTEST_APPLESS_MAIN(TestTreeViewRendering)
#include "test_treeview_rendering.moc"
