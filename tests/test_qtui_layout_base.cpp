#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiSizePolicy.h>
#include <QtTUI/QTuiLayoutItem.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiFrame.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiFormLayout.h>
#include <QSignalSpy>
#include <QStringListModel>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiAbstractItemView.h>
#include <QtTUI/QTuiColumnView.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/QTuiTreeView.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiLCDNumber.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// Concrete widget with configurable minimum size for testing
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

// Concrete layout subclass for testing QTuiLayout base (which is abstract)
class TestLayout : public QTuiLayout {
    Q_OBJECT
public:
    using QTuiLayout::QTuiLayout;
    void apply(const QRect &) override {} // no-op for base class tests
};

class TestQTuiLayoutBase : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiSizePolicy tests
    // ---------------------------------------------------------------

    void sizePolicyDefaultConstruction()
    {
        QTuiSizePolicy sp;
        QCOMPARE(sp.horizontalPolicy(), QTuiSizePolicy::Preferred);
        QCOMPARE(sp.verticalPolicy(), QTuiSizePolicy::Preferred);
        QCOMPARE(sp.horizontalStretch(), 0);
        QCOMPARE(sp.verticalStretch(), 0);
    }

    void sizePolicyParameterizedConstruction()
    {
        QTuiSizePolicy sp(QTuiSizePolicy::Fixed, QTuiSizePolicy::Expanding);
        QCOMPARE(sp.horizontalPolicy(), QTuiSizePolicy::Fixed);
        QCOMPARE(sp.verticalPolicy(), QTuiSizePolicy::Expanding);
        QCOMPARE(sp.horizontalStretch(), 0);
        QCOMPARE(sp.verticalStretch(), 0);
    }

    void sizePolicySetters()
    {
        QTuiSizePolicy sp;
        sp.setHorizontalPolicy(QTuiSizePolicy::Minimum);
        sp.setVerticalPolicy(QTuiSizePolicy::Maximum);
        sp.setHorizontalStretch(50);
        sp.setVerticalStretch(100);
        QCOMPARE(sp.horizontalPolicy(), QTuiSizePolicy::Minimum);
        QCOMPARE(sp.verticalPolicy(), QTuiSizePolicy::Maximum);
        QCOMPARE(sp.horizontalStretch(), 50);
        QCOMPARE(sp.verticalStretch(), 100);
    }

    void sizePolicyStretchClampingProperty()
    {
        /**
         * Stretch values are clamped to [0, 255] for any input.
         */
        rc::check("Stretch is clamped to [0, 255]", []() {
            const int input = *rc::gen::inRange(-1000, 1000);
            QTuiSizePolicy sp;

            sp.setHorizontalStretch(input);
            RC_ASSERT(sp.horizontalStretch() >= 0);
            RC_ASSERT(sp.horizontalStretch() <= 255);

            sp.setVerticalStretch(input);
            RC_ASSERT(sp.verticalStretch() >= 0);
            RC_ASSERT(sp.verticalStretch() <= 255);

            if (input >= 0 && input <= 255) {
                RC_ASSERT(sp.horizontalStretch() == input);
                RC_ASSERT(sp.verticalStretch() == input);
            }
        });
    }

    void sizePolicyAllPolicyValues()
    {
        // Verify all 7 policy enum values can be set and retrieved
        const QList<QTuiSizePolicy::Policy> policies = {
            QTuiSizePolicy::Fixed,
            QTuiSizePolicy::Minimum,
            QTuiSizePolicy::Maximum,
            QTuiSizePolicy::Preferred,
            QTuiSizePolicy::Expanding,
            QTuiSizePolicy::MinimumExpanding,
            QTuiSizePolicy::Ignored
        };
        for (auto p : policies) {
            QTuiSizePolicy sp(p, p);
            QCOMPARE(sp.horizontalPolicy(), p);
            QCOMPARE(sp.verticalPolicy(), p);
        }
    }

    // ---------------------------------------------------------------
    // QTuiWidgetItem tests
    // ---------------------------------------------------------------

    void widgetItemDelegatesToWidget()
    {
        LayoutTestWidget w(QSize(10, 20));
        QTuiWidgetItem item(&w);

        QCOMPARE(item.sizeHint(), QSize(10, 20));
        QCOMPARE(item.minimumSize(), QSize(10, 20));
        QCOMPARE(item.maximumSize(), QSize(16777215, 16777215));
        QCOMPARE(item.widget(), &w);
    }

    void widgetItemDelegatesGeometry()
    {
        LayoutTestWidget w;
        QTuiWidgetItem item(&w);

        item.setGeometry(QRect(5, 10, 30, 40));
        QCOMPARE(w.geometry(), QRect(5, 10, 30, 40));
        QCOMPARE(item.geometry(), QRect(5, 10, 30, 40));
    }

    void widgetItemIsEmptyWhenNullOrHidden()
    {
        // Null widget
        QTuiWidgetItem nullItem(nullptr);
        QVERIFY(nullItem.isEmpty());
        QCOMPARE(nullItem.sizeHint(), QSize(0, 0));
        QCOMPARE(nullItem.minimumSize(), QSize(0, 0));

        // Hidden widget
        LayoutTestWidget w;
        w.setVisible(false);
        QTuiWidgetItem hiddenItem(&w);
        QVERIFY(hiddenItem.isEmpty());

        // Visible widget
        w.setVisible(true);
        QVERIFY(!hiddenItem.isEmpty());
    }

    void widgetItemSizeDelegationProperty()
    {
        /**
         * For any widget minimum size, QTuiWidgetItem delegates correctly.
         */
        rc::check("WidgetItem sizeHint and minimumSize match widget minimumSize", []() {
            const int w = *rc::gen::inRange(0, 500);
            const int h = *rc::gen::inRange(0, 500);

            LayoutTestWidget widget(QSize(w, h));
            QTuiWidgetItem item(&widget);

            RC_ASSERT(item.sizeHint() == QSize(w, h));
            RC_ASSERT(item.minimumSize() == QSize(w, h));
            RC_ASSERT(item.maximumSize() == QSize(16777215, 16777215));
        });
    }

    // ---------------------------------------------------------------
    // QTuiSpacerItem tests
    // ---------------------------------------------------------------

    void spacerItemSizeHint()
    {
        QTuiSpacerItem spacer(15, 25, QTuiSizePolicy::Preferred, QTuiSizePolicy::Preferred);
        QCOMPARE(spacer.sizeHint(), QSize(15, 25));
    }

    void spacerItemMinimumSizeFixedPolicy()
    {
        // Fixed policy: minimumSize returns configured size
        QTuiSpacerItem spacer(10, 20, QTuiSizePolicy::Fixed, QTuiSizePolicy::Fixed);
        QCOMPARE(spacer.minimumSize(), QSize(10, 20));
    }

    void spacerItemMinimumSizeMinimumPolicy()
    {
        QTuiSpacerItem spacer(10, 20, QTuiSizePolicy::Minimum, QTuiSizePolicy::Minimum);
        QCOMPARE(spacer.minimumSize(), QSize(10, 20));
    }

    void spacerItemMinimumSizeExpandingPolicy()
    {
        QTuiSpacerItem spacer(10, 20, QTuiSizePolicy::Expanding, QTuiSizePolicy::Expanding);
        QCOMPARE(spacer.minimumSize(), QSize(10, 20));
    }

    void spacerItemMinimumSizeMinimumExpandingPolicy()
    {
        QTuiSpacerItem spacer(10, 20, QTuiSizePolicy::MinimumExpanding, QTuiSizePolicy::MinimumExpanding);
        QCOMPARE(spacer.minimumSize(), QSize(10, 20));
    }

    void spacerItemMinimumSizeZeroForOtherPolicies()
    {
        // Maximum, Preferred, Ignored return 0 for minimumSize
        QTuiSpacerItem maxSpacer(10, 20, QTuiSizePolicy::Maximum, QTuiSizePolicy::Maximum);
        QCOMPARE(maxSpacer.minimumSize(), QSize(0, 0));

        QTuiSpacerItem prefSpacer(10, 20, QTuiSizePolicy::Preferred, QTuiSizePolicy::Preferred);
        QCOMPARE(prefSpacer.minimumSize(), QSize(0, 0));

        QTuiSpacerItem ignSpacer(10, 20, QTuiSizePolicy::Ignored, QTuiSizePolicy::Ignored);
        QCOMPARE(ignSpacer.minimumSize(), QSize(0, 0));
    }

    void spacerItemMaximumSizeFixedPolicy()
    {
        QTuiSpacerItem spacer(10, 20, QTuiSizePolicy::Fixed, QTuiSizePolicy::Fixed);
        QCOMPARE(spacer.maximumSize(), QSize(10, 20));
    }

    void spacerItemMaximumSizeNonFixedPolicy()
    {
        QTuiSpacerItem spacer(10, 20, QTuiSizePolicy::Expanding, QTuiSizePolicy::Minimum);
        QCOMPARE(spacer.maximumSize(), QSize(16777215, 16777215));
    }

    void spacerItemGeometry()
    {
        QTuiSpacerItem spacer(10, 20);
        spacer.setGeometry(QRect(5, 5, 10, 20));
        QCOMPARE(spacer.geometry(), QRect(5, 5, 10, 20));
    }

    void spacerItemNeverEmpty()
    {
        QTuiSpacerItem spacer(0, 0);
        QVERIFY(!spacer.isEmpty());
    }

    void spacerItemMinimumSizeProperty()
    {
        /**
         * SpacerItem minimumSize depends on policy: Fixed/Minimum/Expanding/MinimumExpanding
         * return configured size, others return 0.
         */
        rc::check("SpacerItem minimumSize follows policy rules", []() {
            const int w = *rc::gen::inRange(1, 500);
            const int h = *rc::gen::inRange(1, 500);
            const int policyIdx = *rc::gen::inRange(0, 7);

            const QTuiSizePolicy::Policy policies[] = {
                QTuiSizePolicy::Fixed,
                QTuiSizePolicy::Minimum,
                QTuiSizePolicy::Maximum,
                QTuiSizePolicy::Preferred,
                QTuiSizePolicy::Expanding,
                QTuiSizePolicy::MinimumExpanding,
                QTuiSizePolicy::Ignored
            };
            auto policy = policies[policyIdx];

            QTuiSpacerItem spacer(w, h, policy, policy);
            QSize minSize = spacer.minimumSize();

            bool hasMinimum = (policy == QTuiSizePolicy::Fixed ||
                               policy == QTuiSizePolicy::Minimum ||
                               policy == QTuiSizePolicy::Expanding ||
                               policy == QTuiSizePolicy::MinimumExpanding);

            if (hasMinimum) {
                RC_ASSERT(minSize.width() == w);
                RC_ASSERT(minSize.height() == h);
            } else {
                RC_ASSERT(minSize.width() == 0);
                RC_ASSERT(minSize.height() == 0);
            }
        });
    }

    // ---------------------------------------------------------------
    // QTuiLayoutItemWrapper (nested layout) tests
    // ---------------------------------------------------------------

    // count() is virtual: a wrapper around a concrete layout that keeps its
    // own item list (e.g. box) must see the real item count, so nested
    // layouts are no longer reported as empty.
    void wrapperSeesConcreteLayoutCount()
    {
        QTuiBoxLayout box(QTuiBoxLayout::TopToBottom);
        QTuiLayoutItemWrapper emptyWrapper(&box);
        QVERIFY(emptyWrapper.isEmpty());

        auto *w = new LayoutTestWidget(QSize(4, 2));
        box.addWidget(w, 1);
        QCOMPARE(static_cast<QTuiLayout &>(box).count(), 1); // virtual dispatch
        QVERIFY(!emptyWrapper.isEmpty());

        delete w;
        QVERIFY(emptyWrapper.isEmpty()); // destroyed widget drops the entry
    }

    // The wrapper reports the wrapped layout's aggregated minimum size.
    void wrapperAggregatesMinimumSize()
    {
        QTuiBoxLayout box(QTuiBoxLayout::TopToBottom);
        auto *a = new LayoutTestWidget(QSize(12, 3));
        auto *b = new LayoutTestWidget(QSize(8, 2));
        box.addWidget(a, 0);
        box.addWidget(b, 0);

        // Vertical box: sum heights, max widths
        QCOMPARE(box.minimumSize(), QSize(12, 5));

        QTuiLayoutItemWrapper wrapper(&box);
        QCOMPARE(wrapper.minimumSize(), QSize(12, 5));
        QCOMPARE(wrapper.sizeHint(), QSize(12, 5));

        // Margins are part of the aggregate
        box.setContentsMargins(1, 2, 3, 4);
        QCOMPARE(wrapper.minimumSize(), QSize(12 + 1 + 3, 5 + 2 + 4));

        delete a;
        delete b;
    }

    // Horizontal box aggregates the other way around: sum widths, max heights
    void horizontalBoxMinimumSize()
    {
        QTuiBoxLayout box(QTuiBoxLayout::LeftToRight);
        auto *a = new LayoutTestWidget(QSize(12, 3));
        auto *b = new LayoutTestWidget(QSize(8, 2));
        box.addWidget(a, 0);
        box.addWidget(b, 0);
        QCOMPARE(box.minimumSize(), QSize(20, 3));
        delete a;
        delete b;
    }

    // setGeometry on the wrapper applies the wrapped layout in that rect
    void wrapperSetGeometryAppliesNestedLayout()
    {
        QTuiBoxLayout box(QTuiBoxLayout::TopToBottom);
        auto *a = new LayoutTestWidget();
        auto *b = new LayoutTestWidget();
        box.addWidget(a, 1);
        box.addWidget(b, 1);

        QTuiLayoutItemWrapper wrapper(&box);
        wrapper.setGeometry(QRect(3, 4, 10, 8));

        QCOMPARE(wrapper.geometry(), QRect(3, 4, 10, 8));
        QCOMPARE(a->geometry(), QRect(3, 4, 10, 4));
        QCOMPARE(b->geometry(), QRect(3, 8, 10, 4));

        delete a;
        delete b;
    }

    // ---------------------------------------------------------------
    // QTuiLayout base class tests
    // ---------------------------------------------------------------

    void layoutAddItemIgnoresNull()
    {
        TestLayout layout;
        layout.addItem(nullptr);
        QCOMPARE(layout.count(), 0);
    }

    void layoutAddWidgetIgnoresNull()
    {
        TestLayout layout;
        layout.addWidget(nullptr);
        QCOMPARE(layout.count(), 0);
    }

    void layoutAddItemAndCount()
    {
        TestLayout layout;
        auto *spacer1 = new QTuiSpacerItem(10, 10);
        auto *spacer2 = new QTuiSpacerItem(20, 20);

        layout.addItem(spacer1);
        QCOMPARE(layout.count(), 1);

        layout.addItem(spacer2);
        QCOMPARE(layout.count(), 2);

        QCOMPARE(layout.itemAt(0), spacer1);
        QCOMPARE(layout.itemAt(1), spacer2);
    }

    void layoutAddWidget()
    {
        TestLayout layout;
        auto *w = new LayoutTestWidget();

        layout.addWidget(w);
        QCOMPARE(layout.count(), 1);
        QVERIFY(layout.itemAt(0) != nullptr);

        // The item wraps the widget
        auto *widgetItem = dynamic_cast<QTuiWidgetItem *>(layout.itemAt(0));
        QVERIFY(widgetItem != nullptr);
        QCOMPARE(widgetItem->widget(), w);

        delete w;
    }

    void layoutItemAtOutOfRange()
    {
        TestLayout layout;
        QCOMPARE(layout.itemAt(-1), nullptr);
        QCOMPARE(layout.itemAt(0), nullptr);
        QCOMPARE(layout.itemAt(100), nullptr);
    }

    void layoutContentsMarginsDefault()
    {
        TestLayout layout;
        int l = -1, t = -1, r = -1, b = -1;
        layout.contentsMargins(&l, &t, &r, &b);
        QCOMPARE(l, 0);
        QCOMPARE(t, 0);
        QCOMPARE(r, 0);
        QCOMPARE(b, 0);
    }

    void layoutSetContentsMargins()
    {
        TestLayout layout;
        layout.setContentsMargins(2, 3, 4, 5);
        int l, t, r, b;
        layout.contentsMargins(&l, &t, &r, &b);
        QCOMPARE(l, 2);
        QCOMPARE(t, 3);
        QCOMPARE(r, 4);
        QCOMPARE(b, 5);
    }

    void layoutNegativeMarginsClampedToZero()
    {
        TestLayout layout;
        layout.setContentsMargins(-10, -5, -3, -1);
        int l, t, r, b;
        layout.contentsMargins(&l, &t, &r, &b);
        QCOMPARE(l, 0);
        QCOMPARE(t, 0);
        QCOMPARE(r, 0);
        QCOMPARE(b, 0);
    }

    void layoutMarginsClampingProperty()
    {
        /**
         * Negative margins are clamped to 0, non-negative are preserved.
         */
        rc::check("Margins: negative clamped to 0, non-negative preserved", []() {
            const int l = *rc::gen::inRange(-100, 100);
            const int t = *rc::gen::inRange(-100, 100);
            const int r = *rc::gen::inRange(-100, 100);
            const int b = *rc::gen::inRange(-100, 100);

            TestLayout layout;
            layout.setContentsMargins(l, t, r, b);

            int gl, gt, gr, gb;
            layout.contentsMargins(&gl, &gt, &gr, &gb);

            RC_ASSERT(gl == std::max(0, l));
            RC_ASSERT(gt == std::max(0, t));
            RC_ASSERT(gr == std::max(0, r));
            RC_ASSERT(gb == std::max(0, b));
        });
    }

    // ---------------------------------------------------------------
    // QTuiBoxLayout post-refactor verification
    // ---------------------------------------------------------------

    void boxLayoutStillWorksAfterRefactor()
    {
        // Verify QTuiBoxLayout inherits from QTuiLayout
        QTuiBoxLayout layout(QTuiBoxLayout::TopToBottom);
        QTuiLayout *base = &layout;
        QVERIFY(base != nullptr);

        // Verify basic functionality still works
        auto *w1 = new LayoutTestWidget(QSize(10, 5));
        auto *w2 = new LayoutTestWidget(QSize(10, 5));

        layout.addWidget(w1, 1);
        layout.addWidget(w2, 1);
        layout.apply(QRect(0, 0, 80, 40));

        // Both widgets should have been positioned
        QVERIFY(w1->geometry().height() > 0);
        QVERIFY(w2->geometry().height() > 0);
        QCOMPARE(w1->geometry().height() + w2->geometry().height(), 40);

        delete w1;
        delete w2;
    }

    void plainWidgetAppliesItsLayout()
    {
        // A plain QTuiWidget is a container: it owns the layout, reports
        // the layout's minimum size, and lays out on each geometry change
        QTuiWidget container;
        auto *a = new LayoutTestWidget(QSize(4, 1), &container);
        auto *b = new LayoutTestWidget(QSize(4, 2), &container);
        auto *layout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        layout->addWidget(a);
        layout->addWidget(b);
        container.setLayout(layout);

        QCOMPARE(container.layout(), layout);
        QCOMPARE(layout->parent(), &container);
        QCOMPARE(container.minimumSize(), QSize(4, 3));

        container.setGeometry(QRect(10, 5, 20, 3));
        QCOMPARE(a->geometry(), QRect(10, 5, 20, 1));
        QCOMPARE(b->geometry(), QRect(10, 6, 20, 2));

        container.setGeometry(QRect(0, 0, 8, 3));
        QCOMPARE(b->geometry(), QRect(0, 1, 8, 2));
    }

    void plainWidgetPaintClearsBackground()
    {
        QTuiWidget w;
        w.setGeometry(QRect(1, 1, 3, 2));
        ScreenBuffer buf(5, 4);
        buf.backCell(2, 2).setChar(U'Z');
        w.paintEvent(buf);
        QCOMPARE(buf.backCell(2, 2).ch, U' ');
    }

    void frameShapeAndContentsRect()
    {
        // A label is a QTuiFrame, as in Qt; NoFrame by default
        QTuiLabel label(QStringLiteral("x"));
        QTuiFrame *frame = &label;
        QCOMPARE(frame->frameShape(), QTuiFrame::NoFrame);
        label.setGeometry(QRect(2, 1, 10, 4));
        QCOMPARE(frame->contentsRect(), QRect(2, 1, 10, 4));
        frame->setFrameShape(QTuiFrame::Box);
        QCOMPARE(frame->frameWidth(), 1);
        QCOMPARE(frame->contentsRect(), QRect(3, 2, 8, 2));

        QTuiLCDNumber lcd;
        QCOMPARE(lcd.frameShape(), QTuiFrame::Box);
    }

    void scrollAreasShareTheScrollBase()
    {
        // The scroll area and the item views are QTuiAbstractScrollAreas,
        // which are QTuiFrames, as in Qt
        QTuiScrollArea area;
        QTuiListView list;
        QTuiTreeView tree;
        QTuiTableView table;
        for (QTuiAbstractScrollArea *a : QList<QTuiAbstractScrollArea *>{&area, &list, &tree, &table}) {
            QCOMPARE(a->scrollOffset(), 0);
            QVERIFY(qobject_cast<QTuiFrame *>(a));
        }
    }

    void itemViewsShareTheViewBase()
    {
        QStringListModel model({QStringLiteral("a"), QStringLiteral("b")});
        QTuiListView list;
        QTuiTreeView tree;
        QTuiTableView table;
        QTuiColumnView column;
        for (QTuiAbstractItemView *view : QList<QTuiAbstractItemView *>{&list, &tree, &table, &column}) {
            QVERIFY(qobject_cast<QTuiAbstractScrollArea *>(view));
            view->setModel(&model);
            QCOMPARE(view->model(), &model);
            view->setCurrentIndex(model.index(1, 0));
            QCOMPARE(view->currentIndex().row(), 1);
        }
        // The signals are the base signals
        QSignalSpy spy(&list, &QTuiAbstractItemView::activated);
        TuiKeyEvent enter(Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(&list, &enter);
        QCOMPARE(spy.count(), 1);
    }

    void rightToLeftMirrorsLayouts()
    {
        // Box: in a right-to-left widget the first item is on the right
        QTuiWidget row;
        auto *a = new LayoutTestWidget(QSize(3, 1), &row);
        auto *b = new LayoutTestWidget(QSize(4, 1), &row);
        auto *box = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        box->addWidget(a);
        box->addWidget(b);
        box->addStretch();
        row.setLayout(box);
        row.setLayoutDirection(Qt::RightToLeft);
        row.setGeometry(QRect(0, 0, 20, 1));
        QCOMPARE(a->geometry(), QRect(17, 0, 3, 1));
        QCOMPARE(b->geometry(), QRect(13, 0, 4, 1));

        // A child inherits the direction; unset goes back to the default
        QVERIFY(a->isRightToLeft());
        row.unsetLayoutDirection();
        QCOMPARE(row.layoutDirection(), Qt::LeftToRight);

        // The application default applies to widgets without a direction
        QTuiApplication::setLayoutDirection(Qt::RightToLeft);
        QTuiWidget form;
        auto *label = new LayoutTestWidget(QSize(5, 1), &form);
        auto *field = new LayoutTestWidget(QSize(5, 1), &form);
        auto *fl = new QTuiFormLayout;
        fl->addRow(label, field);
        form.setLayout(fl);
        form.setGeometry(QRect(0, 0, 20, 1));
        QTuiApplication::setLayoutDirection(Qt::LeftToRight);
        QCOMPARE(label->geometry(), QRect(15, 0, 5, 1)); // label column on the right
        QCOMPARE(field->geometry(), QRect(0, 0, 15, 1));
    }
};

QTEST_MAIN(TestQTuiLayoutBase)
#include "test_qtui_layout_base.moc"
