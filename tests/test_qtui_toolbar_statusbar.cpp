#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/QTuiStatusBar.h>
#include <QtTUI/QTuiWidgetAction.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

// A minimal concrete widget for testing addWidget
class DummyWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit DummyWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return QSize(5, 1); }
};

// A subclass of QTuiWidgetAction that actually creates a widget
class TestWidgetAction : public QTuiWidgetAction {
    Q_OBJECT
public:
    using QTuiWidgetAction::QTuiWidgetAction;
    QTuiWidget *createWidget(QTuiWidget * /*parent*/) override
    {
        // Don't parent to avoid double-delete issues between
        // QTuiWidgetAction destructor and QObject parent cleanup
        return new DummyWidget(nullptr);
    }
};

class TestQTuiToolBarStatusBar : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiToolBar tests
    // ---------------------------------------------------------------

    void toolBar_constructionWithTitle()
    {
        QTuiToolBar bar(QStringLiteral("Main"));
        QCOMPARE(bar.title(), QStringLiteral("Main"));
    }

    void toolBar_defaultConstruction()
    {
        QTuiToolBar bar;
        QCOMPARE(bar.title(), QString());
        QCOMPARE(bar.isMovable(), true);
    }

    void toolBar_addAction()
    {
        QTuiToolBar bar(QStringLiteral("Tools"));
        auto *act = new QTuiAction(QStringLiteral("Cut"), &bar);
        bar.addAction(act);
        // minimumSize should reflect the added action
        QSize ms = bar.minimumSize();
        QVERIFY(ms.width() > 0);
        QCOMPARE(ms.height(), 1);
    }

    void toolBar_addSeparator()
    {
        QTuiToolBar bar;
        auto *act1 = new QTuiAction(QStringLiteral("A"), &bar);
        bar.addAction(act1);
        bar.addSeparator();
        auto *act2 = new QTuiAction(QStringLiteral("B"), &bar);
        bar.addAction(act2);

        // minimumSize should include separator width (3 chars for " | ")
        QSize ms = bar.minimumSize();
        // "A" = 1+2=3, separator=3, "B" = 1+2=3 => 9
        QCOMPARE(ms.width(), 9);
    }

    void toolBar_addWidget()
    {
        QTuiToolBar bar;
        auto *widget = new DummyWidget();
        bar.addWidget(widget);

        QSize ms = bar.minimumSize();
        // DummyWidget minimumSize is 5x1
        QVERIFY(ms.width() >= 5);
    }

    void toolBar_minimumSizeReflectsItems()
    {
        QTuiToolBar bar;
        QSize empty = bar.minimumSize();
        QCOMPARE(empty.width(), 1); // qMax(0, 1)
        QCOMPARE(empty.height(), 1);

        auto *act = new QTuiAction(QStringLiteral("Save"), &bar);
        bar.addAction(act);
        QSize withAction = bar.minimumSize();
        // "Save" = 4 chars + 2 padding = 6
        QCOMPARE(withAction.width(), 6);
    }

    void toolBar_keyNavigationLeftRight()
    {
        QTuiToolBar bar;
        auto *act1 = new QTuiAction(QStringLiteral("A"), &bar);
        auto *act2 = new QTuiAction(QStringLiteral("B"), &bar);
        auto *act3 = new QTuiAction(QStringLiteral("C"), &bar);
        bar.addAction(act1);
        bar.addAction(act2);
        bar.addAction(act3);
        bar.setGeometry(QRect(0, 0, 30, 1));
        bar.setFocus();

        QSignalSpy spy3(act3, &QTuiAction::triggered);

        // Navigate right twice to reach "C", then trigger
        sendKey(bar, Qt::Key_Right);
        sendKey(bar, Qt::Key_Right);
        sendKey(bar, Qt::Key_Return);
        QCOMPARE(spy3.count(), 1);
    }

    void toolBar_keyNavigationWraps()
    {
        QTuiToolBar bar;
        auto *act1 = new QTuiAction(QStringLiteral("A"), &bar);
        auto *act2 = new QTuiAction(QStringLiteral("B"), &bar);
        bar.addAction(act1);
        bar.addAction(act2);
        bar.setGeometry(QRect(0, 0, 20, 1));
        bar.setFocus();

        QSignalSpy spy2(act2, &QTuiAction::triggered);

        // Navigate left from first item should wrap to last
        sendKey(bar, Qt::Key_Left);
        sendKey(bar, Qt::Key_Return);
        QCOMPARE(spy2.count(), 1);
    }

    void toolBar_enterTriggersFocusedAction()
    {
        QTuiToolBar bar;
        auto *act1 = new QTuiAction(QStringLiteral("Open"), &bar);
        bar.addAction(act1);
        bar.setGeometry(QRect(0, 0, 20, 1));
        bar.setFocus();

        QSignalSpy spy(act1, &QTuiAction::triggered);
        sendKey(bar, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
    }

    void toolBar_movableProperty()
    {
        QTuiToolBar bar;
        QCOMPARE(bar.isMovable(), true);
        bar.setMovable(false);
        QCOMPARE(bar.isMovable(), false);
        bar.setMovable(true);
        QCOMPARE(bar.isMovable(), true);
    }

    void toolBar_paintEventNoCrash()
    {
        QTuiToolBar bar(QStringLiteral("Tools"));
        auto *act = new QTuiAction(QStringLiteral("Run"), &bar);
        bar.addAction(act);
        bar.addSeparator();
        auto *widget = new DummyWidget();
        bar.addWidget(widget);
        bar.setGeometry(QRect(0, 0, 30, 1));

        ScreenBuffer buffer(30, 1);
        bar.paintEvent(buffer);

        // Verify action text appears in the buffer
        QString row;
        for (int col = 0; col < 30; ++col)
            row += buffer.backCell(col, 0).character;
        QVERIFY(row.contains(QStringLiteral("Run")));
    }

    // ---------------------------------------------------------------
    // QTuiStatusBar tests
    // ---------------------------------------------------------------

    void statusBar_construction()
    {
        QTuiStatusBar bar;
        QCOMPARE(bar.currentMessage(), QString());
    }

    void statusBar_showMessage()
    {
        QTuiStatusBar bar;
        bar.showMessage(QStringLiteral("Ready"));
        QCOMPARE(bar.currentMessage(), QStringLiteral("Ready"));
    }

    void statusBar_clearMessage()
    {
        QTuiStatusBar bar;
        bar.showMessage(QStringLiteral("Loading..."));
        bar.clearMessage();
        QCOMPARE(bar.currentMessage(), QString());
    }

    void statusBar_showMessageWithTimeout()
    {
        QTuiStatusBar bar;
        QSignalSpy spy(&bar, &QTuiStatusBar::messageChanged);

        bar.showMessage(QStringLiteral("Saved!"), 50);
        QCOMPARE(bar.currentMessage(), QStringLiteral("Saved!"));
        QCOMPARE(spy.count(), 1);

        // Wait for timeout to expire (avoid QTRY macro due to ABI compat)
        QElapsedTimer timer;
        timer.start();
        while (!timer.hasExpired(500) && !bar.currentMessage().isEmpty()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        }
        QCOMPARE(bar.currentMessage(), QString());
        // messageChanged should have been emitted again on timeout
        QVERIFY(spy.count() >= 2);
    }

    void statusBar_addPermanentWidget()
    {
        QTuiStatusBar bar;
        auto *label = new QTuiLabel(QStringLiteral("Ln 1"));
        bar.addPermanentWidget(label);
        // Widget should be parented to the status bar
        QCOMPARE(label->parentWidget(), &bar);
    }

    void statusBar_removeWidget()
    {
        QTuiStatusBar bar;
        auto *label = new QTuiLabel(QStringLiteral("Col 1"));
        bar.addPermanentWidget(label);
        bar.removeWidget(label);
        // After removal, adding again should work without issue
        bar.addPermanentWidget(label);
        QCOMPARE(label->parentWidget(), &bar);
    }

    void statusBar_messageChangedSignal()
    {
        QTuiStatusBar bar;
        QSignalSpy spy(&bar, &QTuiStatusBar::messageChanged);

        bar.showMessage(QStringLiteral("Hello"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("Hello"));

        bar.clearMessage();
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toString(), QString());
    }

    void statusBar_paintEventNoCrash()
    {
        QTuiStatusBar bar;
        bar.showMessage(QStringLiteral("Status message"));
        auto *label = new QTuiLabel(QStringLiteral("OK"));
        bar.addPermanentWidget(label);
        bar.setGeometry(QRect(0, 0, 40, 1));

        ScreenBuffer buffer(40, 1);
        bar.paintEvent(buffer);

        // Verify message text appears in the buffer
        QString row;
        for (int col = 0; col < 40; ++col)
            row += buffer.backCell(col, 0).character;
        QVERIFY(row.contains(QStringLiteral("Status message")));
    }

    // ---------------------------------------------------------------
    // QTuiWidgetAction tests
    // ---------------------------------------------------------------

    void widgetAction_inheritsFromQTuiAction()
    {
        QTuiWidgetAction wa;
        // QTuiWidgetAction inherits QTuiAction which inherits QObject
        auto *asAction = qobject_cast<QTuiAction *>(&wa);
        QVERIFY(asAction != nullptr);
    }

    void widgetAction_constructionWithText()
    {
        QTuiWidgetAction wa(QStringLiteral("Custom"));
        QCOMPARE(wa.text(), QStringLiteral("Custom"));
    }

    void widgetAction_createWidgetReturnsNullByDefault()
    {
        QTuiWidgetAction wa;
        QTuiWidget *w = wa.createWidget(nullptr);
        QCOMPARE(w, nullptr);
    }

    void widgetAction_requestWidgetCallsCreateWidget()
    {
        TestWidgetAction wa;
        QTuiWidget *w = wa.requestWidget(nullptr);
        QVERIFY(w != nullptr);
        QCOMPARE(wa.createdWidgets().size(), 1);
        QCOMPARE(wa.createdWidgets().at(0), w);
    }

    void widgetAction_releaseWidgetRemovesAndDeletes()
    {
        TestWidgetAction wa;
        QTuiWidget *w = wa.requestWidget(nullptr);
        QVERIFY(w != nullptr);
        QCOMPARE(wa.createdWidgets().size(), 1);

        wa.releaseWidget(w);
        QCOMPARE(wa.createdWidgets().size(), 0);
    }

    void widgetAction_destructorCleansUpWidgets()
    {
        QPointer<QTuiWidget> widgetPtr;
        {
            TestWidgetAction wa;
            QTuiWidget *w = wa.requestWidget(nullptr);
            widgetPtr = w;
            QVERIFY(!widgetPtr.isNull());
        }
        // After wa is destroyed, the widget should be deleted
        QVERIFY(widgetPtr.isNull());
    }

    void widgetAction_createdWidgetsReturnsList()
    {
        TestWidgetAction wa;
        QTuiWidget *w1 = wa.requestWidget(nullptr);
        QTuiWidget *w2 = wa.requestWidget(nullptr);
        QCOMPARE(wa.createdWidgets().size(), 2);
        QCOMPARE(wa.createdWidgets().at(0), w1);
        QCOMPARE(wa.createdWidgets().at(1), w2);
    }
};

QTEST_MAIN(TestQTuiToolBarStatusBar)
#include "test_qtui_toolbar_statusbar.moc"
