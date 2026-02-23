#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// Concrete test widget since QTuiWidget is abstract
class TestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit TestWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer & /*buffer*/) override {}
};

class TestQTuiWidget : public QObject {
    Q_OBJECT
private slots:
    // Feature: qttui-framework, Property 10: Widget geometry round-trip
    void geometryRoundTrip()
    {
        rc::check("setGeometry(rect) then geometry() returns same QRect for non-negative values",
            []() {
                TestWidget w;
                const auto x = *rc::gen::inRange(0, 500);
                const auto y = *rc::gen::inRange(0, 500);
                const auto width = *rc::gen::inRange(0, 500);
                const auto height = *rc::gen::inRange(0, 500);
                QRect rect(x, y, width, height);

                w.setGeometry(rect);
                RC_ASSERT(w.geometry() == rect);
            });
    }

    // Feature: qttui-framework, Property 11: Widget visibility state
    void visibilityState()
    {
        rc::check("setVisible(v) then isVisible() returns v",
            []() {
                TestWidget w;
                const auto v = *rc::gen::arbitrary<bool>();

                w.setVisible(v);
                RC_ASSERT(w.isVisible() == v);
            });
    }

    // Feature: qttui-framework, Property 12: Focus chain invariant with signal emission
    void focusChainInvariant()
    {
        rc::check("After setFocus() on W: exactly one widget has focus, signals emitted correctly",
            []() {
                const auto count = *rc::gen::inRange(2, 10);
                QList<TestWidget *> widgets;
                for (int i = 0; i < count; ++i)
                    widgets.append(new TestWidget());

                // Pick a random widget to focus
                const auto idx = *rc::gen::inRange(0, count);
                TestWidget *target = widgets[idx];

                // Track signals
                QSignalSpy gainedSpy(target, &QTuiWidget::focusGained);

                // Optionally give focus to another widget first
                const auto prevIdx = *rc::gen::inRange(0, count);
                if (prevIdx != idx) {
                    widgets[prevIdx]->setFocus();
                }

                QTuiWidget *prevFocused = nullptr;
                for (auto *w : widgets) {
                    if (w->hasFocus())
                        prevFocused = w;
                }

                QSignalSpy lostSpy(prevFocused ? prevFocused : target, &QTuiWidget::focusLost);

                // Now set focus on target
                target->setFocus();

                // Exactly one widget has focus
                int focusCount = 0;
                for (auto *w : widgets) {
                    if (w->hasFocus())
                        ++focusCount;
                }
                RC_ASSERT(focusCount == 1);
                RC_ASSERT(target->hasFocus());

                // focusGained emitted on target
                RC_ASSERT(gainedSpy.count() >= 1);

                // focusLost emitted on previous (if different)
                if (prevFocused && prevFocused != target) {
                    RC_ASSERT(lostSpy.count() >= 1);
                    RC_ASSERT(!prevFocused->hasFocus());
                }

                qDeleteAll(widgets);
            });
    }

    // Feature: qttui-framework, Property 13: Geometry change dirties screen region
    void geometryChangeDirtiesRegion()
    {
        rc::check("Changing geometry marks both old and new regions as dirty",
            []() {
                TestWidget w;

                // Set initial geometry
                const auto x1 = *rc::gen::inRange(0, 100);
                const auto y1 = *rc::gen::inRange(0, 100);
                const auto w1 = *rc::gen::inRange(1, 100);
                const auto h1 = *rc::gen::inRange(1, 100);
                QRect oldRect(x1, y1, w1, h1);
                w.setGeometry(oldRect);
                w.clearDirtyRegions();

                // Set new geometry (ensure it's different)
                const auto x2 = *rc::gen::inRange(0, 100);
                const auto y2 = *rc::gen::inRange(0, 100);
                const auto w2 = *rc::gen::inRange(1, 100);
                const auto h2 = *rc::gen::inRange(1, 100);
                QRect newRect(x2, y2, w2, h2);
                RC_PRE(oldRect != newRect);

                w.setGeometry(newRect);

                RC_ASSERT(w.hasDirtyRegions());
                RC_ASSERT(w.dirtyOldRegion() == oldRect);
                RC_ASSERT(w.dirtyNewRegion() == newRect);
            });
    }
};

QTEST_MAIN(TestQTuiWidget)
#include "test_qtui_widget.moc"
