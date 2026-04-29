#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiTabWidget.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiCheckBox.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractTableModel>
#include <QAbstractListModel>
#include <QCoreApplication>

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
class SimpleTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    SimpleTableModel(int rows, int cols, QObject *parent = nullptr)
        : QAbstractTableModel(parent), m_rows(rows), m_cols(cols)
    {
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r) {
            m_data[r].resize(cols);
            for (int c = 0; c < cols; ++c)
                m_data[r][c] = QStringLiteral("R%1C%2").arg(r).arg(c);
        }
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_rows; }
    int columnCount(const QModelIndex & = QModelIndex()) const override { return m_cols; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || !index.isValid())
            return {};
        return m_data[index.row()][index.column()];
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};
        return QStringLiteral("H%1").arg(section);
    }

private:
    int m_rows;
    int m_cols;
    QVector<QVector<QString>> m_data;
};

// ---------------------------------------------------------------------------
// Simple list model for testing (Qt6::Core only)
// ---------------------------------------------------------------------------
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
        return m_data[index.row()];
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
// Test class: Bug Condition Exploration Tests
// **Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7**
//
// These tests MUST FAIL on unfixed code to confirm the bugs exist.
// DO NOT fix the code or tests when they fail.
// ---------------------------------------------------------------------------
class TestDirtyTrackingBugs : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.1**
    // QTuiLineEdit: Generate random printable chars, send keyPressEvent,
    // assert isDirty() == true after event
    // -----------------------------------------------------------------------
    void lineEditTextEntryMarksDirty()
    {
        QVERIFY(rc::check("QTuiLineEdit isDirty() after printable key press",
            []() {
                TestEnv env;

                QTuiLineEdit lineEdit(&env.mainWindow);
                lineEdit.setGeometry(QRect(0, 0, 40, 1));
                lineEdit.setVisible(true);

                env.drain();
                lineEdit.clearDirtyRegions();
                env.drain();

                // Generate a random printable ASCII character (32-126)
                const int charCode = *rc::gen::inRange(32, 127);
                QChar ch(charCode);

                // Send key press event via sendEvent
                sendKey(lineEdit, charCode, Qt::NoModifier, QString(ch));

                RC_ASSERT(lineEdit.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.1**
    // QTuiLineEdit: Send cursor movement keys (Left, Right, Home, End),
    // assert isDirty() == true
    // -----------------------------------------------------------------------
    void lineEditCursorMovementMarksDirty()
    {
        QVERIFY(rc::check("QTuiLineEdit isDirty() after cursor movement key press",
            []() {
                TestEnv env;

                QTuiLineEdit lineEdit(&env.mainWindow);
                lineEdit.setGeometry(QRect(0, 0, 40, 1));
                lineEdit.setVisible(true);
                lineEdit.setText("Hello World");

                env.drain();
                lineEdit.clearDirtyRegions();
                env.drain();

                // Generate a random cursor movement key
                const auto keyIdx = *rc::gen::inRange(0, 4);
                int keys[] = { Qt::Key_Left, Qt::Key_Right, Qt::Key_Home, Qt::Key_End };
                int key = keys[keyIdx];

                sendKey(lineEdit, key);

                RC_ASSERT(lineEdit.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.2**
    // QTuiTabWidget: Create tab widget with 2+ tabs, switch tabs via
    // setCurrentIndex(), assert TabWidget isDirty() == true
    // -----------------------------------------------------------------------
    void tabWidgetSwitchMarksDirty()
    {
        QVERIFY(rc::check("QTuiTabWidget isDirty() after tab switch",
            []() {
                TestEnv env;

                QTuiTabWidget tabWidget(&env.mainWindow);
                tabWidget.setGeometry(QRect(0, 0, 60, 15));
                tabWidget.setVisible(true);

                // Add 2+ tabs
                const int tabCount = *rc::gen::inRange(2, 6);
                for (int i = 0; i < tabCount; ++i) {
                    auto *page = new QTuiLabel(QStringLiteral("Page%1").arg(i), &tabWidget);
                    page->setVisible(true);
                    tabWidget.addTab(page, QStringLiteral("Tab%1").arg(i));
                }

                env.drain();
                tabWidget.clearDirtyRegions();
                env.drain();

                // Switch to a different tab
                int currentIdx = tabWidget.currentIndex();
                int newIdx = *rc::gen::suchThat(rc::gen::inRange(0, tabCount),
                    [currentIdx](int idx) { return idx != currentIdx; });

                tabWidget.setCurrentIndex(newIdx);

                RC_ASSERT(tabWidget.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.3**
    // QTuiTabWidget: Create tab widget, add first tab, assert page isDirty()
    // == true for initial render
    // -----------------------------------------------------------------------
    void tabWidgetFirstPageDirty()
    {
        QVERIFY(rc::check("QTuiTabWidget first page isDirty() after addTab",
            []() {
                TestEnv env;

                QTuiTabWidget tabWidget(&env.mainWindow);
                tabWidget.setGeometry(QRect(0, 0, 60, 15));
                tabWidget.setVisible(true);

                env.drain();

                // Add the first tab
                auto *page = new QTuiLabel(QStringLiteral("FirstPage"), &tabWidget);
                page->setVisible(true);
                tabWidget.addTab(page, QStringLiteral("First"));

                // The first page should be dirty for initial render
                RC_ASSERT(page->isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.4**
    // AnsiRenderer: Create ScreenBuffer with Unicode chars (U+2500 box-drawing),
    // render, assert output contains UTF-8 bytes \xE2\x94\x80
    // -----------------------------------------------------------------------
    void ansiRendererUnicodeEncoding()
    {
        QVERIFY(rc::check("AnsiRenderer preserves Unicode box-drawing characters as UTF-8",
            []() {
                // Generate a Unicode box-drawing character (U+2500 to U+257F)
                const auto codePoint = *rc::gen::inRange(0x2500, 0x2580);
                QChar unicodeChar(codePoint);

                // Create a small screen buffer and write the character
                ScreenBuffer buffer(5, 1);
                buffer.invalidateAll();
                buffer.swapBuffers();

                // Write the unicode character to the back buffer
                Cell &cell = buffer.backCell(0, 0);
                cell.character = unicodeChar;
                cell.foreground = Color(255, 255, 255);
                cell.background = Color(0, 0, 0);

                // Render
                AnsiRenderer renderer;
                QByteArray output = renderer.render(buffer);

                // The output should contain the UTF-8 encoding of the character
                QByteArray expectedUtf8 = QString(unicodeChar).toUtf8();
                RC_ASSERT(output.contains(expectedUtf8));
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.5**
    // QTuiTableView: Set model, send Down/Up/Left/Right key events,
    // assert isDirty() == true
    // -----------------------------------------------------------------------
    void tableViewNavigationMarksDirty()
    {
        QVERIFY(rc::check("QTuiTableView isDirty() after arrow key navigation",
            []() {
                TestEnv env;

                SimpleTableModel model(10, 3);

                QTuiTableView tableView(&env.mainWindow);
                tableView.setGeometry(QRect(0, 0, 40, 12));
                tableView.setVisible(true);
                tableView.setModel(&model);

                env.drain();
                tableView.clearDirtyRegions();
                env.drain();

                // Generate a random navigation key
                const auto keyIdx = *rc::gen::inRange(0, 4);
                int keys[] = { Qt::Key_Down, Qt::Key_Up, Qt::Key_Left, Qt::Key_Right };
                int key = keys[keyIdx];

                sendKey(tableView, key);

                RC_ASSERT(tableView.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.6**
    // QTuiListView: Set model, send Down/Up key events,
    // assert isDirty() == true
    // -----------------------------------------------------------------------
    void listViewNavigationMarksDirty()
    {
        QVERIFY(rc::check("QTuiListView isDirty() after arrow key navigation",
            []() {
                TestEnv env;

                SimpleListModel model(10);

                QTuiListView listView(&env.mainWindow);
                listView.setGeometry(QRect(0, 0, 40, 10));
                listView.setVisible(true);
                listView.setModel(&model);

                env.drain();
                listView.clearDirtyRegions();
                env.drain();

                // Generate a random navigation key (Up or Down)
                const auto keyIdx = *rc::gen::inRange(0, 2);
                int keys[] = { Qt::Key_Down, Qt::Key_Up };
                int key = keys[keyIdx];

                sendKey(listView, key);

                RC_ASSERT(listView.isDirty());
            }));
    }

    // -----------------------------------------------------------------------
    // **Validates: Requirements 1.7**
    // QTuiCheckBox: Send Space key event, assert isDirty() == true after
    // state change
    // -----------------------------------------------------------------------
    void checkBoxToggleMarksDirty()
    {
        QVERIFY(rc::check("QTuiCheckBox isDirty() after Space key toggles state",
            []() {
                TestEnv env;

                QTuiCheckBox checkBox("Test", &env.mainWindow);
                checkBox.setGeometry(QRect(0, 0, 20, 1));
                checkBox.setVisible(true);

                env.drain();
                checkBox.clearDirtyRegions();
                env.drain();

                // Send Space key to toggle
                sendKey(checkBox, Qt::Key_Space);

                RC_ASSERT(checkBox.isDirty());
            }));
    }
};

QTEST_APPLESS_MAIN(TestDirtyTrackingBugs)
#include "test_dirty_tracking_bugs.moc"
