#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Simple child widget for scroll area tests
class ScrollTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit ScrollTestWidget(QSize minSize = QSize(0, 0), QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        for (int row = 0; row < geo.height(); ++row) {
            for (int col = 0; col < geo.width(); ++col) {
                int px = geo.x() + col;
                int py = geo.y() + row;
                if (px < buffer.width() && py < buffer.height()) {
                    Cell &cell = buffer.backCell(px, py);
                    // Fill with a character based on position for verification
                    cell.setChar(char32_t('A' + ((row + col) % 26)));
                }
            }
        }
    }
    QSize minimumSize() const override { return m_minSize; }
private:
    QSize m_minSize;
};

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestQTuiScrollWidgets : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiScrollBar tests
    // ---------------------------------------------------------------

    void scrollBar_constructionVertical()
    {
        QTuiScrollBar sb(Qt::Vertical);
        QCOMPARE(sb.orientation(), Qt::Vertical);
        QCOMPARE(sb.value(), 0);
        QCOMPARE(sb.minimum(), 0);
        QCOMPARE(sb.maximum(), 0);
        QCOMPARE(sb.singleStep(), 1);
        QCOMPARE(sb.pageStep(), 10);
    }

    void scrollBar_constructionHorizontal()
    {
        QTuiScrollBar sb(Qt::Horizontal);
        QCOMPARE(sb.orientation(), Qt::Horizontal);
        QCOMPARE(sb.value(), 0);
        QCOMPARE(sb.minimum(), 0);
        QCOMPARE(sb.maximum(), 0);
    }

    void scrollBar_valueClampingAboveMax()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);
        sb.setValue(150);
        QCOMPARE(sb.value(), 100);
    }

    void scrollBar_valueClampingBelowMin()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(10, 100);
        sb.setValue(5);
        QCOMPARE(sb.value(), 10);
    }

    void scrollBar_setRangeUpdatesMinMax()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(20, 80);
        QCOMPARE(sb.minimum(), 20);
        QCOMPARE(sb.maximum(), 80);
    }

    void scrollBar_setRangeReclampsValue()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);
        sb.setValue(90);
        QCOMPARE(sb.value(), 90);

        // Shrink range so value is out of bounds
        sb.setRange(0, 50);
        QCOMPARE(sb.value(), 50);
    }

    void scrollBar_setRangeMinGreaterThanMax()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(50, 10); // min > max
        QVERIFY(sb.maximum() >= sb.minimum());
    }

    void scrollBar_singleStepProperty()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setSingleStep(5);
        QCOMPARE(sb.singleStep(), 5);

        // Zero or negative step should be ignored
        sb.setSingleStep(0);
        QCOMPARE(sb.singleStep(), 5);
        sb.setSingleStep(-1);
        QCOMPARE(sb.singleStep(), 5);
    }

    void scrollBar_pageStepProperty()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setPageStep(20);
        QCOMPARE(sb.pageStep(), 20);

        // Zero or negative step should be ignored
        sb.setPageStep(0);
        QCOMPARE(sb.pageStep(), 20);
    }

    void scrollBar_keyPressVerticalUpDown()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);
        sb.setSingleStep(3);
        sb.setValue(50);

        sendKey(sb, Qt::Key_Up);
        QCOMPARE(sb.value(), 47);

        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 50);
    }

    void scrollBar_keyPressVerticalPageUpDown()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);
        sb.setPageStep(15);
        sb.setValue(50);

        sendKey(sb, Qt::Key_PageUp);
        QCOMPARE(sb.value(), 35);

        sendKey(sb, Qt::Key_PageDown);
        QCOMPARE(sb.value(), 50);
    }

    void scrollBar_keyPressHorizontalLeftRight()
    {
        QTuiScrollBar sb(Qt::Horizontal);
        sb.setRange(0, 100);
        sb.setSingleStep(5);
        sb.setValue(50);

        sendKey(sb, Qt::Key_Left);
        QCOMPARE(sb.value(), 45);

        sendKey(sb, Qt::Key_Right);
        QCOMPARE(sb.value(), 50);
    }

    void scrollBar_keyPressHorizontalPageUpDown()
    {
        QTuiScrollBar sb(Qt::Horizontal);
        sb.setRange(0, 100);
        sb.setPageStep(20);
        sb.setValue(50);

        sendKey(sb, Qt::Key_PageUp);
        QCOMPARE(sb.value(), 30);

        sendKey(sb, Qt::Key_PageDown);
        QCOMPARE(sb.value(), 50);
    }

    void scrollBar_keyPressClampsToBounds()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 10);
        sb.setSingleStep(5);
        sb.setValue(3);

        // Down past max should clamp
        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 8);
        sendKey(sb, Qt::Key_Down);
        QCOMPARE(sb.value(), 10);

        // Up past min should clamp
        sb.setValue(2);
        sendKey(sb, Qt::Key_Up);
        QCOMPARE(sb.value(), 0);
    }

    void scrollBar_valueChangedSignalEmitted()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);

        QSignalSpy spy(&sb, &QTuiScrollBar::valueChanged);

        sb.setValue(42);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);
    }

    void scrollBar_noSignalWhenValueUnchanged()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);
        sb.setValue(42);

        QSignalSpy spy(&sb, &QTuiScrollBar::valueChanged);
        sb.setValue(42); // same value
        QCOMPARE(spy.count(), 0);
    }

    void scrollBar_paintEventVertical()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setRange(0, 100);
        sb.setPageStep(10);
        sb.setValue(0);
        sb.setGeometry(QRect(0, 0, 1, 10));

        ScreenBuffer buffer(1, 10);
        sb.paintEvent(buffer);

        // Thumb should be at the top; track chars elsewhere
        // Thumb char is █ (0x2588), track char is │ (0x2502)
        bool foundThumb = false;
        bool foundTrack = false;
        for (int row = 0; row < 10; ++row) {
            const Cell &cell = buffer.backCell(0, row);
            if (cell.ch == 0x2588)
                foundThumb = true;
            else if (cell.ch == 0x2502)
                foundTrack = true;
        }
        QVERIFY(foundThumb);
        QVERIFY(foundTrack);
    }

    void scrollBar_paintEventHorizontal()
    {
        QTuiScrollBar sb(Qt::Horizontal);
        sb.setRange(0, 100);
        sb.setPageStep(10);
        sb.setValue(0);
        sb.setGeometry(QRect(0, 0, 10, 1));

        ScreenBuffer buffer(10, 1);
        sb.paintEvent(buffer);

        // Thumb char is █ (0x2588), track char is ─ (0x2500)
        bool foundThumb = false;
        bool foundTrack = false;
        for (int col = 0; col < 10; ++col) {
            const Cell &cell = buffer.backCell(col, 0);
            if (cell.ch == 0x2588)
                foundThumb = true;
            else if (cell.ch == 0x2500)
                foundTrack = true;
        }
        QVERIFY(foundThumb);
        QVERIFY(foundTrack);
    }

    void scrollBar_minimumSizeVertical()
    {
        QTuiScrollBar sb(Qt::Vertical);
        QSize ms = sb.minimumSize();
        QCOMPARE(ms, QSize(1, 3));
    }

    void scrollBar_minimumSizeHorizontal()
    {
        QTuiScrollBar sb(Qt::Horizontal);
        QSize ms = sb.minimumSize();
        QCOMPARE(ms, QSize(3, 1));
    }

    // ---------------------------------------------------------------
    // QTuiScrollArea tests
    // ---------------------------------------------------------------

    void scrollArea_defaultConstruction()
    {
        QTuiScrollArea sa;
        QVERIFY(sa.widget() == nullptr);
        QVERIFY(sa.verticalScrollBar() != nullptr);
        QVERIFY(sa.horizontalScrollBar() != nullptr);
        // Scroll bars should be hidden initially
        QCOMPARE(sa.verticalScrollBar()->isVisible(), false);
        QCOMPARE(sa.horizontalScrollBar()->isVisible(), false);
    }

    void scrollArea_setWidgetStoresWidget()
    {
        QTuiScrollArea sa;
        auto *child = new ScrollTestWidget(QSize(10, 10));
        child->setGeometry(QRect(0, 0, 10, 10));

        sa.setWidget(child);
        QCOMPARE(sa.widget(), child);
    }

    void scrollArea_scrollBarsAppearWhenChildLarger()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 10, 10));

        // Child larger than scroll area
        auto *child = new ScrollTestWidget(QSize(20, 20));
        child->setGeometry(QRect(0, 0, 20, 20));
        sa.setWidget(child);

        // Trigger updateScrollBars via paintEvent
        ScreenBuffer buffer(10, 10);
        sa.paintEvent(buffer);

        QCOMPARE(sa.verticalScrollBar()->isVisible(), true);
        QCOMPARE(sa.horizontalScrollBar()->isVisible(), true);
    }

    void scrollArea_scrollBarsHiddenWhenChildFits()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 20, 20));

        // Child fits within scroll area
        auto *child = new ScrollTestWidget(QSize(10, 10));
        child->setGeometry(QRect(0, 0, 10, 10));
        sa.setWidget(child);

        ScreenBuffer buffer(20, 20);
        sa.paintEvent(buffer);

        QCOMPARE(sa.verticalScrollBar()->isVisible(), false);
        QCOMPARE(sa.horizontalScrollBar()->isVisible(), false);
    }

    void scrollArea_scrollBarsOnlyVertical()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 20, 10));

        // Child taller but not wider
        auto *child = new ScrollTestWidget(QSize(15, 25));
        child->setGeometry(QRect(0, 0, 15, 25));
        sa.setWidget(child);

        ScreenBuffer buffer(20, 10);
        sa.paintEvent(buffer);

        QCOMPARE(sa.verticalScrollBar()->isVisible(), true);
        // Horizontal may or may not be needed depending on whether the vertical
        // scroll bar reduces viewport width enough. With width 20 and child 15,
        // even with 1-col vscroll bar, 19 > 15, so hscroll should be hidden.
        QCOMPARE(sa.horizontalScrollBar()->isVisible(), false);
    }

    void scrollArea_keyboardScrollingArrowKeys()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 10, 10));

        auto *child = new ScrollTestWidget(QSize(20, 20));
        child->setGeometry(QRect(0, 0, 20, 20));
        sa.setWidget(child);

        // Trigger initial layout
        ScreenBuffer buffer(10, 10);
        sa.paintEvent(buffer);

        QCOMPARE(sa.verticalScrollBar()->value(), 0);

        // Arrow down should scroll vertically
        sendKey(sa, Qt::Key_Down);
        QCOMPARE(sa.verticalScrollBar()->value(), 1);

        sendKey(sa, Qt::Key_Up);
        QCOMPARE(sa.verticalScrollBar()->value(), 0);

        // Arrow right should scroll horizontally
        sendKey(sa, Qt::Key_Right);
        QCOMPARE(sa.horizontalScrollBar()->value(), 1);

        sendKey(sa, Qt::Key_Left);
        QCOMPARE(sa.horizontalScrollBar()->value(), 0);
    }

    void scrollArea_keyboardScrollingPageUpDown()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 10, 10));

        auto *child = new ScrollTestWidget(QSize(20, 40));
        child->setGeometry(QRect(0, 0, 20, 40));
        sa.setWidget(child);

        ScreenBuffer buffer(10, 10);
        sa.paintEvent(buffer);

        int viewportH = 10 - 1; // minus horizontal scroll bar row
        int pageStep = sa.verticalScrollBar()->pageStep();

        sendKey(sa, Qt::Key_PageDown);
        QCOMPARE(sa.verticalScrollBar()->value(), pageStep);

        sendKey(sa, Qt::Key_PageUp);
        QCOMPARE(sa.verticalScrollBar()->value(), 0);
    }

    void scrollArea_keyboardScrollingHomeEnd()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 10, 10));

        auto *child = new ScrollTestWidget(QSize(20, 30));
        child->setGeometry(QRect(0, 0, 20, 30));
        sa.setWidget(child);

        ScreenBuffer buffer(10, 10);
        sa.paintEvent(buffer);

        int maxV = sa.verticalScrollBar()->maximum();
        QVERIFY(maxV > 0);

        sendKey(sa, Qt::Key_End);
        QCOMPARE(sa.verticalScrollBar()->value(), maxV);

        sendKey(sa, Qt::Key_Home);
        QCOMPARE(sa.verticalScrollBar()->value(), 0);
        QCOMPARE(sa.horizontalScrollBar()->value(), 0);
    }

    void scrollArea_ensureVisibleScrollsToPosition()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 10, 10));

        auto *child = new ScrollTestWidget(QSize(30, 30));
        child->setGeometry(QRect(0, 0, 30, 30));
        sa.setWidget(child);

        // Trigger layout
        ScreenBuffer buffer(10, 10);
        sa.paintEvent(buffer);

        // Position (25, 25) is outside the initial viewport
        sa.ensureVisible(25, 25);

        // After ensureVisible, the scroll should have moved so (25,25) is visible
        QVERIFY(sa.verticalScrollBar()->value() > 0);
        QVERIFY(sa.horizontalScrollBar()->value() > 0);
    }

    void scrollArea_viewportClipping()
    {
        QTuiScrollArea sa;
        sa.setGeometry(QRect(0, 0, 10, 10));

        // Child is 20x20, larger than the 10x10 area
        auto *child = new ScrollTestWidget(QSize(20, 20));
        child->setGeometry(QRect(0, 0, 20, 20));
        sa.setWidget(child);

        ScreenBuffer buffer(10, 10);
        sa.paintEvent(buffer);

        // The viewport area (excluding scroll bars) should contain child content
        // With both scroll bars visible: viewport is (10-1) x (10-1) = 9x9
        bool hasChildContent = false;
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                const Cell &cell = buffer.backCell(col, row);
                // Our test widget fills with 'A' + ((row+col) % 26)
                const char32_t expected = char32_t('A' + ((row + col) % 26));
                if (cell.ch == expected) {
                    hasChildContent = true;
                }
            }
        }
        QVERIFY(hasChildContent);

        // The scroll bar column (col 9) should have scroll bar characters, not child content
        bool hasScrollBarChar = false;
        for (int row = 0; row < 9; ++row) {
            const Cell &cell = buffer.backCell(9, row);
            if (cell.ch == 0x2588 || cell.ch == 0x2502) {
                hasScrollBarChar = true;
                break;
            }
        }
        QVERIFY(hasScrollBarChar);
    }

    void scrollBar_dragFollowsMouse()
    {
        QTuiScrollBar sb(Qt::Vertical);
        sb.setGeometry(QRect(0, 0, 1, 11));
        sb.setRange(0, 100);
        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(0, 0));
        QCoreApplication::sendEvent(&sb, &press);
        QCOMPARE(sb.value(), 0);
        TuiMouseEvent drag(TuiMouseEvent::Move, Qt::LeftButton, QPoint(0, 5));
        QCoreApplication::sendEvent(&sb, &drag);
        QCOMPARE(sb.value(), 50);
        // Beyond the end (the grab sends positions outside the bar)
        TuiMouseEvent past(TuiMouseEvent::Move, Qt::LeftButton, QPoint(0, 30));
        QCoreApplication::sendEvent(&sb, &past);
        QCOMPARE(sb.value(), 100);
        TuiMouseEvent hover(TuiMouseEvent::Move, Qt::NoButton, QPoint(0, 2));
        QCoreApplication::sendEvent(&sb, &hover);
        QCOMPARE(sb.value(), 100);
    }
};

QTEST_MAIN(TestQTuiScrollWidgets)
#include "test_qtui_scroll_widgets.moc"
