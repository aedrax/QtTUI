// Focus control: QTuiWidget::setTabOrder().
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
};

QTEST_APPLESS_MAIN(TestFocusControl)
#include "test_focus_control.moc"
