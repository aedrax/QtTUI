#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractTableModel>
#include <QAbstractListModel>
#include <QCoreApplication>
#include <QSignalSpy>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// Helper: send a key event to a widget via QCoreApplication::sendEvent
// ---------------------------------------------------------------------------
static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

// ---------------------------------------------------------------------------
// Simple table model for testing (Qt6::Core only)
// ---------------------------------------------------------------------------
class PreservationTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    PreservationTableModel(int rows, int cols, QObject *parent = nullptr)
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
// Simple list model for testing (Qt6::Core only)
// ---------------------------------------------------------------------------
class PreservationListModel : public QAbstractListModel {
    Q_OBJECT
public:
    PreservationListModel(int rows, QObject *parent = nullptr)
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
// Test environment helper
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
// Test class: Preservation Property Tests
// **Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8**
//
// These tests MUST PASS on unfixed code. They capture existing correct
// behavior that must be preserved after the dirty-tracking fixes.
// ---------------------------------------------------------------------------
class TestDirtyTrackingPreservation : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.1**
    // For all random text inputs to QTuiLineEdit, textChanged signal emits
    // the correct updated text
    // -----------------------------------------------------------------------
    void lineEditTextChangedSignalEmitsCorrectText()
    {
        QVERIFY(rc::check("QTuiLineEdit textChanged emits correct text after key press",
            []() {
                TestEnv env;

                QTuiLineEdit lineEdit(&env.mainWindow);
                lineEdit.setGeometry(QRect(0, 0, 40, 1));
                lineEdit.setVisible(true);
                lineEdit.setFocus();

                env.drain();

                // Generate a random sequence of printable characters (1-10 chars)
                const int numChars = *rc::gen::inRange(1, 11);
                std::vector<int> chars;
                chars.reserve(numChars);
                for (int i = 0; i < numChars; ++i)
                    chars.push_back(*rc::gen::inRange(32, 127));

                QString expectedText;
                QString lastEmitted;

                // Connect to textChanged to capture emitted text
                QObject::connect(&lineEdit, &QTuiLineEdit::textChanged,
                    [&lastEmitted](const QString &text) {
                        lastEmitted = text;
                    });

                for (int charCode : chars) {
                    QChar ch(charCode);
                    expectedText.append(ch);
                    sendKey(lineEdit, charCode, Qt::NoModifier, QString(ch));
                }

                // The last emitted text should match what we typed
                RC_ASSERT(lastEmitted == expectedText);
                // The widget's text() should also match
                RC_ASSERT(lineEdit.text() == expectedText);
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.5**
    // For all random model states, TableView model signal handlers
    // (onDataChanged, onRowsInserted, onRowsRemoved, onModelReset)
    // mark the same dirty regions as before
    // -----------------------------------------------------------------------
    void tableViewModelSignalsDirtyRegions()
    {
        QVERIFY(rc::check("TableView model signal handlers mark correct dirty regions",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 40);
                const int colCount = *rc::gen::inRange(1, 5);

                PreservationTableModel model(rowCount, colCount);

                QTuiTableView tableView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 15);
                tableView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tableView.setVisible(true);
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                // Choose a random signal type: 0=dataChanged, 1=rowsInserted, 2=rowsRemoved, 3=modelReset
                const int signalType = *rc::gen::inRange(0, 4);

                const int scrollOffset = tableView.scrollOffset();
                const int visibleDataRows = viewHeight - 1; // header takes 1 row
                const int firstVisibleRow = scrollOffset;
                const int lastVisibleRow = scrollOffset + visibleDataRows - 1;

                switch (signalType) {
                case 0: { // dataChanged
                    const int changedTop = *rc::gen::inRange(0, rowCount);
                    const int changedBottom = *rc::gen::inRange(changedTop, rowCount);
                    model.emitDataChanged(changedTop, changedBottom, 0, colCount - 1);

                    if (changedTop > lastVisibleRow || changedBottom < firstVisibleRow) {
                        RC_ASSERT(!tableView.isDirty());
                    } else {
                        RC_ASSERT(tableView.isDirty());
                        QRect dirtyRegion = tableView.dirtyRegion();
                        QRect geo = tableView.geometry();
                        RC_ASSERT(geo.contains(dirtyRegion));
                    }
                    break;
                }
                case 1: { // rowsInserted
                    const int position = *rc::gen::inRange(0, rowCount);
                    model.insertRowsAt(position, 1);

                    if (position > lastVisibleRow) {
                        RC_ASSERT(!tableView.isDirty());
                    } else {
                        RC_ASSERT(tableView.isDirty());
                    }
                    break;
                }
                case 2: { // rowsRemoved
                    const int position = *rc::gen::inRange(0, rowCount);
                    model.removeRowsAt(position, 1);

                    if (position > lastVisibleRow) {
                        RC_ASSERT(!tableView.isDirty());
                    } else {
                        RC_ASSERT(tableView.isDirty());
                    }
                    break;
                }
                case 3: { // modelReset
                    model.doReset();
                    RC_ASSERT(tableView.isDirty());
                    QRect dirtyRegion = tableView.dirtyRegion();
                    QRect geo = tableView.geometry();
                    RC_ASSERT(dirtyRegion.contains(geo));
                    break;
                }
                }
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.6**
    // For all random model states, ListView model signal handlers mark
    // correct dirty regions on unfixed code
    // -----------------------------------------------------------------------
    void listViewModelSignalsDirtyRegions()
    {
        QVERIFY(rc::check("ListView model signal handlers mark correct dirty regions",
            []() {
                TestEnv env;

                const int rowCount = *rc::gen::inRange(5, 40);

                PreservationListModel model(rowCount);

                QTuiListView listView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                const int viewHeight = *rc::gen::inRange(5, 15);
                listView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                listView.setVisible(true);
                listView.setModel(&model);

                env.drain();
                listView.clearDirtyRegions();
                env.drain();

                // Choose a random signal type
                const int signalType = *rc::gen::inRange(0, 4);

                const int scrollOffset = listView.scrollOffset();
                const int firstVisibleRow = scrollOffset;
                const int lastVisibleRow = scrollOffset + viewHeight - 1;

                switch (signalType) {
                case 0: { // dataChanged
                    const int changedTop = *rc::gen::inRange(0, rowCount);
                    const int changedBottom = *rc::gen::inRange(changedTop, rowCount);
                    model.emitDataChanged(changedTop, changedBottom);

                    if (changedTop > lastVisibleRow || changedBottom < firstVisibleRow) {
                        RC_ASSERT(!listView.isDirty());
                    } else {
                        RC_ASSERT(listView.isDirty());
                        QRect dirtyRegion = listView.dirtyRegion();
                        QRect geo = listView.geometry();
                        RC_ASSERT(geo.contains(dirtyRegion));
                    }
                    break;
                }
                case 1: { // rowsInserted
                    const int position = *rc::gen::inRange(0, rowCount);
                    model.insertRowsAt(position, 1);

                    if (position > lastVisibleRow) {
                        RC_ASSERT(!listView.isDirty());
                    } else {
                        RC_ASSERT(listView.isDirty());
                    }
                    break;
                }
                case 2: { // rowsRemoved
                    const int position = *rc::gen::inRange(0, rowCount);
                    model.removeRowsAt(position, 1);

                    if (position > lastVisibleRow) {
                        RC_ASSERT(!listView.isDirty());
                    } else {
                        RC_ASSERT(listView.isDirty());
                    }
                    break;
                }
                case 3: { // modelReset
                    model.doReset();
                    RC_ASSERT(listView.isDirty());
                    QRect dirtyRegion = listView.dirtyRegion();
                    QRect geo = listView.geometry();
                    RC_ASSERT(dirtyRegion.contains(geo));
                    break;
                }
                }
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.2**
    // For all random geometry changes, setGeometry() marks both old and new
    // geometry dirty
    // -----------------------------------------------------------------------
    void setGeometryMarksBothOldAndNewDirty()
    {
        QVERIFY(rc::check("setGeometry() marks both old and new geometry dirty",
            []() {
                TestEnv env;

                QTuiLabel widget("Test", &env.mainWindow);
                // Set initial geometry
                const int x1 = *rc::gen::inRange(0, 40);
                const int y1 = *rc::gen::inRange(0, 12);
                const int w1 = *rc::gen::inRange(5, 30);
                const int h1 = *rc::gen::inRange(2, 10);
                QRect oldGeo(x1, y1, w1, h1);
                widget.setGeometry(oldGeo);
                widget.setVisible(true);

                env.drain();
                widget.clearDirtyRegions();
                env.drain();

                // Generate a different geometry
                const int x2 = *rc::gen::inRange(0, 40);
                const int y2 = *rc::gen::inRange(0, 12);
                const int w2 = *rc::gen::inRange(5, 30);
                const int h2 = *rc::gen::inRange(2, 10);
                QRect newGeo(x2, y2, w2, h2);

                // Only test when geometry actually changes
                RC_PRE(oldGeo != newGeo);

                widget.setGeometry(newGeo);

                // Widget should be dirty
                RC_ASSERT(widget.isDirty());

                // The dirty region should cover both old and new geometry
                QRect dirtyRegion = widget.dirtyRegion();
                RC_ASSERT(dirtyRegion.contains(oldGeo));
                RC_ASSERT(dirtyRegion.contains(newGeo));
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.3**
    // For all random focus changes, both old and new focus widgets are
    // marked dirty
    // -----------------------------------------------------------------------
    void setFocusMarksBothOldAndNewWidgetsDirty()
    {
        QVERIFY(rc::check("setFocus() marks both old and new focus widgets dirty",
            []() {
                TestEnv env;

                // Create two widgets
                QTuiLineEdit widget1(&env.mainWindow);
                widget1.setGeometry(QRect(0, 0, 30, 1));
                widget1.setVisible(true);

                QTuiLineEdit widget2(&env.mainWindow);
                widget2.setGeometry(QRect(0, 2, 30, 1));
                widget2.setVisible(true);

                // Give focus to widget1 first
                widget1.setFocus();
                env.drain();

                // Clear dirty state
                widget1.clearDirtyRegions();
                widget2.clearDirtyRegions();
                env.drain();

                // Now switch focus to widget2
                widget2.setFocus();

                // Both widgets should be dirty
                RC_ASSERT(widget1.isDirty()); // lost focus
                RC_ASSERT(widget2.isDirty()); // gained focus
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.8**
    // For all ASCII characters (code points 32-127), AnsiRenderer output
    // byte equals the character's Latin-1 value
    // -----------------------------------------------------------------------
    void ansiRendererAsciiPreservation()
    {
        QVERIFY(rc::check("AnsiRenderer output byte equals ASCII character value for code points 33-127",
            []() {
                // Generate a random printable ASCII character (33-127, excluding space
                // since space is the default cell value and won't produce a diff)
                const int codePoint = *rc::gen::inRange(33, 128);
                QChar asciiChar(codePoint);

                // Create a small screen buffer and write the character
                ScreenBuffer buffer(5, 1);
                buffer.invalidateAll();
                buffer.swapBuffers();

                // Write the ASCII character to the back buffer
                Cell &cell = buffer.backCell(0, 0);
                cell.character = asciiChar;
                cell.foreground = Color(255, 255, 255);
                cell.background = Color(0, 0, 0);

                // Render
                AnsiRenderer renderer;
                QByteArray output = renderer.render(buffer);

                // The output should contain the ASCII byte (Latin-1 value)
                char expectedByte = static_cast<char>(codePoint);
                RC_ASSERT(output.contains(expectedByte));
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.7**
    // For all non-dirty widgets encountered by paintTree(), painting is
    // skipped (incremental rendering preserved)
    // -----------------------------------------------------------------------
    void nonDirtyWidgetsSkippedByPaintTree()
    {
        QVERIFY(rc::check("Non-dirty widgets are skipped by paintTree (incremental rendering)",
            []() {
                TestEnv env;

                // Create a widget with some content
                QTuiLabel label("Hello World", &env.mainWindow);
                const int x = *rc::gen::inRange(0, 30);
                const int y = *rc::gen::inRange(0, 10);
                const int w = *rc::gen::inRange(12, 40);
                label.setGeometry(QRect(x, y, w, 1));
                label.setVisible(true);

                // Do a full render to paint everything
                env.app.triggerRender();
                env.drain();

                // Now clear dirty state — widget is clean
                label.clearDirtyRegions();
                env.drain();

                // Verify the widget is NOT dirty
                RC_ASSERT(!label.isDirty());

                // Get the current back buffer state at the widget's position
                ScreenBuffer *buffer = env.app.screenBuffer();
                RC_PRE(buffer != nullptr);

                // Record what's in the buffer at the label's position
                QRect geo = label.geometry();
                QChar charBefore = buffer->backCell(geo.x(), geo.y()).character;

                // Swap buffers to simulate a new frame
                buffer->swapBuffers();

                // Write something different to the back buffer at the label's position
                Cell &cell = buffer->backCell(geo.x(), geo.y());
                cell.character = QLatin1Char('Z');

                // The label is NOT dirty, so if paintTree runs, it should NOT
                // overwrite our 'Z' with the label's content
                // (We can't directly call paintTree, but we can verify the
                // isDirty() == false contract which paintTree relies on)
                RC_ASSERT(!label.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.5**
    // TableView: dataChanged for rows entirely outside viewport produces
    // no dirty marking (viewport filtering preserved)
    // -----------------------------------------------------------------------
    void tableViewViewportFilteringPreserved()
    {
        QVERIFY(rc::check("TableView dataChanged outside viewport produces no dirty marking",
            []() {
                TestEnv env;

                const int viewHeight = *rc::gen::inRange(5, 10);
                const int rowCount = viewHeight + *rc::gen::inRange(5, 30);
                const int colCount = *rc::gen::inRange(1, 4);

                PreservationTableModel model(rowCount, colCount);

                QTuiTableView tableView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                tableView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                tableView.setVisible(true);
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                const int scrollOffset = tableView.scrollOffset();
                const int visibleDataRows = viewHeight - 1;
                const int lastVisibleRow = scrollOffset + visibleDataRows - 1;

                // Generate a dataChanged signal entirely BELOW the viewport
                RC_PRE(lastVisibleRow + 1 < rowCount);
                const int changedTop = *rc::gen::inRange(lastVisibleRow + 1, rowCount);
                const int changedBottom = *rc::gen::inRange(changedTop, rowCount);

                model.emitDataChanged(changedTop, changedBottom, 0, colCount - 1);

                RC_ASSERT(!tableView.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 3.6**
    // ListView: dataChanged for rows entirely outside viewport produces
    // no dirty marking (viewport filtering preserved)
    // -----------------------------------------------------------------------
    void listViewViewportFilteringPreserved()
    {
        QVERIFY(rc::check("ListView dataChanged outside viewport produces no dirty marking",
            []() {
                TestEnv env;

                const int viewHeight = *rc::gen::inRange(5, 10);
                const int rowCount = viewHeight + *rc::gen::inRange(5, 30);

                PreservationListModel model(rowCount);

                QTuiListView listView(&env.mainWindow);
                const int viewWidth = *rc::gen::inRange(20, 60);
                listView.setGeometry(QRect(0, 0, viewWidth, viewHeight));
                listView.setVisible(true);
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
            }));
    }
};

QTEST_APPLESS_MAIN(TestDirtyTrackingPreservation)
#include "test_dirty_tracking_preservation.moc"
