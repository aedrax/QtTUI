#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiGroupBox.h>
#include <QtTUI/QTuiCollapsible.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

// Simple test child widget
class ContainerTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit ContainerTestWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        for (int row = 0; row < geo.height(); ++row) {
            for (int col = 0; col < geo.width(); ++col) {
                int px = geo.x() + col;
                int py = geo.y() + row;
                if (px < buffer.width() && py < buffer.height()) {
                    buffer.backCell(px, py).setChar(U'X');
                }
            }
        }
    }
    QSize minimumSize() const override { return QSize(3, 1); }
};

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

static void sendMousePress(QTuiWidget &w, const QPoint &pos)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, pos);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiContainerWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiGroupBox tests
    // ---------------------------------------------------------------

    void groupBox_defaultConstruction()
    {
        QTuiGroupBox gb;
        QCOMPARE(gb.title(), QString());
        QCOMPARE(gb.isCheckable(), false);
        QCOMPARE(gb.isChecked(), true);
        QVERIFY(gb.layout() == nullptr);
    }

    void groupBox_constructionWithTitle()
    {
        QTuiGroupBox gb(QStringLiteral("Settings"));
        QCOMPARE(gb.title(), QStringLiteral("Settings"));
    }

    void groupBox_titleProperty()
    {
        QTuiGroupBox gb;
        gb.setTitle(QStringLiteral("Options"));
        QCOMPARE(gb.title(), QStringLiteral("Options"));
    }

    void groupBox_checkableProperty()
    {
        QTuiGroupBox gb;
        QCOMPARE(gb.isCheckable(), false);
        gb.setCheckable(true);
        QCOMPARE(gb.isCheckable(), true);
    }

    void groupBox_checkedProperty()
    {
        QTuiGroupBox gb;
        QCOMPARE(gb.isChecked(), true);
        gb.setChecked(false);
        QCOMPARE(gb.isChecked(), false);
    }

    void groupBox_toggledSignalEmitted()
    {
        QTuiGroupBox gb;
        gb.setCheckable(true);

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);
        gb.setChecked(false);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);

        gb.setChecked(true);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toBool(), true);
    }

    void groupBox_noSignalWhenSameValue()
    {
        QTuiGroupBox gb;
        gb.setCheckable(true);
        gb.setChecked(true); // already true

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);
        gb.setChecked(true);
        QCOMPARE(spy.count(), 0);
    }

    void groupBox_checkableToggleDisablesChildren()
    {
        QTuiGroupBox gb(QStringLiteral("Group"));
        gb.setCheckable(true);
        gb.setChecked(true);

        auto *child1 = new ContainerTestWidget(&gb);
        auto *child2 = new ContainerTestWidget(&gb);

        QCOMPARE(child1->isEnabled(), true);
        QCOMPARE(child2->isEnabled(), true);

        // Uncheck - children are disabled but stay visible (matches
        // QGroupBox; they previously were hidden, which was a defect)
        gb.setChecked(false);
        QCOMPARE(child1->isEnabled(), false);
        QCOMPARE(child2->isEnabled(), false);
        QCOMPARE(child1->isVisible(), true);
        QCOMPARE(child2->isVisible(), true);

        // Re-check - children should be enabled again
        gb.setChecked(true);
        QCOMPARE(child1->isEnabled(), true);
        QCOMPARE(child2->isEnabled(), true);
    }

    void groupBox_keyPressTogglesCheckable()
    {
        QTuiGroupBox gb(QStringLiteral("Group"));
        gb.setCheckable(true);
        gb.setChecked(true);

        auto *child = new ContainerTestWidget(&gb);
        QCOMPARE(child->isEnabled(), true);

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);

        // Space toggles; children are disabled, not hidden (matches QGroupBox)
        sendKey(gb, Qt::Key_Space);
        QCOMPARE(gb.isChecked(), false);
        QCOMPARE(child->isEnabled(), false);
        QCOMPARE(child->isVisible(), true);
        QCOMPARE(spy.count(), 1);

        // Enter toggles back
        sendKey(gb, Qt::Key_Return);
        QCOMPARE(gb.isChecked(), true);
        QCOMPARE(child->isEnabled(), true);
        QCOMPARE(spy.count(), 2);
    }

    void groupBox_keyPressIgnoredWhenNotCheckable()
    {
        QTuiGroupBox gb(QStringLiteral("Group"));
        gb.setCheckable(false);
        gb.setChecked(true);

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);
        sendKey(gb, Qt::Key_Space);
        QCOMPARE(spy.count(), 0);
        QCOMPARE(gb.isChecked(), true);
    }

    void groupBox_setLayout()
    {
        QTuiGroupBox gb(QStringLiteral("Group"));
        auto *layout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        gb.setLayout(layout);
        QCOMPARE(gb.layout(), layout);
    }

    void groupBox_minimumSize()
    {
        QTuiGroupBox gb(QStringLiteral("Test"));
        QSize ms = gb.minimumSize();
        // Title "Test" = 4 chars + 2 space padding + 2 border = 8
        QVERIFY(ms.width() >= 8);
        QVERIFY(ms.height() >= 3);
    }

    void groupBox_minimumSizeIncludesLayout()
    {
        // Two 3x1 children in a vertical layout need 2 rows inside the
        // border, so the group box needs 4 rows, not the 3-row default
        QTuiGroupBox gb(QStringLiteral("G"));
        auto *layout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        layout->addWidget(new ContainerTestWidget(&gb));
        layout->addWidget(new ContainerTestWidget(&gb));
        gb.setLayout(layout);
        QCOMPARE(gb.minimumSize().height(), 4);
        QVERIFY(gb.minimumSize().width() >= 5);
    }

    void groupBox_minimumSizeCheckable()
    {
        QTuiGroupBox gb(QStringLiteral("Test"));
        gb.setCheckable(true);
        QSize ms = gb.minimumSize();
        // "[x] Test" = 8 chars + 2 space padding + 2 border = 12
        QVERIFY(ms.width() >= 12);
    }

    void groupBox_paintEventRendersBorder()
    {
        QTuiGroupBox gb(QStringLiteral("Box"));
        gb.setGeometry(QRect(0, 0, 10, 5));

        ScreenBuffer buffer(10, 5);
        gb.paintEvent(buffer);

        // Top-left corner should be a border character (not space)
        const Cell &topLeft = buffer.backCell(0, 0);
        QVERIFY(topLeft.ch != U' ');

        // Bottom-right corner should be a border character
        const Cell &bottomRight = buffer.backCell(9, 4);
        QVERIFY(bottomRight.ch != U' ');

        // Title "Box" should appear in top border
        // Title starts at col 2
        bool foundTitle = false;
        QString topRow;
        for (int col = 0; col < 10; ++col) {
            topRow += buffer.backCell(col, 0).text();
        }
        if (topRow.contains(QStringLiteral("Box")))
            foundTitle = true;
        QVERIFY(foundTitle);
    }

    void groupBox_paintEventCheckableTitle()
    {
        QTuiGroupBox gb(QStringLiteral("Opt"));
        gb.setCheckable(true);
        gb.setChecked(true);
        gb.setGeometry(QRect(0, 0, 15, 5));

        ScreenBuffer buffer(15, 5);
        gb.paintEvent(buffer);

        // Top row should contain "[x] Opt"
        QString topRow;
        for (int col = 0; col < 15; ++col) {
            topRow += buffer.backCell(col, 0).text();
        }
        QVERIFY(topRow.contains(QStringLiteral("[x]")));
        QVERIFY(topRow.contains(QStringLiteral("Opt")));
    }

    void groupBox_paintEventUncheckedTitle()
    {
        QTuiGroupBox gb(QStringLiteral("Opt"));
        gb.setCheckable(true);
        gb.setChecked(false);
        gb.setGeometry(QRect(0, 0, 15, 5));

        ScreenBuffer buffer(15, 5);
        gb.paintEvent(buffer);

        QString topRow;
        for (int col = 0; col < 15; ++col) {
            topRow += buffer.backCell(col, 0).text();
        }
        QVERIFY(topRow.contains(QStringLiteral("[ ]")));
    }

    void groupBox_mousePressOnTitleRowToggles()
    {
        QTuiGroupBox gb(QStringLiteral("Opt"));
        gb.setCheckable(true);
        gb.setChecked(true);
        gb.setGeometry(QRect(0, 0, 15, 5));

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);

        // Painted title span is "[x] Opt" (7 chars) on row 0, columns 2..8.
        sendMousePress(gb, QPoint(2, 0)); // first '[' cell
        QCOMPARE(gb.isChecked(), false);
        QCOMPARE(spy.count(), 1);

        sendMousePress(gb, QPoint(8, 0)); // last title char 't'
        QCOMPARE(gb.isChecked(), true);
        QCOMPARE(spy.count(), 2);
    }

    void groupBox_mousePressOutsideTitleSpanDoesNotToggle()
    {
        QTuiGroupBox gb(QStringLiteral("Opt"));
        gb.setCheckable(true);
        gb.setChecked(true);
        gb.setGeometry(QRect(0, 0, 15, 5));

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);

        // Content area (inside the border, padding around children)
        sendMousePress(gb, QPoint(5, 2));
        // Bottom border
        sendMousePress(gb, QPoint(5, 4));
        // Title row but left of the title span (border corner)
        sendMousePress(gb, QPoint(0, 0));
        sendMousePress(gb, QPoint(1, 0));
        // Title row but right of the "[x] Opt" span (span ends at col 8)
        sendMousePress(gb, QPoint(9, 0));
        sendMousePress(gb, QPoint(13, 0));

        QCOMPARE(gb.isChecked(), true);
        QCOMPARE(spy.count(), 0);
    }

    void groupBox_mousePressIgnoredWhenNotCheckable()
    {
        QTuiGroupBox gb(QStringLiteral("Opt"));
        gb.setCheckable(false);
        gb.setGeometry(QRect(0, 0, 15, 5));

        QSignalSpy spy(&gb, &QTuiGroupBox::toggled);
        sendMousePress(gb, QPoint(3, 0)); // on the title span
        QCOMPARE(gb.isChecked(), true);
        QCOMPARE(spy.count(), 0);
    }

    void groupBox_mousePressUsesAbsoluteCoordinates()
    {
        // Geometry offset from origin: hit-test must use absolute coords.
        QTuiGroupBox gb(QStringLiteral("Opt"));
        gb.setCheckable(true);
        gb.setChecked(true);
        gb.setGeometry(QRect(5, 3, 15, 5));

        // Title span "[x] Opt" on row 3, columns 7..13.
        sendMousePress(gb, QPoint(7, 3));
        QCOMPARE(gb.isChecked(), false);

        // Same column, one row down: no toggle.
        sendMousePress(gb, QPoint(7, 4));
        QCOMPARE(gb.isChecked(), false);
    }

    // ---------------------------------------------------------------
    // QTuiCollapsible tests
    // ---------------------------------------------------------------

    void collapsible_defaultConstruction()
    {
        QTuiCollapsible col;
        QCOMPARE(col.title(), QString());
        QCOMPARE(col.isCollapsed(), false);
        QCOMPARE(col.isExpanded(), true);
    }

    void collapsible_constructionWithTitle()
    {
        QTuiCollapsible col(QStringLiteral("Details"));
        QCOMPARE(col.title(), QStringLiteral("Details"));
    }

    void collapsible_titleProperty()
    {
        QTuiCollapsible col;
        col.setTitle(QStringLiteral("Info"));
        QCOMPARE(col.title(), QStringLiteral("Info"));
    }

    void collapsible_collapsedExpandedState()
    {
        QTuiCollapsible col;
        QCOMPARE(col.isCollapsed(), false);
        QCOMPARE(col.isExpanded(), true);

        col.setCollapsed(true);
        QCOMPARE(col.isCollapsed(), true);
        QCOMPARE(col.isExpanded(), false);

        col.setExpanded(true);
        QCOMPARE(col.isCollapsed(), false);
        QCOMPARE(col.isExpanded(), true);
    }

    void collapsible_toggledSignalEmitted()
    {
        QTuiCollapsible col;

        QSignalSpy spy(&col, &QTuiCollapsible::toggled);

        col.setCollapsed(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false); // expanded = false

        col.setCollapsed(false);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toBool(), true); // expanded = true
    }

    void collapsible_noSignalWhenSameState()
    {
        QTuiCollapsible col;
        col.setCollapsed(false); // already expanded

        QSignalSpy spy(&col, &QTuiCollapsible::toggled);
        col.setCollapsed(false);
        QCOMPARE(spy.count(), 0);
    }

    void collapsible_collapseHidesChildren()
    {
        QTuiCollapsible col(QStringLiteral("Section"));

        auto *child1 = new ContainerTestWidget(&col);
        auto *child2 = new ContainerTestWidget(&col);

        QCOMPARE(child1->isVisible(), true);
        QCOMPARE(child2->isVisible(), true);

        col.setCollapsed(true);
        QCOMPARE(child1->isVisible(), false);
        QCOMPARE(child2->isVisible(), false);

        col.setExpanded(true);
        QCOMPARE(child1->isVisible(), true);
        QCOMPARE(child2->isVisible(), true);
    }

    void collapsible_expandedChildrenGetRowsBelowTitle()
    {
        QTuiCollapsible col(QStringLiteral("Section"));
        auto *child1 = new ContainerTestWidget(&col);
        auto *child2 = new ContainerTestWidget(&col);
        // Title row + 2 children of 1 row each
        QCOMPARE(col.minimumSize().height(), 3);

        col.setGeometry(QRect(2, 5, 20, 3));
        ScreenBuffer buf(30, 10);
        col.paintEvent(buf);

        QCOMPARE(child1->geometry(), QRect(2, 6, 20, 1));
        QCOMPARE(child2->geometry(), QRect(2, 7, 20, 1));
        QCOMPARE(buf.backCell(2, 6).ch, U'X');
        QCOMPARE(buf.backCell(2, 7).ch, U'X');
    }

    void collapsible_expandCollapseChangesMinimumHeight()
    {
        QTuiCollapsible col(QStringLiteral("Section"));

        QSize expandedMin = col.minimumSize();

        col.setCollapsed(true);
        QSize collapsedMin = col.minimumSize();

        // Collapsed should have height 1 (title bar only)
        QCOMPARE(collapsedMin.height(), 1);
        // Expanded should be taller
        QVERIFY(expandedMin.height() > collapsedMin.height());
    }

    void collapsible_keyPressTogglesState()
    {
        QTuiCollapsible col(QStringLiteral("Section"));

        auto *child = new ContainerTestWidget(&col);
        QCOMPARE(col.isExpanded(), true);
        QCOMPARE(child->isVisible(), true);

        QSignalSpy spy(&col, &QTuiCollapsible::toggled);

        // Space collapses
        sendKey(col, Qt::Key_Space);
        QCOMPARE(col.isCollapsed(), true);
        QCOMPARE(child->isVisible(), false);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);

        // Enter expands
        sendKey(col, Qt::Key_Return);
        QCOMPARE(col.isExpanded(), true);
        QCOMPARE(child->isVisible(), true);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toBool(), true);
    }

    void collapsible_paintEventShowsIndicator()
    {
        QTuiCollapsible col(QStringLiteral("Info"));
        col.setGeometry(QRect(0, 0, 15, 5));

        ScreenBuffer buffer(15, 5);
        col.paintEvent(buffer);

        // Title bar should contain ▼ (expanded indicator) and "Info"
        QString titleRow;
        for (int col2 = 0; col2 < 15; ++col2) {
            titleRow += buffer.backCell(col2, 0).text();
        }
        QVERIFY(titleRow.contains(QChar(0x25BC))); // ▼
        QVERIFY(titleRow.contains(QStringLiteral("Info")));
    }

    void collapsible_paintEventCollapsedIndicator()
    {
        QTuiCollapsible col(QStringLiteral("Info"));
        col.setCollapsed(true);
        col.setGeometry(QRect(0, 0, 15, 5));

        ScreenBuffer buffer(15, 5);
        col.paintEvent(buffer);

        // Title bar should contain ▶ (collapsed indicator)
        QString titleRow;
        for (int col2 = 0; col2 < 15; ++col2) {
            titleRow += buffer.backCell(col2, 0).text();
        }
        QVERIFY(titleRow.contains(QChar(0x25B6))); // ▶
        QVERIFY(titleRow.contains(QStringLiteral("Info")));
    }

    void collapsible_mousePressOnHeaderRowToggles()
    {
        QTuiCollapsible col(QStringLiteral("Info"));
        col.setGeometry(QRect(0, 0, 15, 5));
        QCOMPARE(col.isExpanded(), true);

        QSignalSpy spy(&col, &QTuiCollapsible::toggled);

        // Header bar occupies the entire top row (row 0).
        sendMousePress(col, QPoint(4, 0));
        QCOMPARE(col.isCollapsed(), true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false); // expanded = false

        sendMousePress(col, QPoint(14, 0)); // far end of the header row
        QCOMPARE(col.isExpanded(), true);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toBool(), true);
    }

    void collapsible_mousePressOnContentAreaDoesNotToggle()
    {
        QTuiCollapsible col(QStringLiteral("Info"));
        col.setGeometry(QRect(0, 0, 15, 5));

        auto *child = new ContainerTestWidget(&col);
        QCOMPARE(child->isVisible(), true);

        QSignalSpy spy(&col, &QTuiCollapsible::toggled);

        // Clicks below the header row must not collapse.
        sendMousePress(col, QPoint(4, 1));
        sendMousePress(col, QPoint(4, 2));
        sendMousePress(col, QPoint(14, 4));

        QCOMPARE(col.isExpanded(), true);
        QCOMPARE(child->isVisible(), true);
        QCOMPARE(spy.count(), 0);
    }

    void collapsible_mousePressUsesAbsoluteCoordinates()
    {
        // Geometry offset from origin: hit-test must use absolute coords.
        QTuiCollapsible col(QStringLiteral("Info"));
        col.setGeometry(QRect(2, 4, 15, 5));

        // Header row is absolute row 4.
        sendMousePress(col, QPoint(5, 4));
        QCOMPARE(col.isCollapsed(), true);

        // Same column inside the (now hidden) content area: no toggle.
        sendMousePress(col, QPoint(5, 6));
        QCOMPARE(col.isCollapsed(), true);
    }
};

QTEST_MAIN(TestQTuiContainerWidgets)
#include "test_qtui_container_widgets.moc"
