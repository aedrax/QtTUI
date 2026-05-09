// Feature: widget-rendering-overhaul, Property 28: Focus Change Dirtying
// Feature: widget-rendering-overhaul, Property 29: Focus Styling
#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QAbstractListModel>

using namespace QtTUI;

// Simple list model for testing (Qt6::Core only, no QStandardItemModel)
class SimpleListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit SimpleListModel(const QStringList &items, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_items(items) {}

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

class TestFocusRendering : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 28: Focus Change Dirtying
    // -----------------------------------------------------------------------
    void focusChangeDirtying()
    {
        rc::check("Focus transfer from widget A to widget B marks both dirty",
            []() {
                // **Validates: Requirements 12.1, 12.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Generate random geometries for two labels
                const int x1 = *rc::gen::inRange(0, 61);
                const int y1 = *rc::gen::inRange(0, 61);
                const int w1 = *rc::gen::inRange(1, 21);
                const int h1 = *rc::gen::inRange(1, 21);

                const int x2 = *rc::gen::inRange(0, 61);
                const int y2 = *rc::gen::inRange(0, 61);
                const int w2 = *rc::gen::inRange(1, 21);
                const int h2 = *rc::gen::inRange(1, 21);

                // Labels default to Qt::NoFocus; these tests exercise focus
                // rendering, so opt them into programmatic focus explicitly.
                QTuiLabel label1(QStringLiteral("Label A"), &mainWindow);
                label1.setFocusPolicy(Qt::StrongFocus);
                label1.setGeometry(QRect(x1, y1, w1, h1));

                QTuiLabel label2(QStringLiteral("Label B"), &mainWindow);
                label2.setFocusPolicy(Qt::StrongFocus);
                label2.setGeometry(QRect(x2, y2, w2, h2));

                // Drain initial events from construction
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Give focus to widget A
                label1.setFocus();

                // Drain pending renders so we start from a clean state
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Clear dirty regions on both widgets
                label1.clearDirtyRegions();
                label2.clearDirtyRegions();

                // Drain any pending renders from clearing
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();

                // Verify both are clean before the focus transfer
                RC_ASSERT(!label1.isDirty());
                RC_ASSERT(!label2.isDirty());

                // Transfer focus from widget A to widget B
                label2.setFocus();

                // Verify: label1 (lost focus) should be marked dirty
                RC_ASSERT(label1.isDirty());

                // Verify: label2 (gained focus) should be marked dirty
                RC_ASSERT(label2.isDirty());

                // Verify: scheduler has a pending render
                RC_ASSERT(app.scheduler()->hasPendingRender());
            });
    }

    // -----------------------------------------------------------------------
    // Feature: widget-rendering-overhaul, Property 29: Focus Styling
    // -----------------------------------------------------------------------
    void focusStyling()
    {
        rc::check("Focused widget uses focus colors, unfocused uses normal colors",
            []() {
                // **Validates: Requirements 12.3, 12.4**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                // Create a label with some text
                const int x = *rc::gen::inRange(0, 40);
                const int y = *rc::gen::inRange(0, 10);
                const int w = *rc::gen::inRange(5, 30);
                const int h = *rc::gen::inRange(1, 5);

                // NoFocus labels ignore setFocus(); opt in for this rendering test
                QTuiLabel label(QStringLiteral("Hello"), &mainWindow);
                label.setFocusPolicy(Qt::StrongFocus);
                label.setGeometry(QRect(x, y, w, h));

                ScreenBuffer buffer(80, 24);

                // Get theme properties for comparison
                auto theme = label.themeProperties();

                // --- Render without focus ---
                label.paintEvent(buffer);

                // Check that the first text cell uses normal foreground/background
                Color unfocusedFg = buffer.backCell(x, y).foreground;
                Color unfocusedBg = buffer.backCell(x, y).background;
                RC_ASSERT(unfocusedFg == theme.foreground);
                RC_ASSERT(unfocusedBg == theme.background);

                // --- Give focus and re-render ---
                label.setFocus();

                // Clear buffer and re-render
                for (int row = 0; row < 24; ++row)
                    for (int col = 0; col < 80; ++col) {
                        buffer.backCell(col, row).setChar(U' ');
                        buffer.backCell(col, row).foreground = Color(0, 0, 0);
                        buffer.backCell(col, row).background = Color(0, 0, 0);
                    }

                label.paintEvent(buffer);

                // Check that the first text cell uses focus foreground/background
                Color focusedFg = buffer.backCell(x, y).foreground;
                Color focusedBg = buffer.backCell(x, y).background;
                RC_ASSERT(focusedFg == theme.focusForeground);
                RC_ASSERT(focusedBg == theme.focusBackground);

                // The focused rendering must differ from unfocused rendering
                // (unless theme happens to have same colors, which is unlikely with defaults)
                bool colorsDiffer = (focusedFg != unfocusedFg) || (focusedBg != unfocusedBg);
                RC_ASSERT(colorsDiffer);
            });
    }

    void focusStylingLineEdit()
    {
        rc::check("Focused QTuiLineEdit uses focus colors for all cells",
            []() {
                // **Validates: Requirements 12.3, 12.4**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                const int x = *rc::gen::inRange(0, 40);
                const int y = *rc::gen::inRange(0, 20);
                const int w = *rc::gen::inRange(5, 30);

                QTuiLineEdit lineEdit(&mainWindow);
                lineEdit.setGeometry(QRect(x, y, w, 1));
                lineEdit.setText(QStringLiteral("test"));

                ScreenBuffer buffer(80, 24);
                auto theme = lineEdit.themeProperties();

                // --- Render without focus ---
                lineEdit.paintEvent(buffer);

                // Non-cursor cell should use normal colors
                Color unfocusedFg = buffer.backCell(x, y).foreground;
                Color unfocusedBg = buffer.backCell(x, y).background;
                RC_ASSERT(unfocusedFg == theme.foreground);
                RC_ASSERT(unfocusedBg == theme.background);

                // --- Give focus and re-render ---
                lineEdit.setFocus();

                for (int col = 0; col < 80; ++col) {
                    buffer.backCell(col, y).foreground = Color(0, 0, 0);
                    buffer.backCell(col, y).background = Color(0, 0, 0);
                }

                lineEdit.paintEvent(buffer);

                // Non-cursor cells should use focus colors
                // Check a cell that is NOT the cursor position
                int checkCol = 0; // first char 't'
                if (checkCol == lineEdit.cursorPosition())
                    checkCol = 1;

                Color focusedFg = buffer.backCell(x + checkCol, y).foreground;
                Color focusedBg = buffer.backCell(x + checkCol, y).background;
                RC_ASSERT(focusedFg == theme.focusForeground);
                RC_ASSERT(focusedBg == theme.focusBackground);
            });
    }

    void focusStylingListView()
    {
        rc::check("Focused QTuiListView highlights current row with focus colors",
            []() {
                // **Validates: Requirements 12.3, 12.4, 7.2**
                static int argc = 1;
                static char arg0[] = "test";
                static char *argv[] = { arg0, nullptr };
                QTuiApplication app(argc, argv);

                QTuiMainWindow mainWindow;
                mainWindow.setGeometry(QRect(0, 0, 80, 24));
                mainWindow.setVisible(true);
                app.setMainWindow(&mainWindow);

                const int x = 0;
                const int y = 0;
                const int w = *rc::gen::inRange(10, 40);
                const int h = *rc::gen::inRange(3, 10);

                QTuiListView listView(&mainWindow);
                listView.setGeometry(QRect(x, y, w, h));

                SimpleListModel model({
                    QStringLiteral("Item 1"),
                    QStringLiteral("Item 2"),
                    QStringLiteral("Item 3")
                });
                listView.setModel(&model);

                ScreenBuffer buffer(80, 24);
                auto theme = listView.themeProperties();

                // --- Render without focus ---
                listView.paintEvent(buffer);

                // Current row (row 0) should NOT use focus colors when unfocused
                Color unfocusedFg = buffer.backCell(x, y).foreground;
                Color unfocusedBg = buffer.backCell(x, y).background;
                RC_ASSERT(unfocusedFg == theme.foreground);
                RC_ASSERT(unfocusedBg == theme.background);

                // --- Give focus and re-render ---
                listView.setFocus();

                // Clear buffer
                for (int row = 0; row < 24; ++row)
                    for (int col = 0; col < 80; ++col) {
                        buffer.backCell(col, row).setChar(U' ');
                        buffer.backCell(col, row).foreground = Color(0, 0, 0);
                        buffer.backCell(col, row).background = Color(0, 0, 0);
                    }

                listView.paintEvent(buffer);

                // Current row (row 0) should use focus colors when focused
                Color focusedFg = buffer.backCell(x, y).foreground;
                Color focusedBg = buffer.backCell(x, y).background;
                RC_ASSERT(focusedFg == theme.focusForeground);
                RC_ASSERT(focusedBg == theme.focusBackground);

                // Non-current row should still use normal colors
                Color nonCurrentFg = buffer.backCell(x, y + 1).foreground;
                Color nonCurrentBg = buffer.backCell(x, y + 1).background;
                RC_ASSERT(nonCurrentFg == theme.foreground);
                RC_ASSERT(nonCurrentBg == theme.background);
            });
    }
};

QTEST_APPLESS_MAIN(TestFocusRendering)
#include "test_focus_rendering.moc"
