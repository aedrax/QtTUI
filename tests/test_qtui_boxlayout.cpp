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
};

QTEST_MAIN(TestQTuiBoxLayout)
#include "test_qtui_boxlayout.moc"
