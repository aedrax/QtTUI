#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QtTUI/QTuiStackedLayout.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

class StackedTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit StackedTestWidget(QSize minSize = QSize(0, 0), QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return m_minSize; }
private:
    QSize m_minSize;
};

class TestQTuiStackedLayout : public QObject {
    Q_OBJECT
private slots:

    // Req 31.1: inherits from QTuiLayout
    void inheritsFromQTuiLayout()
    {
        QTuiStackedLayout layout;
        QTuiLayout *base = &layout;
        QVERIFY(base != nullptr);
    }

    // Req 31.3: addWidget returns index
    void addWidgetReturnsIndex()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();
        auto *w2 = new StackedTestWidget();

        QCOMPARE(layout.addWidget(w0), 0);
        QCOMPARE(layout.addWidget(w1), 1);
        QCOMPARE(layout.addWidget(w2), 2);

        delete w0; delete w1; delete w2;
    }

    // Req 31.3: addWidget(nullptr) returns -1
    void addWidgetNullReturnsNegative()
    {
        QTuiStackedLayout layout;
        QCOMPARE(layout.addWidget(nullptr), -1);
    }

    // Req 31.2: first widget becomes current
    void firstWidgetBecomesCurrent()
    {
        QTuiStackedLayout layout;
        auto *w = new StackedTestWidget();

        layout.addWidget(w);
        QCOMPARE(layout.currentIndex(), 0);
        QVERIFY(w->isVisible());

        delete w;
    }

    // Req 31.2: subsequent widgets are hidden
    void subsequentWidgetsAreHidden()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();
        auto *w2 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);
        layout.addWidget(w2);

        QVERIFY(w0->isVisible());
        QVERIFY(!w1->isVisible());
        QVERIFY(!w2->isVisible());

        delete w0; delete w1; delete w2;
    }

    // Req 31.4: setCurrentIndex shows correct widget
    void setCurrentIndexShowsCorrectWidget()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();
        auto *w2 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);
        layout.addWidget(w2);

        layout.setCurrentIndex(1);
        QCOMPARE(layout.currentIndex(), 1);
        QVERIFY(!w0->isVisible());
        QVERIFY(w1->isVisible());
        QVERIFY(!w2->isVisible());
        QCOMPARE(layout.currentWidget(), w1);

        delete w0; delete w1; delete w2;
    }

    // Req 31.5: setCurrentIndex emits currentChanged signal
    void setCurrentIndexEmitsSignal()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);

        QSignalSpy spy(&layout, &QTuiStackedLayout::currentChanged);
        layout.setCurrentIndex(1);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);

        delete w0; delete w1;
    }

    // Req 31.4: out-of-range index is ignored
    void setCurrentIndexOutOfRangeIgnored()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);

        QSignalSpy spy(&layout, &QTuiStackedLayout::currentChanged);

        layout.setCurrentIndex(-1);
        QCOMPARE(layout.currentIndex(), 0);

        layout.setCurrentIndex(999);
        QCOMPARE(layout.currentIndex(), 0);

        // No signal should have been emitted for invalid indices
        QCOMPARE(spy.count(), 0);

        delete w0; delete w1;
    }

    // Req 31.4: setCurrentWidget by pointer
    void setCurrentWidgetByPointer()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();
        auto *w2 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);
        layout.addWidget(w2);

        layout.setCurrentWidget(w2);
        QCOMPARE(layout.currentIndex(), 2);
        QCOMPARE(layout.currentWidget(), w2);
        QVERIFY(w2->isVisible());
        QVERIFY(!w0->isVisible());

        delete w0; delete w1; delete w2;
    }

    // Req 31.6: apply gives current widget full area
    void applyGivesCurrentWidgetFullArea()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget(QSize(10, 5));
        auto *w1 = new StackedTestWidget(QSize(20, 15));

        layout.addWidget(w0);
        layout.addWidget(w1);

        layout.apply(QRect(5, 10, 40, 20));

        // Current widget (w0) should get the full contents rect
        QCOMPARE(w0->geometry(), QRect(5, 10, 40, 20));
        QVERIFY(w0->isVisible());

        delete w0; delete w1;
    }

    // Req 31.2: apply hides non-current widgets
    void applyHidesNonCurrentWidgets()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();
        auto *w2 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);
        layout.addWidget(w2);

        layout.setCurrentIndex(1);
        layout.apply(QRect(0, 0, 40, 20));

        QVERIFY(!w0->isVisible());
        QVERIFY(w1->isVisible());
        QVERIFY(!w2->isVisible());

        delete w0; delete w1; delete w2;
    }

    // Req 31.3: widget accessor
    void widgetAccessor()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);

        QCOMPARE(layout.widget(0), w0);
        QCOMPARE(layout.widget(1), w1);
        QCOMPARE(layout.widget(-1), nullptr);
        QCOMPARE(layout.widget(99), nullptr);

        delete w0; delete w1;
    }

    // Req 31.5: setting same index doesn't emit signal
    void currentChangedNotEmittedForSameIndex()
    {
        QTuiStackedLayout layout;
        auto *w0 = new StackedTestWidget();
        auto *w1 = new StackedTestWidget();

        layout.addWidget(w0);
        layout.addWidget(w1);

        layout.setCurrentIndex(1);

        QSignalSpy spy(&layout, &QTuiStackedLayout::currentChanged);
        layout.setCurrentIndex(1);  // same index again
        QCOMPARE(spy.count(), 0);

        delete w0; delete w1;
    }
};

QTEST_MAIN(TestQTuiStackedLayout)
#include "test_qtui_stacked_layout.moc"
