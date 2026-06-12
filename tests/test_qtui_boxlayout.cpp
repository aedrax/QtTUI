#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// Concrete widget with configurable minimum size
class LayoutTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit LayoutTestWidget(QSize minSize = QSize(0, 0), QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return m_minSize; }
    void setMinSize(QSize s) { m_minSize = s; }
private:
    QSize m_minSize;
};

// Widget with a size hint larger than its minimum size
class HintWidget : public QTuiWidget {
    Q_OBJECT
public:
    HintWidget(QSize minSize, QSize hint) : m_min(minSize), m_hint(hint) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return m_min; }
    QSize sizeHint() const override { return m_hint; }
private:
    QSize m_min, m_hint;
};

class TestQTuiBoxLayout : public QObject {
    Q_OBJECT
private slots:

    // Feature: qttui-framework, Property 31: Box layout arrangement and proportional distribution
    void boxLayoutArrangementVertical()
    {
        rc::check("Vertical: widgets arranged sequentially, no overlap, total = available - margins, stretch proportional ±1",
            []() {
                const int n = *rc::gen::inRange(1, 6);
                const int areaW = *rc::gen::inRange(20, 200);
                const int areaH = *rc::gen::inRange(n * 2, 300);
                const int ml = *rc::gen::inRange(0, 5);
                const int mt = *rc::gen::inRange(0, 5);
                const int mr = *rc::gen::inRange(0, 5);
                const int mb = *rc::gen::inRange(0, 5);

                RC_PRE(areaH > mt + mb + n);
                RC_PRE(areaW > ml + mr);

                QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
                layout.setContentsMargins(ml, mt, mr, mb);

                QList<LayoutTestWidget *> widgets;
                QList<int> stretches;
                for (int i = 0; i < n; ++i) {
                    auto *w = new LayoutTestWidget();
                    int s = *rc::gen::inRange(1, 10);
                    stretches.append(s);
                    layout.addWidget(w, s);
                    widgets.append(w);
                }

                layout.apply(QRect(0, 0, areaW, areaH));

                const int contentH = areaH - mt - mb;
                const int contentW = areaW - ml - mr;
                const int contentX = ml;

                // Check sequential arrangement, no overlap
                int totalH = 0;
                for (int i = 0; i < n; ++i) {
                    QRect g = widgets[i]->geometry();
                    RC_ASSERT(g.x() == contentX);
                    RC_ASSERT(g.width() == contentW);
                    RC_ASSERT(g.height() > 0);
                    if (i > 0) {
                        QRect prev = widgets[i - 1]->geometry();
                        // No overlap: current top >= previous bottom
                        RC_ASSERT(g.y() >= prev.y() + prev.height());
                        // Sequential: no gap
                        RC_ASSERT(g.y() == prev.y() + prev.height());
                    }
                    totalH += g.height();
                }

                // Total space equals content area
                RC_ASSERT(totalH == contentH);

                // Stretch proportionality within ±1 cell
                int totalStretch = 0;
                for (int s : stretches) totalStretch += s;

                for (int i = 0; i < n; ++i) {
                    int expected = (contentH * stretches[i]) / totalStretch;
                    int actual = widgets[i]->geometry().height();
                    RC_ASSERT(std::abs(actual - expected) <= 1);
                }

                qDeleteAll(widgets);
            });
    }

    void boxLayoutArrangementHorizontal()
    {
        rc::check("Horizontal: widgets arranged sequentially, no overlap, total = available - margins, stretch proportional ±1",
            []() {
                const int n = *rc::gen::inRange(1, 6);
                const int areaW = *rc::gen::inRange(n * 2, 300);
                const int areaH = *rc::gen::inRange(20, 200);
                const int ml = *rc::gen::inRange(0, 5);
                const int mt = *rc::gen::inRange(0, 5);
                const int mr = *rc::gen::inRange(0, 5);
                const int mb = *rc::gen::inRange(0, 5);

                RC_PRE(areaW > ml + mr + n);
                RC_PRE(areaH > mt + mb);

                QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);
                layout.setContentsMargins(ml, mt, mr, mb);

                QList<LayoutTestWidget *> widgets;
                QList<int> stretches;
                for (int i = 0; i < n; ++i) {
                    auto *w = new LayoutTestWidget();
                    int s = *rc::gen::inRange(1, 10);
                    stretches.append(s);
                    layout.addWidget(w, s);
                    widgets.append(w);
                }

                layout.apply(QRect(0, 0, areaW, areaH));

                const int contentW = areaW - ml - mr;
                const int contentH = areaH - mt - mb;
                const int contentY = mt;

                int totalW = 0;
                for (int i = 0; i < n; ++i) {
                    QRect g = widgets[i]->geometry();
                    RC_ASSERT(g.y() == contentY);
                    RC_ASSERT(g.height() == contentH);
                    RC_ASSERT(g.width() > 0);
                    if (i > 0) {
                        QRect prev = widgets[i - 1]->geometry();
                        RC_ASSERT(g.x() >= prev.x() + prev.width());
                        RC_ASSERT(g.x() == prev.x() + prev.width());
                    }
                    totalW += g.width();
                }

                RC_ASSERT(totalW == contentW);

                int totalStretch = 0;
                for (int s : stretches) totalStretch += s;

                for (int i = 0; i < n; ++i) {
                    int expected = (contentW * stretches[i]) / totalStretch;
                    int actual = widgets[i]->geometry().width();
                    RC_ASSERT(std::abs(actual - expected) <= 1);
                }

                qDeleteAll(widgets);
            });
    }

    void boxLayoutStretchZeroWidgets()
    {
        rc::check("Stretch=0 widgets get minimumSize, stretch>0 widgets share remainder",
            []() {
                const int areaH = *rc::gen::inRange(50, 300);
                const int fixedH = *rc::gen::inRange(1, 15);
                const int stretchVal = *rc::gen::inRange(1, 10);

                RC_PRE(areaH > fixedH + 2);

                QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);

                auto *fixed = new LayoutTestWidget(QSize(0, fixedH));
                auto *stretchy = new LayoutTestWidget();

                layout.addWidget(fixed, 0);
                layout.addWidget(stretchy, stretchVal);

                layout.apply(QRect(0, 0, 80, areaH));

                RC_ASSERT(fixed->geometry().height() == fixedH);
                RC_ASSERT(stretchy->geometry().height() == areaH - fixedH);

                delete fixed;
                delete stretchy;
            });
    }

    // Feature: qttui-framework, Property 32: Layout minimum size respect
    void layoutMinimumSizeRespect()
    {
        rc::check("Widget with minimumSize > proportional allocation gets at least minimumSize",
            []() {
                // Two stretch widgets, one with a large minimum size
                const int areaH = *rc::gen::inRange(40, 200);
                const int minH = *rc::gen::inRange(areaH / 2 + 1, areaH - 1);

                RC_PRE(minH > areaH / 2);  // Ensure min exceeds proportional (50%)
                RC_PRE(areaH - minH >= 1); // Other widget gets at least 1

                QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);

                auto *bigMin = new LayoutTestWidget(QSize(0, minH));
                auto *other = new LayoutTestWidget();

                layout.addWidget(bigMin, 1);
                layout.addWidget(other, 1);

                layout.apply(QRect(0, 0, 80, areaH));

                // bigMin should get at least its minimum
                RC_ASSERT(bigMin->geometry().height() >= minH);
                // other gets the remainder
                RC_ASSERT(other->geometry().height() == areaH - bigMin->geometry().height());
                // Total fills available
                RC_ASSERT(bigMin->geometry().height() + other->geometry().height() == areaH);

                delete bigMin;
                delete other;
            });
    }

    void layoutMinimumSizeRespectHorizontal()
    {
        rc::check("Horizontal: widget with minimumSize > proportional gets at least minimumSize",
            []() {
                const int areaW = *rc::gen::inRange(40, 200);
                const int minW = *rc::gen::inRange(areaW / 2 + 1, areaW - 1);

                RC_PRE(minW > areaW / 2);
                RC_PRE(areaW - minW >= 1);

                QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);

                auto *bigMin = new LayoutTestWidget(QSize(minW, 0));
                auto *other = new LayoutTestWidget();

                layout.addWidget(bigMin, 1);
                layout.addWidget(other, 1);

                layout.apply(QRect(0, 0, areaW, 40));

                RC_ASSERT(bigMin->geometry().width() >= minW);
                RC_ASSERT(other->geometry().width() == areaW - bigMin->geometry().width());
                RC_ASSERT(bigMin->geometry().width() + other->geometry().width() == areaW);

                delete bigMin;
                delete other;
            });
    }

    // Edge case: duplicate addWidget is no-op
    void duplicateAddWidget()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        auto *w = new LayoutTestWidget();
        layout.addWidget(w, 1);
        layout.addWidget(w, 2); // should be no-op
        QCOMPARE(layout.count(), 1);
        QCOMPARE(layout.items()[0].stretch, 1); // original stretch preserved
        delete w;
    }

    // Edge case: negative stretch clamped to 0
    void negativeStretchClamped()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        auto *w = new LayoutTestWidget();
        layout.addWidget(w, -5);
        QCOMPARE(layout.items()[0].stretch, 0);
        delete w;
    }

    // A widget hidden by the USER must be skipped during measurement and
    // distribution (its space goes to the others) and must never be
    // force-shown by apply().
    void userHiddenWidgetSkippedAndNotForceShown()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        auto *a = new LayoutTestWidget(QSize(0, 4));
        auto *b = new LayoutTestWidget(QSize(0, 4));
        auto *c = new LayoutTestWidget(QSize(0, 4));
        layout.addWidget(a, 1);
        layout.addWidget(b, 1);
        layout.addWidget(c, 1);

        b->setVisible(false); // user hides b

        layout.apply(QRect(0, 0, 20, 12));

        // b stays hidden even though there is plenty of space
        QVERIFY(!b->isVisible());
        // its space is redistributed to a and c, contiguously
        QCOMPARE(a->geometry().y(), 0);
        QCOMPARE(c->geometry().y(), a->geometry().height());
        QCOMPARE(a->geometry().height() + c->geometry().height(), 12);

        // Re-applying must not force-show the user-hidden widget either
        layout.apply(QRect(0, 0, 20, 12));
        QVERIFY(!b->isVisible());

        delete a;
        delete b;
        delete c;
    }

    // A widget the LAYOUT hid because it did not fit must come back
    // automatically when space returns.
    void layoutHiddenWidgetReappearsWhenSpaceReturns()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        auto *a = new LayoutTestWidget(QSize(0, 6));
        auto *b = new LayoutTestWidget(QSize(0, 6));
        layout.addWidget(a, 0);
        layout.addWidget(b, 0);

        // Only a fits: the layout hides b
        layout.apply(QRect(0, 0, 20, 6));
        QVERIFY(a->isVisible());
        QVERIFY(!b->isVisible());

        // Space returns: b (hidden by the layout, not the user) reappears
        layout.apply(QRect(0, 0, 20, 12));
        QVERIFY(b->isVisible());
        QCOMPARE(b->geometry(), QRect(0, 6, 20, 6));

        delete a;
        delete b;
    }

    // User-hidden and layout-hidden widgets must be distinguished: after both
    // kinds of hide, only the layout-hidden one is restored by apply().
    void userHiddenNotRestoredWhileLayoutHiddenIs()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        auto *a = new LayoutTestWidget(QSize(0, 6));
        auto *b = new LayoutTestWidget(QSize(0, 6));
        auto *c = new LayoutTestWidget(QSize(0, 6));
        layout.addWidget(a, 0);
        layout.addWidget(b, 0);
        layout.addWidget(c, 0);

        b->setVisible(false);          // user hides b
        layout.apply(QRect(0, 0, 20, 6)); // a fits, layout hides c
        QVERIFY(!b->isVisible());
        QVERIFY(!c->isVisible());

        layout.apply(QRect(0, 0, 20, 18)); // room for everything
        QVERIFY(!b->isVisible()); // user-hidden: stays hidden
        QVERIFY(c->isVisible());  // layout-hidden: restored
        // c packs directly after a — b takes no space
        QCOMPARE(c->geometry().y(), a->geometry().height());

        delete a;
        delete b;
        delete c;
    }

    void spacingGoesBetweenItems()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);
        LayoutTestWidget a(QSize(3, 1)), b(QSize(4, 1));
        layout.addWidget(&a);
        layout.addWidget(&b);
        layout.setSpacing(2);
        QCOMPARE(layout.minimumSize(), QSize(9, 1));

        layout.apply(QRect(0, 0, 20, 1));
        QCOMPARE(a.geometry(), QRect(0, 0, 3, 1));
        QCOMPARE(b.geometry(), QRect(5, 0, 4, 1));
    }

    void addSpacingAddsFixedCells()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        LayoutTestWidget a(QSize(1, 2)), b(QSize(1, 2));
        layout.addWidget(&a);
        layout.addSpacing(3);
        layout.addWidget(&b);
        QCOMPARE(layout.minimumSize().height(), 7);

        layout.apply(QRect(0, 0, 10, 20));
        QCOMPARE(b.geometry().y(), 5);
    }

    void addStretchPushesItemsApart()
    {
        // [a][stretch][b]: b goes to the far end, a keeps its minimum
        QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);
        LayoutTestWidget a(QSize(3, 1)), b(QSize(4, 1));
        layout.addWidget(&a);
        layout.addStretch();
        layout.addWidget(&b);

        layout.apply(QRect(0, 0, 20, 1));
        QCOMPARE(a.geometry(), QRect(0, 0, 3, 1));
        QCOMPARE(b.geometry(), QRect(16, 0, 4, 1));
    }

    void fixedPolicyUsesHintOnCrossAxis()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        HintWidget fixed(QSize(4, 1), QSize(8, 1));
        HintWidget fill(QSize(4, 1), QSize(8, 1));
        fixed.setSizePolicy(QTuiSizePolicy::Fixed, QTuiSizePolicy::Fixed);
        layout.addWidget(&fixed);
        layout.addWidget(&fill);

        layout.apply(QRect(0, 0, 30, 10));
        QCOMPARE(fixed.geometry().width(), 8);  // size hint, not 30
        QCOMPARE(fill.geometry().width(), 30);   // Preferred fills
    }

    void hintUsedWhenItFitsElseMinimum()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);
        HintWidget a(QSize(3, 1), QSize(10, 1));
        HintWidget b(QSize(3, 1), QSize(10, 1));
        layout.addWidget(&a);
        layout.addWidget(&b);

        layout.apply(QRect(0, 0, 25, 1));
        QCOMPARE(a.geometry().width(), 10);
        QCOMPARE(b.geometry().width(), 10);

        layout.apply(QRect(0, 0, 15, 1)); // hints (20) do not fit
        QCOMPARE(a.geometry().width(), 3);
        QCOMPARE(b.geometry().width(), 3);
    }

    void expandingTakesFreeSpaceAndFixedNeverGrows()
    {
        QTuiBoxLayout layout(QTuiBoxLayout::LeftToRight);
        LayoutTestWidget plain(QSize(4, 1));
        LayoutTestWidget expanding(QSize(4, 1));
        LayoutTestWidget fixed(QSize(4, 1));
        expanding.setSizePolicy(QTuiSizePolicy::Expanding, QTuiSizePolicy::Preferred);
        fixed.setSizePolicy(QTuiSizePolicy::Fixed, QTuiSizePolicy::Preferred);
        layout.addWidget(&plain);
        layout.addWidget(&expanding);
        layout.addWidget(&fixed, 3); // stretch is ignored for Fixed

        layout.apply(QRect(0, 0, 30, 1));
        QCOMPARE(plain.geometry().width(), 4);
        QCOMPARE(fixed.geometry().width(), 4);
        QCOMPARE(expanding.geometry().width(), 22);
    }
};

QTEST_MAIN(TestQTuiBoxLayout)
#include "test_qtui_boxlayout.moc"
