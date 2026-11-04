// Focus control: QTuiWidget::setTabOrder(), focusNextPrevChild() and the
// label buddy.
#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/ScreenBuffer.h>
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
    void alt(char letter)
    {
        sendKey(Qt::Key_A + (letter - 'a'), Qt::AltModifier, QString(QLatin1Char(letter)));
    }
    // The text of a screen row after a render pass
    QString row(int y, int x, int length)
    {
        QString text;
        for (int i = 0; i < length; ++i)
            text += app.screenBuffer()->backCell(x + i, y).text();
        return text;
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

    // -----------------------------------------------------------------------
    // Label buddy
    // -----------------------------------------------------------------------

    void ampersandIsTextWithoutABuddy()
    {
        Env env;
        QTuiLabel label(QStringLiteral("&Name"));
        QCOMPARE(label.displayText(), QStringLiteral("&Name"));
        QVERIFY(label.mnemonic().isNull());
        QCOMPARE(label.minimumSize().width(), 5);
    }

    void buddyHidesTheAmpersandAndUnderlinesTheLetter()
    {
        Env env;
        QTuiLineEdit edit;
        QTuiLabel label(QStringLiteral("Last &Name"));
        label.setBuddy(&edit);
        QCOMPARE(label.buddy(), &edit);
        QCOMPARE(label.displayText(), QStringLiteral("Last Name"));
        QCOMPARE(label.mnemonic(), QChar(QLatin1Char('N')));
        QCOMPARE(label.minimumSize().width(), 9);

        label.setGeometry(QRect(0, 0, 12, 1));
        ScreenBuffer buffer(12, 1);
        label.paintEvent(buffer);
        for (int x = 0; x < 9; ++x) {
            QCOMPARE(buffer.backCell(x, 0).text(), QStringLiteral("Last Name").mid(x, 1));
            QCOMPARE(buffer.backCell(x, 0).underline, x == 5);
        }
    }

    void doubleAmpersandShowsOne()
    {
        Env env;
        QTuiLineEdit edit;
        QTuiLabel label(QStringLiteral("Cut && &Paste && &more&"));
        label.setBuddy(&edit);
        // Only the first mnemonic counts; an & at the end is text
        QCOMPARE(label.displayText(), QStringLiteral("Cut & Paste & more&"));
        QCOMPARE(label.mnemonic(), QChar(QLatin1Char('P')));

        label.setTextFormat(Qt::MarkdownText);
        label.setText(QStringLiteral("**&Bold**"));
        QCOMPARE(label.displayText(), QStringLiteral("Bold"));
        QCOMPARE(label.mnemonic(), QChar(QLatin1Char('B')));

        // In rich text an & starts an entity
        label.setTextFormat(Qt::RichText);
        label.setText(QStringLiteral("Tom &amp; &Jerry"));
        QCOMPARE(label.displayText(), QStringLiteral("Tom & &Jerry"));
        QVERIFY(label.mnemonic().isNull());
    }

    void setBuddyRepaintsTheLabel()
    {
        Env env;
        auto *label = new QTuiLabel(QStringLiteral("&Name"), &env.window);
        label->setGeometry(QRect(2, 1, 8, 1));
        auto *edit = new QTuiLineEdit(&env.window);
        edit->setGeometry(QRect(2, 3, 10, 1));
        QCoreApplication::processEvents();
        QCOMPARE(env.row(1, 2, 5), QStringLiteral("&Name"));

        label->setBuddy(edit);
        QVERIFY(label->isDirty());
        QCoreApplication::processEvents();
        QCOMPARE(env.row(1, 2, 5), QStringLiteral("Name "));
        QVERIFY(env.app.screenBuffer()->backCell(2, 1).underline);

        // The same buddy again changes nothing
        label->setBuddy(edit);
        QVERIFY(!label->isDirty());

        // A deleted buddy is no buddy
        delete edit;
        QVERIFY(!label->buddy());
        QCoreApplication::processEvents();
        QCOMPARE(env.row(1, 2, 5), QStringLiteral("&Name"));
        env.alt('n'); // must not crash
    }

    void altLetterGivesTheFocusToTheBuddy()
    {
        Env env;
        auto *other = env.button("other");
        auto *label = new QTuiLabel(QStringLiteral("&Name"), &env.window);
        auto *edit = new QTuiLineEdit(&env.window);
        label->setBuddy(edit);
        other->setFocus();

        env.alt('x');
        QVERIFY(other->hasFocus());
        env.alt('n');
        QVERIFY(edit->hasFocus());

        // The kitty keyboard protocol sends Alt+letter without text
        other->setFocus();
        env.sendKey(Qt::Key_N, Qt::AltModifier);
        QVERIFY(edit->hasFocus());

        // Also while a text editor has the focus, and for the other case
        auto *label2 = new QTuiLabel(QStringLiteral("&Other"), &env.window);
        label2->setBuddy(other);
        env.sendKey(Qt::Key_O, Qt::AltModifier | Qt::ShiftModifier, QStringLiteral("O"));
        QVERIFY(other->hasFocus());
    }

    void altLetterNeedsAVisibleLabelAndAUsableBuddy()
    {
        Env env;
        auto *other = env.button("other");
        auto *box = new QTuiWidget(&env.window);
        auto *label = new QTuiLabel(QStringLiteral("&Name"), box);
        auto *edit = new QTuiLineEdit(&env.window);
        label->setBuddy(edit);
        other->setFocus();

        box->setVisible(false); // the parent of the label
        env.alt('n');
        QVERIFY(other->hasFocus());
        box->setVisible(true);

        edit->setEnabled(false);
        env.alt('n');
        QVERIFY(other->hasFocus());
        edit->setEnabled(true);

        edit->setVisible(false);
        env.alt('n');
        QVERIFY(other->hasFocus());
        edit->setVisible(true);

        edit->setFocusPolicy(Qt::NoFocus);
        env.alt('n');
        QVERIFY(other->hasFocus());
        edit->setFocusPolicy(Qt::StrongFocus);

        env.alt('n');
        QVERIFY(edit->hasFocus());
    }

    void labelMnemonicComesBeforeTheMenuBar()
    {
        Env env;
        auto *bar = new QTuiMenuBar(&env.window);
        QTuiMenu *menu = bar->addMenu(QStringLiteral("&File"));
        env.window.setMenuBar(bar);
        auto *label = new QTuiLabel(QStringLiteral("&Find"), &env.window);
        auto *edit = new QTuiLineEdit(&env.window);
        label->setBuddy(edit);

        env.alt('f');
        QVERIFY(edit->hasFocus());
        QVERIFY(!menu->isVisible());

        // Without a usable buddy the menu bar gets the key
        edit->setEnabled(false);
        env.alt('f');
        QVERIFY(menu->isVisible());
    }

    void labelMnemonicWorksInADialog()
    {
        Env env;
        auto *outsideLabel = new QTuiLabel(QStringLiteral("&Outside"), &env.window);
        auto *outside = env.button("outside");
        outsideLabel->setBuddy(outside);

        QTuiDialog dialog;
        auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        auto *ok = env.button("ok", &dialog);
        auto *label = new QTuiLabel(QStringLiteral("&Name"), &dialog);
        auto *edit = new QTuiLineEdit(&dialog);
        label->setBuddy(edit);
        row->addWidget(ok);
        row->addWidget(label);
        row->addWidget(edit);
        dialog.setLayout(row);
        dialog.open();
        QVERIFY(ok->hasFocus());

        env.alt('o'); // a label out of the dialog
        QVERIFY(ok->hasFocus());
        env.alt('n');
        QVERIFY(edit->hasFocus());
    }
};

QTEST_APPLESS_MAIN(TestFocusControl)
#include "test_focus_control.moc"
