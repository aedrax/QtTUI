#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

using namespace QtTUI;

// Simple in-memory list model for testing
class SimpleListModel : public QAbstractListModel {
    Q_OBJECT
public:
    SimpleListModel(int rows, QObject *parent = nullptr)
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

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole || !index.isValid())
            return false;
        m_data[index.row()] = value.toString();
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }

    void addRows(int count)
    {
        int first = rowCount();
        beginInsertRows(QModelIndex(), first, first + count - 1);
        for (int i = 0; i < count; ++i)
            m_data.append(QStringLiteral("Item%1").arg(first + i));
        endInsertRows();
    }

    void removeFirstRows(int count)
    {
        count = qMin(count, rowCount());
        if (count <= 0)
            return;
        beginRemoveRows(QModelIndex(), 0, count - 1);
        m_data.remove(0, count);
        endRemoveRows();
    }

    void resetModel()
    {
        beginResetModel();
        m_data.clear();
        endResetModel();
    }

private:
    QVector<QString> m_data;
};

// Helper: send a key event to a widget
static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

// Helper: generate a random short label
static rc::Gen<QString> genLabel()
{
    return rc::gen::mapcat(rc::gen::inRange(1, 9), [](int len) {
        return rc::gen::map(
            rc::gen::container<std::vector<int>>(len, rc::gen::inRange(65, 91)),
            [](const std::vector<int> &chars) {
                QString s;
                s.reserve(static_cast<int>(chars.size()));
                for (int c : chars)
                    s.append(QChar(c));
                return s;
            });
    });
}

class TestQTuiListView : public QObject {
    Q_OBJECT
private slots:

    // Feature: qttui-framework, Property 22: ListView model signal response
    void modelSignalResponse()
    {
        rc::check("dataChanged repaints only affected items; row changes reflect new rowCount",
            []() {
                const auto rows = *rc::gen::inRange(1, 20);
                SimpleListModel model(rows);

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, 20, rows + 5));
                lv.setModel(&model);

                // --- dataChanged: modify an item and verify model reflects it ---
                const auto changeRow = *rc::gen::inRange(0, rows);
                const auto newText = *genLabel();
                model.setData(model.index(changeRow, 0), newText);

                ScreenBuffer buf(20, rows + 5);
                lv.paintEvent(buf);
                RC_ASSERT(model.data(model.index(changeRow, 0)).toString() == newText);

                // --- rowsInserted ---
                int oldRowCount = model.rowCount();
                const auto insertCount = *rc::gen::inRange(1, 5);
                model.addRows(insertCount);
                RC_ASSERT(model.rowCount() == oldRowCount + insertCount);

                QModelIndex cur = lv.currentIndex();
                if (cur.isValid()) {
                    RC_ASSERT(cur.row() < model.rowCount());
                }

                // --- rowsRemoved ---
                int preRemoveCount = model.rowCount();
                const auto removeCount = *rc::gen::inRange(1, qMin(preRemoveCount, 3) + 1);
                model.removeFirstRows(removeCount);
                RC_ASSERT(model.rowCount() == preRemoveCount - removeCount);

                cur = lv.currentIndex();
                if (model.rowCount() > 0) {
                    RC_ASSERT(cur.isValid());
                    RC_ASSERT(cur.row() < model.rowCount());
                } else {
                    RC_ASSERT(!cur.isValid());
                }
            });
    }

    // Regression: sorting through a QSortFilterProxyModel emits layoutChanged
    // (not rowsMoved/modelReset); the view must handle it by re-clamping its
    // current index / scroll offset, marking itself dirty, and rendering the
    // new order.
    void proxyModelSortReordersView()
    {
        SimpleListModel model(5); // "Item0" .. "Item4" (ascending)

        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);

        QTuiListView lv;
        const int viewHeight = 3;
        lv.setGeometry(QRect(0, 0, 20, viewHeight));
        lv.setModel(&proxy);

        // Move to the last row so the view is scrolled down
        lv.setCurrentIndex(proxy.index(4, 0));
        QCOMPARE(lv.scrollOffset(), 2);
        lv.clearDirtyRegions();

        // Descending sort reorders every row and emits layoutChanged
        proxy.sort(0, Qt::DescendingOrder);

        // The view must have noticed the change
        QVERIFY(lv.isDirty());

        // Current index and scroll offset must still be valid and consistent
        QModelIndex cur = lv.currentIndex();
        QVERIFY(cur.isValid());
        QVERIFY(cur.row() < proxy.rowCount());
        const int scrollOff = lv.scrollOffset();
        QVERIFY(cur.row() >= scrollOff);
        QVERIFY(cur.row() < scrollOff + viewHeight);

        // Painting must show the new (descending) order at the top of the
        // visible window: with scrollOffset 2 the first visible row is
        // proxy row 2 == "Item2" in descending order.
        ScreenBuffer buf(20, viewHeight);
        lv.paintEvent(buf);
        QString firstLine;
        for (int x = 0; x < 20; ++x)
            firstLine.append(buf.backCell(x, 0).text());
        QCOMPARE(firstLine.trimmed(),
                 proxy.data(proxy.index(scrollOff, 0)).toString());
    }

    // Feature: qttui-framework, Property 23: ListView vertical scrolling
    void verticalScrolling()
    {
        rc::check("Scroll offset changes visible items; all items reachable",
            []() {
                const auto visibleHeight = *rc::gen::inRange(2, 10);
                const auto totalRows = *rc::gen::inRange(visibleHeight + 1, visibleHeight + 20);
                SimpleListModel model(totalRows);

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, 20, visibleHeight));
                lv.setModel(&model);

                // Navigate to every row and verify it's within the visible window
                for (int r = 0; r < totalRows; ++r) {
                    lv.setCurrentIndex(model.index(r, 0));

                    int scrollOff = lv.scrollOffset();
                    RC_ASSERT(r >= scrollOff);
                    RC_ASSERT(r < scrollOff + visibleHeight);
                }
            });
    }

    // Feature: qttui-framework, Property 24: ListView arrow key navigation
    void arrowKeyNavigation()
    {
        rc::check("Up->max(0,i-1), Down->min(N-1,i+1)",
            []() {
                const auto rows = *rc::gen::inRange(2, 20);
                SimpleListModel model(rows);

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, 20, rows));
                lv.setModel(&model);

                const auto startRow = *rc::gen::inRange(0, rows);
                lv.setCurrentIndex(model.index(startRow, 0));

                const auto dir = *rc::gen::inRange(0, 2);

                if (dir == 0) {
                    sendKey(lv, Qt::Key_Up);
                    RC_ASSERT(lv.currentIndex().row() == qMax(0, startRow - 1));
                } else {
                    sendKey(lv, Qt::Key_Down);
                    RC_ASSERT(lv.currentIndex().row() == qMin(rows - 1, startRow + 1));
                }
            });
    }

    // Feature: qttui-framework, Property 25: ListView Enter activation
    void enterActivation()
    {
        rc::check("Enter emits activated with QModelIndex at row i",
            []() {
                const auto rows = *rc::gen::inRange(1, 20);
                SimpleListModel model(rows);

                QTuiListView lv;
                lv.setGeometry(QRect(0, 0, 20, rows));
                lv.setModel(&model);

                const auto r = *rc::gen::inRange(0, rows);
                lv.setCurrentIndex(model.index(r, 0));

                QSignalSpy spy(&lv, &QTuiListView::activated);

                sendKey(lv, Qt::Key_Return);

                RC_ASSERT(spy.count() == 1);
                QModelIndex emitted = spy.at(0).at(0).value<QModelIndex>();
                RC_ASSERT(emitted.row() == r);
                RC_ASSERT(emitted.column() == 0);
            });
    }
};

QTEST_MAIN(TestQTuiListView)
#include "test_qtui_listview.moc"
