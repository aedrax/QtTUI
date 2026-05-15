#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiStackedWidget.h>
#include <QtTUI/QTuiSplitter.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Simple concrete child widget for testing
class SplitterTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit SplitterTestWidget(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(2, 1) {}
    SplitterTestWidget(QSize minSize, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        for (int row = 0; row < geo.height(); ++row) {
            for (int col = 0; col < geo.width(); ++col) {
                int px = geo.x() + col;
                int py = geo.y() + row;
                if (px < buffer.width() && py < buffer.height())
                    buffer.backCell(px, py).setChar(U'X');
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

class TestStackedSplitter : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiStackedWidget
    // ---------------------------------------------------------------

    void stacked_defaultConstruction()
    {
        QTuiStackedWidget sw;
        QCOMPARE(sw.count(), 0);
        QCOMPARE(sw.currentIndex(), -1);
        QVERIFY(sw.currentWidget() == nullptr);
    }

    void stacked_addWidget()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;

        int idx1 = sw.addWidget(w1);
        QCOMPARE(idx1, 0);
        QCOMPARE(sw.count(), 1);
        QCOMPARE(sw.currentIndex(), 0);
        QCOMPARE(sw.currentWidget(), w1);
        QCOMPARE(w1->isVisible(), true);

        int idx2 = sw.addWidget(w2);
        QCOMPARE(idx2, 1);
        QCOMPARE(sw.count(), 2);
        // First widget stays current
        QCOMPARE(sw.currentIndex(), 0);
        QCOMPARE(w2->isVisible(), false);
    }

    void stacked_setCurrentIndex()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        auto *w3 = new SplitterTestWidget;
        sw.addWidget(w1);
        sw.addWidget(w2);
        sw.addWidget(w3);

        QSignalSpy spy(&sw, &QTuiStackedWidget::currentChanged);

        sw.setCurrentIndex(2);
        QCOMPARE(sw.currentIndex(), 2);
        QCOMPARE(sw.currentWidget(), w3);
        QCOMPARE(w1->isVisible(), false);
        QCOMPARE(w2->isVisible(), false);
        QCOMPARE(w3->isVisible(), true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 2);
    }

    void stacked_setCurrentWidget()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sw.addWidget(w1);
        sw.addWidget(w2);

        sw.setCurrentWidget(w2);
        QCOMPARE(sw.currentIndex(), 1);
        QCOMPARE(sw.currentWidget(), w2);
    }

    void stacked_setCurrentIndexOutOfRange()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        sw.addWidget(w1);

        QSignalSpy spy(&sw, &QTuiStackedWidget::currentChanged);
        sw.setCurrentIndex(5);
        QCOMPARE(sw.currentIndex(), 0); // unchanged
        QCOMPARE(spy.count(), 0);

        sw.setCurrentIndex(-1);
        QCOMPARE(sw.currentIndex(), 0);
        QCOMPARE(spy.count(), 0);
    }

    void stacked_noSignalWhenSameIndex()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        sw.addWidget(w1);

        QSignalSpy spy(&sw, &QTuiStackedWidget::currentChanged);
        sw.setCurrentIndex(0);
        QCOMPARE(spy.count(), 0);
    }

    void stacked_removeWidget()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        auto *w3 = new SplitterTestWidget;
        sw.addWidget(w1);
        sw.addWidget(w2);
        sw.addWidget(w3);
        sw.setCurrentIndex(1);

        // Remove non-current widget before current
        sw.removeWidget(w1);
        QCOMPARE(sw.count(), 2);
        QCOMPARE(sw.currentIndex(), 0); // adjusted
        QCOMPARE(sw.currentWidget(), w2);
    }

    void stacked_removeCurrentWidget()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sw.addWidget(w1);
        sw.addWidget(w2);
        sw.setCurrentIndex(0);

        QSignalSpy spy(&sw, &QTuiStackedWidget::currentChanged);
        sw.removeWidget(w1);
        QCOMPARE(sw.count(), 1);
        QCOMPARE(sw.currentWidget(), w2);
        QCOMPARE(spy.count(), 1);
    }

    void stacked_indexOf()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sw.addWidget(w1);
        sw.addWidget(w2);

        QCOMPARE(sw.indexOf(w1), 0);
        QCOMPARE(sw.indexOf(w2), 1);
        QCOMPARE(sw.indexOf(nullptr), -1);
    }

    void stacked_widgetAt()
    {
        QTuiStackedWidget sw;
        auto *w1 = new SplitterTestWidget;
        sw.addWidget(w1);

        QCOMPARE(sw.widget(0), w1);
        QVERIFY(sw.widget(1) == nullptr);
        QVERIFY(sw.widget(-1) == nullptr);
    }

    void stacked_onlyCurrentWidgetPainted()
    {
        QTuiStackedWidget sw;
        sw.setGeometry(QRect(0, 0, 10, 5));

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sw.addWidget(w1);
        sw.addWidget(w2);
        sw.setCurrentIndex(1);

        ScreenBuffer buffer(10, 5);
        sw.paintEvent(buffer);

        // w2 is current and should be painted (its geometry set to sw's geometry)
        QCOMPARE(w2->isVisible(), true);
        QCOMPARE(w1->isVisible(), false);
    }

    // ---------------------------------------------------------------
    // QTuiSplitter
    // ---------------------------------------------------------------

    void splitter_defaultConstruction()
    {
        QTuiSplitter sp;
        QCOMPARE(sp.count(), 0);
        QCOMPARE(sp.orientation(), Qt::Horizontal);
        QVERIFY(sp.sizes().isEmpty());
    }

    void splitter_constructionWithOrientation()
    {
        QTuiSplitter sp(Qt::Vertical);
        QCOMPARE(sp.orientation(), Qt::Vertical);
    }

    void splitter_addWidget()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 21, 5));

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;

        sp.addWidget(w1);
        QCOMPARE(sp.count(), 1);
        QCOMPARE(sp.widget(0), w1);

        sp.addWidget(w2);
        QCOMPARE(sp.count(), 2);
        QCOMPARE(sp.widget(1), w2);

        // Should have 1 handle between 2 widgets
        QVERIFY(sp.handle(0) != nullptr);
    }

    void splitter_sizesDistributedEvenly()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 21, 5)); // 21 cols, 1 handle = 20 for widgets

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        QList<int> sizes = sp.sizes();
        QCOMPARE(sizes.size(), 2);
        QCOMPARE(sizes[0] + sizes[1], 20); // 21 - 1 handle
    }

    void splitter_setSizes()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 21, 5));

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        sp.setSizes({15, 5});
        QList<int> sizes = sp.sizes();
        QCOMPARE(sizes[0], 15);
        QCOMPARE(sizes[1], 5);
    }

    void splitter_respectsMinimumSizes()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 11, 5)); // 11 cols, 1 handle = 10 for widgets

        // w1 needs at least 5 wide, w2 needs at least 3 wide
        auto *w1 = new SplitterTestWidget(QSize(5, 1));
        auto *w2 = new SplitterTestWidget(QSize(3, 1));
        sp.addWidget(w1);
        sp.addWidget(w2);

        // Try to set sizes that violate w1's minimum
        sp.setSizes({2, 8});
        QList<int> sizes = sp.sizes();
        QVERIFY(sizes[0] >= 5); // w1 minimum respected
        QVERIFY(sizes[1] >= 3); // w2 minimum respected
    }

    void splitter_handleMovement()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 21, 5));

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        sp.setSizes({10, 10});

        QSignalSpy spy(&sp, &QTuiSplitter::splitterMoved);

        // Send right arrow to handle to move it
        QTuiSplitterHandle *h = sp.handle(0);
        QVERIFY(h != nullptr);
        sendKey(*h, Qt::Key_Right);

        QList<int> sizes = sp.sizes();
        QCOMPARE(sizes[0], 11);
        QCOMPARE(sizes[1], 9);
        QCOMPARE(spy.count(), 1);
    }

    void splitter_handleMovementRespectsMinimum()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 11, 5));

        auto *w1 = new SplitterTestWidget(QSize(3, 1));
        auto *w2 = new SplitterTestWidget(QSize(3, 1));
        sp.addWidget(w1);
        sp.addWidget(w2);

        sp.setSizes({3, 7});

        // Try to move handle left, w1 is already at minimum
        QTuiSplitterHandle *h = sp.handle(0);
        sendKey(*h, Qt::Key_Left);

        QList<int> sizes = sp.sizes();
        QVERIFY(sizes[0] >= 3);
        QVERIFY(sizes[1] >= 3);
    }

    void splitter_verticalOrientation()
    {
        QTuiSplitter sp(Qt::Vertical);
        sp.setGeometry(QRect(0, 0, 10, 11)); // 11 rows, 1 handle = 10 for widgets

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        QList<int> sizes = sp.sizes();
        QCOMPARE(sizes.size(), 2);
        QCOMPARE(sizes[0] + sizes[1], 10);

        // Handle responds to up/down for vertical
        sp.setSizes({5, 5});
        QTuiSplitterHandle *h = sp.handle(0);
        sendKey(*h, Qt::Key_Down);

        sizes = sp.sizes();
        QCOMPARE(sizes[0], 6);
        QCOMPARE(sizes[1], 4);
    }

    void splitter_setOrientation()
    {
        QTuiSplitter sp(Qt::Horizontal);
        sp.setGeometry(QRect(0, 0, 21, 5));

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        sp.setOrientation(Qt::Vertical);
        QCOMPARE(sp.orientation(), Qt::Vertical);

        QTuiSplitterHandle *h = sp.handle(0);
        QCOMPARE(h->orientation(), Qt::Vertical);
    }

    void splitter_handlePaintsVisibleDivider()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 11, 3));

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        ScreenBuffer buffer(11, 3);
        sp.paintEvent(buffer);

        // The handle should be at the divider position
        QTuiSplitterHandle *h = sp.handle(0);
        QRect hGeo = h->geometry();
        QCOMPARE(hGeo.width(), 1);

        // The handle cell should contain the │ character
        const Cell &cell = buffer.backCell(hGeo.x(), hGeo.y());
        QCOMPARE(cell.ch, char32_t(0x2502));
    }

    void splitter_threeWidgets()
    {
        QTuiSplitter sp;
        sp.setGeometry(QRect(0, 0, 32, 5)); // 32 cols, 2 handles = 30 for widgets

        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        auto *w3 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);
        sp.addWidget(w3);

        QCOMPARE(sp.count(), 3);
        QVERIFY(sp.handle(0) != nullptr);
        QVERIFY(sp.handle(1) != nullptr);

        QList<int> sizes = sp.sizes();
        QCOMPARE(sizes.size(), 3);
        QCOMPARE(sizes[0] + sizes[1] + sizes[2], 30);
    }

    void splitter_minimumSize()
    {
        QTuiSplitter sp;
        auto *w1 = new SplitterTestWidget(QSize(5, 2));
        auto *w2 = new SplitterTestWidget(QSize(3, 4));
        sp.addWidget(w1);
        sp.addWidget(w2);

        QSize ms = sp.minimumSize();
        // Horizontal: width = 5 + 1 (handle) + 3 = 9, height = max(2, 4) = 4
        QCOMPARE(ms.width(), 9);
        QCOMPARE(ms.height(), 4);
    }

    void splitter_widgetOutOfRange()
    {
        QTuiSplitter sp;
        QVERIFY(sp.widget(-1) == nullptr);
        QVERIFY(sp.widget(0) == nullptr);
        QVERIFY(sp.handle(-1) == nullptr);
        QVERIFY(sp.handle(0) == nullptr);
    }

    // ---------------------------------------------------------------
    // QTuiSplitterHandle
    // ---------------------------------------------------------------

    void handle_orientation()
    {
        QTuiSplitter sp(Qt::Horizontal);
        sp.setGeometry(QRect(0, 0, 11, 3));
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        QTuiSplitterHandle *h = sp.handle(0);
        QCOMPARE(h->orientation(), Qt::Horizontal);
        QCOMPARE(h->splitter(), &sp);
    }

    void handle_minimumSize()
    {
        QTuiSplitter sp(Qt::Horizontal);
        sp.setGeometry(QRect(0, 0, 11, 3));
        auto *w1 = new SplitterTestWidget;
        auto *w2 = new SplitterTestWidget;
        sp.addWidget(w1);
        sp.addWidget(w2);

        QTuiSplitterHandle *h = sp.handle(0);
        QSize ms = h->minimumSize();
        QCOMPARE(ms.width(), 1);
        QCOMPARE(ms.height(), 1);
    }
};

QTEST_MAIN(TestStackedSplitter)
#include "test_qtui_stacked_splitter.moc"
