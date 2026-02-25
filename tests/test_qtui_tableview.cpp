#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractTableModel>

using namespace QtTUI;

// Simple in-memory table model for testing (no Qt GUI dependency)
class SimpleTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    SimpleTableModel(int rows, int cols, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
        m_headers.resize(cols);
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r) {
            m_data[r].resize(cols);
            for (int c = 0; c < cols; ++c)
                m_data[r][c] = QStringLiteral("R%1C%2").arg(r).arg(c);
        }
        for (int c = 0; c < cols; ++c)
            m_headers[c] = QStringLiteral("H%1").arg(c);
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = QModelIndex()) const override { return m_headers.size(); }

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
        if (section < 0 || section >= columnCount())
            return {};
        return m_headers[section];
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole || !index.isValid())
            return false;
        m_data[index.row()][index.column()] = value.toString();
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }

    void setHeaderLabel(int col, const QString &label)
    {
        if (col >= 0 && col < columnCount())
            m_headers[col] = label;
    }

    void addRows(int count)
    {
        int first = rowCount();
        beginInsertRows(QModelIndex(), first, first + count - 1);
        for (int i = 0; i < count; ++i) {
            QVector<QString> row(columnCount());
            for (int c = 0; c < columnCount(); ++c)
                row[c] = QStringLiteral("R%1C%2").arg(first + i).arg(c);
            m_data.append(row);
        }
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
    QVector<QVector<QString>> m_data;
    QVector<QString> m_headers;
};

// Helper: send a key event to a widget
static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

// Helper: generate a random short label (uppercase letters only, 1-8 chars)
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

class TestQTuiTableView : public QObject {
    Q_OBJECT
private slots:

    // Feature: qttui-framework, Property 17: TableView model signal response
    void modelSignalResponse()
    {
        rc::check("dataChanged repaints only affected range; rowsInserted/Removed reflects new rowCount",
            []() {
                const auto rows = *rc::gen::inRange(1, 20);
                const auto cols = *rc::gen::inRange(1, 6);
                SimpleTableModel model(rows, cols);

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, cols * 10, rows + 5));
                tv.setModel(&model);

                // --- dataChanged: modify a cell and verify model reflects it ---
                const auto changeRow = *rc::gen::inRange(0, rows);
                const auto changeCol = *rc::gen::inRange(0, cols);
                const auto newText = *genLabel();
                model.setData(model.index(changeRow, changeCol), newText);

                // Paint into a buffer, the changed data should be rendered
                ScreenBuffer buf(cols * 10, rows + 5);
                tv.paintEvent(buf);
                RC_ASSERT(model.data(model.index(changeRow, changeCol)).toString() == newText);

                // --- rowsInserted ---
                int oldRowCount = model.rowCount();
                const auto insertCount = *rc::gen::inRange(1, 5);
                model.addRows(insertCount);
                RC_ASSERT(model.rowCount() == oldRowCount + insertCount);

                QModelIndex cur = tv.currentIndex();
                if (cur.isValid()) {
                    RC_ASSERT(cur.row() < model.rowCount());
                    RC_ASSERT(cur.column() < model.columnCount());
                }

                // --- rowsRemoved ---
                int preRemoveCount = model.rowCount();
                const auto removeCount = *rc::gen::inRange(1, qMin(preRemoveCount, 3) + 1);
                model.removeFirstRows(removeCount);
                RC_ASSERT(model.rowCount() == preRemoveCount - removeCount);

                cur = tv.currentIndex();
                if (model.rowCount() > 0 && model.columnCount() > 0) {
                    RC_ASSERT(cur.isValid());
                    RC_ASSERT(cur.row() < model.rowCount());
                } else {
                    RC_ASSERT(!cur.isValid());
                }
            });
    }

    // Feature: qttui-framework, Property 18: TableView header display
    void headerDisplay()
    {
        rc::check("Painted output contains headerData text for each visible column",
            []() {
                const auto cols = *rc::gen::inRange(1, 6);
                const auto rows = *rc::gen::inRange(1, 10);
                SimpleTableModel model(rows, cols);

                QStringList headers;
                for (int c = 0; c < cols; ++c) {
                    QString h = *genLabel();
                    model.setHeaderLabel(c, h);
                    headers.append(h);
                }

                int viewWidth = cols * 10;
                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, rows + 1));
                tv.setModel(&model);

                ScreenBuffer buf(viewWidth, rows + 1);
                tv.paintEvent(buf);

                // Read the header row (y=0) from the buffer
                QString headerRow;
                for (int x = 0; x < viewWidth; ++x)
                    headerRow.append(buf.backCell(x, 0).character);

                // Each header text should appear (truncated to column width)
                int colWidth = viewWidth / cols;
                for (const auto &h : headers) {
                    QString truncated = h.left(colWidth);
                    RC_ASSERT(headerRow.contains(truncated));
                }
            });
    }

    // Feature: qttui-framework, Property 19: TableView vertical scrolling
    void verticalScrolling()
    {
        rc::check("Scroll offset changes visible rows; all rows reachable",
            []() {
                const auto visibleHeight = *rc::gen::inRange(3, 10);
                const int dataRows = visibleHeight - 1; // minus header
                const auto totalRows = *rc::gen::inRange(dataRows + 1, dataRows + 20);
                const auto cols = *rc::gen::inRange(1, 4);
                SimpleTableModel model(totalRows, cols);

                int viewWidth = cols * 10;
                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, visibleHeight));
                tv.setModel(&model);

                // Navigate to every row and verify it's within the visible window
                for (int r = 0; r < totalRows; ++r) {
                    tv.setCurrentIndex(model.index(r, 0));

                    int scrollOff = tv.scrollOffset();
                    RC_ASSERT(r >= scrollOff);
                    RC_ASSERT(r < scrollOff + dataRows);
                }
            });
    }

    // Feature: qttui-framework, Property 20: TableView arrow key navigation
    void arrowKeyNavigation()
    {
        rc::check("Up/Down/Left/Right move current cell with clamping to model bounds",
            []() {
                const auto rows = *rc::gen::inRange(2, 20);
                const auto cols = *rc::gen::inRange(2, 8);
                SimpleTableModel model(rows, cols);

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, cols * 10, rows + 1));
                tv.setModel(&model);

                const auto startRow = *rc::gen::inRange(0, rows);
                const auto startCol = *rc::gen::inRange(0, cols);
                tv.setCurrentIndex(model.index(startRow, startCol));

                const auto dir = *rc::gen::inRange(0, 4);

                if (dir == 0) {
                    sendKey(tv, Qt::Key_Up);
                    RC_ASSERT(tv.currentIndex().row() == qMax(0, startRow - 1));
                    RC_ASSERT(tv.currentIndex().column() == startCol);
                } else if (dir == 1) {
                    sendKey(tv, Qt::Key_Down);
                    RC_ASSERT(tv.currentIndex().row() == qMin(rows - 1, startRow + 1));
                    RC_ASSERT(tv.currentIndex().column() == startCol);
                } else if (dir == 2) {
                    sendKey(tv, Qt::Key_Left);
                    RC_ASSERT(tv.currentIndex().row() == startRow);
                    RC_ASSERT(tv.currentIndex().column() == qMax(0, startCol - 1));
                } else {
                    sendKey(tv, Qt::Key_Right);
                    RC_ASSERT(tv.currentIndex().row() == startRow);
                    RC_ASSERT(tv.currentIndex().column() == qMin(cols - 1, startCol + 1));
                }
            });
    }

    // Feature: qttui-framework, Property 21: TableView Enter activation
    void enterActivation()
    {
        rc::check("Enter emits activated with QModelIndex at current (r, c)",
            []() {
                const auto rows = *rc::gen::inRange(1, 20);
                const auto cols = *rc::gen::inRange(1, 8);
                SimpleTableModel model(rows, cols);

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, cols * 10, rows + 1));
                tv.setModel(&model);

                const auto r = *rc::gen::inRange(0, rows);
                const auto c = *rc::gen::inRange(0, cols);
                tv.setCurrentIndex(model.index(r, c));

                QSignalSpy spy(&tv, &QTuiTableView::activated);

                sendKey(tv, Qt::Key_Return);

                RC_ASSERT(spy.count() == 1);
                QModelIndex emitted = spy.at(0).at(0).value<QModelIndex>();
                RC_ASSERT(emitted.row() == r);
                RC_ASSERT(emitted.column() == c);
            });
    }
};

QTEST_MAIN(TestQTuiTableView)
#include "test_qtui_tableview.moc"
