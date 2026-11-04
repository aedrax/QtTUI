// Focus control: QTuiWidget::setTabOrder() and focusNextPrevChild().
#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiDialog.h>
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

    void sendKey(int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                 const QString &text = QString())
    {
        TuiKeyEvent ev(key, mods, text);
        QCoreApplication::sendEvent(&window, &ev);
    }
    void tab() { sendKey(Qt::Key_Tab); }
    void backtab() { sendKey(Qt::Key_Backtab, Qt::ShiftModifier); }

    QTuiPushButton *button(const char *name, QTuiWidget *parent = nullptr)
    {
        return new QTuiPushButton(QLatin1String(name), parent ? parent : &window);
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

// A widget that records the keys it gets and answers focusNextPrevChild()
// from a fixed plan
class Probe : public QTuiWidget {
public:
    explicit Probe(QTuiWidget *parent)
        : QTuiWidget(parent)
    {
        setFocusPolicy(Qt::StrongFocus);
    }

    enum Plan { Default, KeepTab, GoToTarget };
    Plan plan = Default;
    QTuiWidget *target = nullptr;
    QList<int> keys;
    QList<bool> asked;

    using QTuiWidget::focusNextChild;
    using QTuiWidget::focusPreviousChild;

protected:
    bool focusNextPrevChild(bool next) override
    {
        asked.append(next);
        if (plan == KeepTab)
            return false;
        if (plan == GoToTarget) {
            target->setFocus();
            return true;
        }
        return QTuiWidget::focusNextPrevChild(next);
    }
    void keyPressEvent(TuiKeyEvent *event) override { keys.append(event->key()); }
};

} // namespace

class TestFocusControl : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // setTabOrder()
    // -----------------------------------------------------------------------

    void tabOrderChangesTheChain()
    {
        Env env;
        auto *a = env.button("a");
        auto *b = env.button("b");
        auto *c = env.button("c");

        // Tree order is a, b, c; the tab order becomes c, b, a
        QTuiWidget::setTabOrder(c, b);
        QTuiWidget::setTabOrder(b, a);

        c->setFocus();
        env.tab();
        QVERIFY(b->hasFocus());
        env.tab();
        QVERIFY(a->hasFocus());
        env.tab();
        QVERIFY(c->hasFocus()); // wraps

        env.backtab();
        QVERIFY(a->hasFocus());
        env.backtab();
        QVERIFY(b->hasFocus());
        env.backtab();
        QVERIFY(c->hasFocus());
    }

    void tabOrderSkipsWidgetsThatTakeNoTabFocus()
    {
        Env env;
        auto *a = env.button("a");
        auto *b = env.button("b");
        auto *c = env.button("c");
        auto *d = env.button("d");
        auto *e = env.button("e");
        // e, d, c, b, a
        QTuiWidget::setTabOrder(e, d);
        QTuiWidget::setTabOrder(d, c);
        QTuiWidget::setTabOrder(c, b);
        QTuiWidget::setTabOrder(b, a);

        d->setVisible(false);
        c->setEnabled(false);
        b->setFocusPolicy(Qt::ClickFocus);

        e->setFocus();
        env.tab();
        QVERIFY2(a->hasFocus(), "Tab skips the hidden, disabled and ClickFocus widgets");
        env.backtab();
        QVERIFY(e->hasFocus());

        // The pairs still hold when the widgets come back
        d->setVisible(true);
        c->setEnabled(true);
        b->setFocusPolicy(Qt::StrongFocus);
        env.tab();
        QVERIFY(d->hasFocus());
        env.tab();
        QVERIFY(c->hasFocus());
        env.tab();
        QVERIFY(b->hasFocus());
    }

    void tabOrderSkipsTheChildrenOfAHiddenParent()
    {
        Env env;
        auto *a = env.button("a");
        auto *box = new QTuiWidget(&env.window);
        auto *inBox = env.button("inBox", box);
        auto *c = env.button("c");
        QTuiWidget::setTabOrder(a, c);
        QTuiWidget::setTabOrder(c, inBox);
        box->setVisible(false);

        a->setFocus();
        env.tab();
        QVERIFY(c->hasFocus());
        env.tab();
        QVERIFY(a->hasFocus());
    }

    void tabOrderWorksInADialog()
    {
        Env env;
        auto *outside = env.button("outside");
        QTuiDialog dialog;
        auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        auto *one = env.button("one", &dialog);
        auto *two = env.button("two", &dialog);
        auto *three = env.button("three", &dialog);
        row->addWidget(one);
        row->addWidget(two);
        row->addWidget(three);
        dialog.setLayout(row);
        QTuiWidget::setTabOrder(one, three);
        QTuiWidget::setTabOrder(three, two);
        // A pair with a widget out of the dialog does nothing in the dialog
        QTuiWidget::setTabOrder(outside, two);

        dialog.open();
        one->setFocus();
        env.tab();
        QVERIFY(three->hasFocus());
        env.tab();
        QVERIFY(two->hasFocus());
        env.tab();
        QVERIFY(one->hasFocus());
        env.backtab();
        QVERIFY(two->hasFocus());
        env.backtab();
        QVERIFY(three->hasFocus());
    }

    void tabOrderIsSafeWithDeletedWidgets()
    {
        Env env;
        auto *a = env.button("a");
        auto *b = env.button("b");
        auto *c = env.button("c");
        QTuiWidget::setTabOrder(c, b);
        QTuiWidget::setTabOrder(b, a);
        QTuiWidget::setTabOrder(nullptr, a);
        QTuiWidget::setTabOrder(a, nullptr);
        QTuiWidget::setTabOrder(a, a);
        delete b;

        // The pairs with b are gone: tree order a, c
        a->setFocus();
        env.tab();
        QVERIFY(c->hasFocus());
        env.tab();
        QVERIFY(a->hasFocus());
    }

    // -----------------------------------------------------------------------
    // focusNextPrevChild()
    // -----------------------------------------------------------------------

    void tabAsksTheFocusWidget()
    {
        Env env;
        auto *probe = new Probe(&env.window);
        auto *b = env.button("b");
        auto *c = env.button("c");

        // The default goes up to the window, which moves the focus
        probe->setFocus();
        env.tab();
        QCOMPARE(probe->asked, QList<bool>{true});
        QVERIFY(b->hasFocus());
        QVERIFY(probe->keys.isEmpty());

        probe->setFocus();
        env.backtab();
        QCOMPARE(probe->asked, (QList<bool>{true, false}));
        QVERIFY(c->hasFocus());
    }

    void widgetDecidesTheNextFocusWidget()
    {
        Env env;
        auto *probe = new Probe(&env.window);
        env.button("b");
        auto *c = env.button("c");
        probe->plan = Probe::GoToTarget;
        probe->target = c;

        probe->setFocus();
        env.tab();
        QVERIFY(c->hasFocus());
        QVERIFY(probe->keys.isEmpty());
    }

    void widgetThatReturnsFalseGetsTabAsAKey()
    {
        Env env;
        auto *probe = new Probe(&env.window);
        env.button("b");
        probe->plan = Probe::KeepTab;

        probe->setFocus();
        env.tab();
        env.backtab();
        QVERIFY(probe->hasFocus());
        QCOMPARE(probe->keys, (QList<int>{Qt::Key_Tab, Qt::Key_Backtab}));
    }

    void parentDecidesForItsChildren()
    {
        Env env;
        auto *box = new Probe(&env.window);
        auto *inBox = env.button("inBox", box);
        auto *inBox2 = env.button("inBox2", box);
        auto *out = env.button("out");
        box->plan = Probe::GoToTarget;
        box->target = out;

        // The call goes from the focus widget up through its parent
        inBox->setFocus();
        env.tab();
        QVERIFY(out->hasFocus());
        QVERIFY(!inBox2->hasFocus());
        QCOMPARE(box->asked, QList<bool>{true});
    }

    void focusNextChildMovesTheFocusFromCode()
    {
        Env env;
        auto *probe = new Probe(&env.window);
        auto *b = env.button("b");
        auto *c = env.button("c");

        probe->setFocus();
        QVERIFY(probe->focusNextChild());
        QVERIFY(b->hasFocus());
        probe->setFocus();
        QVERIFY(probe->focusPreviousChild());
        QVERIFY(c->hasFocus());

        // No main window above the widget: nothing moves
        Probe alone(nullptr);
        QVERIFY(!alone.focusNextChild());
    }

    void focusNextPrevChildWorksInADialog()
    {
        Env env;
        env.button("outside");
        QTuiDialog dialog;
        auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        auto *probe = new Probe(&dialog);
        auto *two = env.button("two", &dialog);
        auto *three = env.button("three", &dialog);
        row->addWidget(probe);
        row->addWidget(two);
        row->addWidget(three);
        dialog.setLayout(row);
        dialog.open();

        // The default stays in the dialog
        probe->setFocus();
        env.backtab();
        QVERIFY(three->hasFocus());
        env.tab();
        QVERIFY(probe->hasFocus());

        probe->plan = Probe::KeepTab;
        env.tab();
        QVERIFY(probe->hasFocus());
        QCOMPARE(probe->keys, QList<int>{Qt::Key_Tab});

        probe->plan = Probe::GoToTarget;
        probe->target = three;
        env.tab();
        QVERIFY(three->hasFocus());
    }

    void tabWithoutFocusWidgetEntersTheChain()
    {
        Env env;
        auto *a = env.button("a");
        env.button("b");
        QVERIFY(!QTuiWidget::focusWidget());
        env.tab();
        QVERIFY(a->hasFocus());
    }
};

QTEST_APPLESS_MAIN(TestFocusControl)
#include "test_focus_control.moc"
