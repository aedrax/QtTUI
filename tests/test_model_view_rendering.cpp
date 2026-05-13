#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractListModel>
#include <QAbstractItemModel>

using namespace QtTUI;

// Feature: widget-rendering-overhaul, Property 16: Model/View DisplayRole Rendering
// Feature: widget-rendering-overhaul, Property 17: List View Scroll Keeps Current Visible

// ---------------------------------------------------------------------------
// Simple list model for testing
// ---------------------------------------------------------------------------
class TestListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit TestListModel(const QStringList &items, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_items(items)
    {
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_items.size(); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || !index.isValid())
            return {};
        if (index.row() < 0 || index.row() >= m_items.size())
            return {};
        return m_items.at(index.row());
    }

private:
    QStringList m_items;
};

// ---------------------------------------------------------------------------
// Simple tree model for testing DisplayRole rendering
// ---------------------------------------------------------------------------
class SimpleTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    struct Node {
        QString text;
        QList<Node *> children;
        Node *parent = nullptr;
        int row = 0;
    };

    explicit SimpleTreeModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent)
    {
        m_root = new Node{QStringLiteral("root"), {}, nullptr, 0};
    }

    ~SimpleTreeModel() override { deleteNode(m_root); }

    Node *addChild(Node *parent, const QString &text)
    {
        auto *child = new Node{text, {}, parent, static_cast<int>(parent->children.size())};
        parent->children.append(child);
        return child;
    }

    Node *rootNode() const { return m_root; }

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

class TestModelViewRendering : public QObject {
    Q_OBJECT
private slots:

    // **Validates: Requirements 7.1, 8.3**
    // Feature: widget-rendering-overhaul, Property 16: Model/View DisplayRole Rendering
    void listViewDisplayRoleRendering()
    {
        rc::check("QTuiListView renders text matching data(index, Qt::DisplayRole) for each visible row",
            []() {
                // Generate random list items
                const int itemCount = *rc::gen::inRange(1, 15);
                QStringList items;
                for (int i = 0; i < itemCount; ++i) {
                    int len = *rc::gen::inRange(1, 12);
                    QString text;
                    for (int j = 0; j < len; ++j)
                        text.append(QChar(*rc::gen::inRange(0x41, 0x5B))); // A-Z
                    items.append(text);
                }

                TestListModel model(items);

                const int viewWidth = *rc::gen::inRange(5, 30);
                const int viewHeight = *rc::gen::inRange(1, qMin(itemCount, 10));

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                lv.setModel(&model);

                ScreenBuffer buf(viewWidth, viewHeight);
                lv.paintEvent(buf);

                // Verify each visible row contains the DisplayRole text (subject to truncation)
                int scrollOffset = lv.scrollOffset();
                for (int vr = 0; vr < viewHeight; ++vr) {
                    int modelRow = scrollOffset + vr;
                    if (modelRow >= itemCount)
                        break;

                    QString expectedText = items.at(modelRow);
                    // Read rendered text from buffer
                    QString renderedText;
                    for (int x = 0; x < viewWidth; ++x) {
                        QString ch = buf.backCell(x, vr).text();
                        renderedText.append(ch);
                    }

                    // The rendered text should start with the expected text (or truncated version)
                    if (expectedText.length() <= viewWidth) {
                        // Full text should appear at the start
                        for (int i = 0; i < expectedText.length(); ++i) {
                            RC_ASSERT(renderedText.at(i) == expectedText.at(i));
                        }
                    } else {
                        // Text is truncated: first viewWidth-1 chars + ellipsis
                        if (viewWidth >= 2) {
                            for (int i = 0; i < viewWidth - 1; ++i) {
                                RC_ASSERT(renderedText.at(i) == expectedText.at(i));
                            }
                            RC_ASSERT(renderedText.at(viewWidth - 1) == QChar(0x2026));
                        } else if (viewWidth == 1) {
                            RC_ASSERT(renderedText.at(0) == QChar(0x2026));
                        }
                    }
                }
            });
    }

    // Feature: widget-rendering-overhaul, Property 16: Model/View DisplayRole Rendering (TreeView)
    void treeViewDisplayRoleRendering()
    {
        rc::check("QTuiTreeView renders text matching data(index, Qt::DisplayRole) for visible items",
            []() {
                // Create a simple flat tree (all items at depth 0, no children)
                const int itemCount = *rc::gen::inRange(1, 10);
                QStringList items;
                for (int i = 0; i < itemCount; ++i) {
                    int len = *rc::gen::inRange(1, 8);
                    QString text;
                    for (int j = 0; j < len; ++j)
                        text.append(QChar(*rc::gen::inRange(0x41, 0x5B)));
                    items.append(text);
                }

                SimpleTreeModel model;
                for (const auto &text : items)
                    model.addChild(model.rootNode(), text);

                // Use wide enough view to avoid truncation of short items
                // Leaf items have 2 chars of indicator space ("  ") at depth 0
                const int viewWidth = *rc::gen::inRange(15, 40);
                const int viewHeight = *rc::gen::inRange(1, qMin(itemCount, 8));

                QTuiTreeView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tv.setModel(&model);

                ScreenBuffer buf(viewWidth, viewHeight);
                tv.paintEvent(buf);

                // For depth-0 leaf items: indentation = 0, indicator = "  " (2 spaces)
                // So text starts at offset 2
                const int textOffset = 2; // depth*indentWidth(2) + indicator(2) = 0 + 2 = 2

                for (int vr = 0; vr < viewHeight && vr < itemCount; ++vr) {
                    QString expectedText = items.at(vr);

                    // Read rendered text starting from textOffset
                    int availWidth = viewWidth - textOffset;
                    QString renderedText;
                    for (int x = textOffset; x < viewWidth; ++x) {
                        QString ch = buf.backCell(x, vr).text();
                        renderedText.append(ch);
                    }

                    // Verify text content (subject to truncation)
                    if (expectedText.length() <= availWidth) {
                        for (int i = 0; i < expectedText.length(); ++i) {
                            RC_ASSERT(renderedText.at(i) == expectedText.at(i));
                        }
                    } else if (availWidth >= 2) {
                        for (int i = 0; i < availWidth - 1; ++i) {
                            RC_ASSERT(renderedText.at(i) == expectedText.at(i));
                        }
                        RC_ASSERT(renderedText.at(availWidth - 1) == QChar(0x2026));
                    }
                }
            });
    }

    // **Validates: Requirements 7.4**
    // Feature: widget-rendering-overhaul, Property 17: List View Scroll Keeps Current Visible
    void listViewScrollKeepsCurrentVisible()
    {
        rc::check("The current row is always within [scrollOffset, scrollOffset + visibleHeight - 1]",
            []() {
                // Generate a list with more items than the view height
                const int viewHeight = *rc::gen::inRange(3, 8);
                const int itemCount = *rc::gen::inRange(viewHeight + 1, viewHeight + 20);

                QStringList items;
                for (int i = 0; i < itemCount; ++i)
                    items.append(QStringLiteral("Item%1").arg(i));

                TestListModel model(items);

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, 20, viewHeight));
                lv.setModel(&model);

                // Navigate to a random row
                int targetRow = *rc::gen::inRange(0, itemCount);
                lv.setCurrentIndex(model.index(targetRow, 0));

                // Verify the invariant: current row is within visible range
                int scrollOffset = lv.scrollOffset();
                int currentRow = lv.currentIndex().row();

                RC_ASSERT(currentRow >= scrollOffset);
                RC_ASSERT(currentRow < scrollOffset + viewHeight);
            });
    }

    // Feature: widget-rendering-overhaul, Property 17: List View Scroll via Key Navigation
    void listViewScrollViaKeyNavigation()
    {
        rc::check("After any sequence of Up/Down key presses, current row remains visible",
            []() {
                const int viewHeight = *rc::gen::inRange(3, 6);
                const int itemCount = *rc::gen::inRange(viewHeight + 1, viewHeight + 15);

                QStringList items;
                for (int i = 0; i < itemCount; ++i)
                    items.append(QStringLiteral("Item%1").arg(i));

                TestListModel model(items);

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, 20, viewHeight));
                lv.setModel(&model);

                // Perform a random sequence of key presses
                int numPresses = *rc::gen::inRange(1, 30);
                for (int i = 0; i < numPresses; ++i) {
                    bool goDown = *rc::gen::arbitrary<bool>();
                    Qt::Key key = goDown ? Qt::Key_Down : Qt::Key_Up;
                    TuiKeyEvent ev(key, Qt::NoModifier);
                    QCoreApplication::sendEvent(&lv, &ev);

                    // After each key press, verify the invariant
                    int scrollOffset = lv.scrollOffset();
                    QModelIndex current = lv.currentIndex();
                    if (current.isValid()) {
                        int currentRow = current.row();
                        RC_ASSERT(currentRow >= scrollOffset);
                        RC_ASSERT(currentRow < scrollOffset + viewHeight);
                    }
                }
            });
    }
};

QTEST_MAIN(TestModelViewRendering)
#include "test_model_view_rendering.moc"
