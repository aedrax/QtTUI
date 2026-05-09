// Enabled state + focus policy (backlog #12): disabled widgets are inert and
// skipped by focus acquisition; the Tab chain only contains widgets that
// opted in via focusPolicy.
#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiGroupBox.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

namespace {

struct Env {
    Env()
        : app(argc(), argv())
    {
        window.setGeometry(QRect(0, 0, 80, 24));
        window.setVisible(true);
        app.setMainWindow(&window);
    }

    void sendKeyToWindow(int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        TuiKeyEvent ev(key, mods);
        QCoreApplication::sendEvent(&window, &ev);
    }

    void clickAt(const QPoint &pos)
    {
        TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, pos);
        QCoreApplication::sendEvent(&window, &ev);
    }

    static int &argc()
    {
        static int c = 1;
        return c;
    }
    static char **argv()
    {
        static char arg0[] = "test";
        static char *args[] = { arg0, nullptr };
        return args;
    }

    QTuiApplication app;
    QTuiMainWindow window;
};

} // namespace

class TestEnabledFocusPolicy : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // Enabled state basics
    // -----------------------------------------------------------------------
    void enabledDefaultsAndSignal()
    {
        QTuiPushButton btn(QStringLiteral("Go"));
        QVERIFY(btn.isEnabled());
        QVERIFY(btn.isEffectivelyEnabled());

        QSignalSpy spy(&btn, &QTuiWidget::enabledChanged);
        btn.setEnabled(false);
        QVERIFY(!btn.isEnabled());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);

        // No signal when the value does not change
        btn.setEnabled(false);
        QCOMPARE(spy.count(), 1);
    }

    void setEnabledMarksDirty()
    {
        Env env;
        QTuiPushButton btn(QStringLiteral("Go"), &env.window);
        btn.setGeometry(QRect(0, 0, 10, 3));
        btn.clearDirtyRegions();

        btn.setEnabled(false);
        QVERIFY(btn.isDirty());
    }

    void disabledParentDisablesChildren()
    {
        Env env;
        QTuiGroupBox box(QStringLiteral("Box"), &env.window);
        box.setGeometry(QRect(0, 0, 30, 10));
        auto *child = new QTuiPushButton(QStringLiteral("Go"), &box);
        child->setGeometry(QRect(2, 2, 10, 3));

        QVERIFY(child->isEffectivelyEnabled());

        box.setEnabled(false);
        QVERIFY(child->isEnabled());              // own flag untouched
        QVERIFY(!child->isEffectivelyEnabled());  // but effectively disabled

        box.setEnabled(true);
        QVERIFY(child->isEffectivelyEnabled());
    }

    void disabledWidgetIgnoresKeysAndClicks()
    {
        Env env;
        QTuiPushButton btn(QStringLiteral("Go"), &env.window);
        btn.setGeometry(QRect(0, 0, 10, 3));
        QSignalSpy clicks(&btn, &QTuiPushButton::clicked);

        btn.setFocus();
        btn.setEnabled(false);

        // Key events are consumed by the base event() without reaching the
        // handler
        TuiKeyEvent keyEv(Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(&btn, &keyEv);
        QCOMPARE(clicks.count(), 0);

        // Mouse presses routed through the window are also inert
        env.clickAt(QPoint(2, 1));
        QCOMPARE(clicks.count(), 0);
    }

    void disablingFocusedWidgetClearsFocus()
    {
        Env env;
        QTuiPushButton btn(QStringLiteral("Go"), &env.window);
        btn.setGeometry(QRect(0, 0, 10, 3));
        btn.setFocus();
        QVERIFY(btn.hasFocus());

        btn.setEnabled(false);
        QVERIFY(!btn.hasFocus());
        QVERIFY(QTuiWidget::focusWidget() == nullptr);
    }

    void disablingAncestorClearsDescendantFocus()
    {
        Env env;
        QTuiGroupBox box(QStringLiteral("Box"), &env.window);
        box.setGeometry(QRect(0, 0, 30, 10));
        auto *child = new QTuiPushButton(QStringLiteral("Go"), &box);
        child->setGeometry(QRect(2, 2, 10, 3));
        child->setFocus();
        QVERIFY(child->hasFocus());

        box.setEnabled(false);
        QVERIFY(!child->hasFocus());
    }

    // -----------------------------------------------------------------------
    // Focus policy
    // -----------------------------------------------------------------------
    void defaultPoliciesPerWidgetKind()
    {
        QTuiLabel label(QStringLiteral("L"));
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);

        QTuiPushButton btn(QStringLiteral("B"));
        QCOMPARE(btn.focusPolicy(), Qt::StrongFocus);

        QTuiLineEdit edit;
        QCOMPARE(edit.focusPolicy(), Qt::StrongFocus);

        // Group boxes only become focusable when checkable
        QTuiGroupBox box(QStringLiteral("G"));
        QCOMPARE(box.focusPolicy(), Qt::NoFocus);
        box.setCheckable(true);
        QCOMPARE(box.focusPolicy(), Qt::StrongFocus);
        box.setCheckable(false);
        QCOMPARE(box.focusPolicy(), Qt::NoFocus);
    }

    void setFocusIsNoOpForNoFocusWidgets()
    {
        Env env;
        QTuiPushButton btn(QStringLiteral("Go"), &env.window);
        btn.setGeometry(QRect(0, 0, 10, 3));
        QTuiLabel label(QStringLiteral("Info"), &env.window);
        label.setGeometry(QRect(0, 4, 10, 1));

        btn.setFocus();
        QVERIFY(btn.hasFocus());

        label.setFocus(); // NoFocus: must not steal focus
        QVERIFY(!label.hasFocus());
        QVERIFY(btn.hasFocus());
    }

    void tabChainSkipsNoFocusAndDisabledWidgets()
    {
        Env env;
        auto *btn1 = new QTuiPushButton(QStringLiteral("One"), &env.window);
        btn1->setGeometry(QRect(0, 0, 10, 3));
        auto *label = new QTuiLabel(QStringLiteral("Label"), &env.window);
        label->setGeometry(QRect(0, 3, 10, 1));
        auto *btn2 = new QTuiPushButton(QStringLiteral("Two"), &env.window);
        btn2->setGeometry(QRect(0, 4, 10, 3));
        auto *btn3 = new QTuiPushButton(QStringLiteral("Three"), &env.window);
        btn3->setGeometry(QRect(0, 7, 10, 3));

        btn2->setEnabled(false);

        btn1->setFocus();
        QVERIFY(btn1->hasFocus());

        // Tab skips the NoFocus label and the disabled button
        env.sendKeyToWindow(Qt::Key_Tab);
        QVERIFY2(btn3->hasFocus(), "Tab must skip the label and disabled btn2");

        // Wraps back to btn1 (label still skipped)
        env.sendKeyToWindow(Qt::Key_Tab);
        QVERIFY(btn1->hasFocus());

        // Shift+Tab backward also skips them
        env.sendKeyToWindow(Qt::Key_Backtab, Qt::ShiftModifier);
        QVERIFY(btn3->hasFocus());
    }

    void tabChainSkipsHiddenWidgets()
    {
        Env env;
        auto *btn1 = new QTuiPushButton(QStringLiteral("One"), &env.window);
        btn1->setGeometry(QRect(0, 0, 10, 3));
        auto *btn2 = new QTuiPushButton(QStringLiteral("Two"), &env.window);
        btn2->setGeometry(QRect(0, 4, 10, 3));
        auto *btn3 = new QTuiPushButton(QStringLiteral("Three"), &env.window);
        btn3->setGeometry(QRect(0, 8, 10, 3));
        btn2->setVisible(false);

        btn1->setFocus();
        env.sendKeyToWindow(Qt::Key_Tab);
        QVERIFY(btn3->hasFocus());
    }

    void clickingNoFocusWidgetDoesNotStealFocus()
    {
        Env env;
        auto *btn = new QTuiPushButton(QStringLiteral("Go"), &env.window);
        btn->setGeometry(QRect(0, 0, 10, 3));
        auto *label = new QTuiLabel(QStringLiteral("Some label"), &env.window);
        label->setGeometry(QRect(0, 5, 15, 1));

        btn->setFocus();
        QVERIFY(btn->hasFocus());

        env.clickAt(QPoint(2, 5)); // inside the label
        QVERIFY2(btn->hasFocus(), "Clicking a NoFocus label must not move focus");
        QVERIFY(!label->hasFocus());
    }

    void clickingClickFocusWidgetFocusesIt()
    {
        Env env;
        auto *btn1 = new QTuiPushButton(QStringLiteral("One"), &env.window);
        btn1->setGeometry(QRect(0, 0, 10, 3));
        auto *btn2 = new QTuiPushButton(QStringLiteral("Two"), &env.window);
        btn2->setGeometry(QRect(0, 4, 10, 3));

        btn1->setFocus();
        env.clickAt(QPoint(2, 5)); // inside btn2
        QVERIFY(btn2->hasFocus());
    }

    void clickingDisabledWidgetDoesNotFocusIt()
    {
        Env env;
        auto *btn1 = new QTuiPushButton(QStringLiteral("One"), &env.window);
        btn1->setGeometry(QRect(0, 0, 10, 3));
        auto *btn2 = new QTuiPushButton(QStringLiteral("Two"), &env.window);
        btn2->setGeometry(QRect(0, 4, 10, 3));
        btn2->setEnabled(false);

        btn1->setFocus();
        env.clickAt(QPoint(2, 5)); // inside disabled btn2
        QVERIFY(!btn2->hasFocus());
        QVERIFY(btn1->hasFocus());
    }

    // -----------------------------------------------------------------------
    // Group box: unchecked disables (not hides) children
    // -----------------------------------------------------------------------
    void groupBoxUncheckDisablesChildrenNotHides()
    {
        Env env;
        QTuiGroupBox box(QStringLiteral("Options"), &env.window);
        box.setGeometry(QRect(0, 0, 30, 10));
        box.setCheckable(true);
        box.setChecked(true);

        auto *child = new QTuiPushButton(QStringLiteral("Go"), &box);
        child->setGeometry(QRect(2, 2, 10, 3));
        QSignalSpy clicks(child, &QTuiPushButton::clicked);

        box.setChecked(false);
        QVERIFY2(child->isVisible(), "children stay visible (Qt semantics)");
        QVERIFY(!child->isEnabled());
        QVERIFY(!child->isEffectivelyEnabled());

        // ... and are inert while disabled
        env.clickAt(QPoint(4, 3));
        QCOMPARE(clicks.count(), 0);

        box.setChecked(true);
        QVERIFY(child->isEnabled());
        env.clickAt(QPoint(4, 3));
        QCOMPARE(clicks.count(), 1);
    }
};

QTEST_APPLESS_MAIN(TestEnabledFocusPolicy)
#include "test_enabled_focuspolicy.moc"
