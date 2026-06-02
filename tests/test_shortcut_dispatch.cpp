// Global shortcut dispatch (backlog #9): QTuiKeySequence parsing and
// QTuiMainWindow's action-shortcut / menu-mnemonic dispatch.
#include <QtTest/QtTest>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiKeySequence.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

namespace {

// Shared per-test scaffolding: an application, a main window sized to the
// terminal, and helpers to push key events through the window's dispatch.
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

class TestShortcutDispatch : public QObject {
    Q_OBJECT

private slots:

    // -----------------------------------------------------------------------
    // QTuiKeySequence parsing
    // -----------------------------------------------------------------------
    void parserValidSequences()
    {
        auto seq = QTuiKeySequence::fromString(QStringLiteral("Ctrl+S"));
        QVERIFY(seq.isValid());
        QCOMPARE(seq.key(), int(Qt::Key_S));
        QCOMPARE(seq.modifiers(), Qt::KeyboardModifiers(Qt::ControlModifier));

        seq = QTuiKeySequence::fromString(QStringLiteral("Ctrl+Shift+Z"));
        QVERIFY(seq.isValid());
        QCOMPARE(seq.key(), int(Qt::Key_Z));
        QCOMPARE(seq.modifiers(), Qt::ControlModifier | Qt::ShiftModifier);

        seq = QTuiKeySequence::fromString(QStringLiteral("Alt+F4"));
        QVERIFY(seq.isValid());
        QCOMPARE(seq.key(), int(Qt::Key_F4));
        QCOMPARE(seq.modifiers(), Qt::KeyboardModifiers(Qt::AltModifier));

        seq = QTuiKeySequence::fromString(QStringLiteral("F5"));
        QVERIFY(seq.isValid());
        QCOMPARE(seq.key(), int(Qt::Key_F5));
        QCOMPARE(seq.modifiers(), Qt::KeyboardModifiers(Qt::NoModifier));

        seq = QTuiKeySequence::fromString(QStringLiteral("F35"));
        QCOMPARE(seq.key(), int(Qt::Key_F35));

        seq = QTuiKeySequence::fromString(QStringLiteral("Meta+Home"));
        QVERIFY(seq.isValid());
        QCOMPARE(seq.key(), int(Qt::Key_Home));
        QCOMPARE(seq.modifiers(), Qt::KeyboardModifiers(Qt::MetaModifier));

        // Bare letter and digit
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Q")).key(), int(Qt::Key_Q));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("7")).key(), int(Qt::Key_7));
    }

    void parserNamedKeys()
    {
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Escape")).key(), int(Qt::Key_Escape));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Esc")).key(), int(Qt::Key_Escape));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Tab")).key(), int(Qt::Key_Tab));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Backspace")).key(), int(Qt::Key_Backspace));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Delete")).key(), int(Qt::Key_Delete));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Del")).key(), int(Qt::Key_Delete));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Enter")).key(), int(Qt::Key_Return));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Return")).key(), int(Qt::Key_Return));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Space")).key(), int(Qt::Key_Space));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Home")).key(), int(Qt::Key_Home));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("End")).key(), int(Qt::Key_End));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("PgUp")).key(), int(Qt::Key_PageUp));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("PageUp")).key(), int(Qt::Key_PageUp));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("PgDown")).key(), int(Qt::Key_PageDown));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("PageDown")).key(), int(Qt::Key_PageDown));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Insert")).key(), int(Qt::Key_Insert));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Ins")).key(), int(Qt::Key_Insert));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Up")).key(), int(Qt::Key_Up));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Down")).key(), int(Qt::Key_Down));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Left")).key(), int(Qt::Key_Left));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("Right")).key(), int(Qt::Key_Right));
    }

    void parserCaseInsensitive()
    {
        const auto a = QTuiKeySequence::fromString(QStringLiteral("ctrl+shift+s"));
        const auto b = QTuiKeySequence::fromString(QStringLiteral("CTRL+SHIFT+S"));
        const auto c = QTuiKeySequence::fromString(QStringLiteral("Control+Shift+s"));
        QVERIFY(a.isValid());
        QVERIFY(a == b);
        QVERIFY(a == c);

        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("alt+f4")).key(), int(Qt::Key_F4));
        QCOMPARE(QTuiKeySequence::fromString(QStringLiteral("ESCAPE")).key(), int(Qt::Key_Escape));
    }

    void parserInvalidSequences()
    {
        const QStringList invalid = {
            QString(),
            QStringLiteral("   "),
            QStringLiteral("Ctrl+"),
            QStringLiteral("+S"),
            QStringLiteral("Ctrl++"),
            QStringLiteral("Foo+S"),
            QStringLiteral("Ctrl+Foo"),
            QStringLiteral("Ctrl"), // modifier alone is not a key
            QStringLiteral("F0"),
            QStringLiteral("F36"),
            QStringLiteral("Ctrl+Alt"), // ends on a modifier
        };
        for (const QString &s : invalid) {
            const auto seq = QTuiKeySequence::fromString(s);
            QVERIFY2(!seq.isValid(), qPrintable(s));
            // Invalid sequences never match anything
            QVERIFY(!seq.matches(Qt::Key_S, Qt::ControlModifier));
            QVERIFY(!seq.matches(0, Qt::NoModifier));
        }
    }

    void parserExactModifierMatching()
    {
        const auto seq = QTuiKeySequence::fromString(QStringLiteral("Ctrl+S"));
        QVERIFY(seq.matches(Qt::Key_S, Qt::ControlModifier));
        QVERIFY(!seq.matches(Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier));
        QVERIFY(!seq.matches(Qt::Key_S, Qt::NoModifier));
        QVERIFY(!seq.matches(Qt::Key_A, Qt::ControlModifier));

        // Keypad Enter is normalized to Return
        const auto ret = QTuiKeySequence::fromString(QStringLiteral("Ctrl+Enter"));
        QVERIFY(ret.matches(Qt::Key_Return, Qt::ControlModifier));
        QVERIFY(ret.matches(Qt::Key_Enter, Qt::ControlModifier));
    }

    void parserBareTextKeyClassification()
    {
        QVERIFY(QTuiKeySequence::fromString(QStringLiteral("Q")).isBareTextKey());
        QVERIFY(QTuiKeySequence::fromString(QStringLiteral("Shift+Q")).isBareTextKey());
        QVERIFY(QTuiKeySequence::fromString(QStringLiteral("Space")).isBareTextKey());
        QVERIFY(!QTuiKeySequence::fromString(QStringLiteral("Ctrl+Q")).isBareTextKey());
        QVERIFY(!QTuiKeySequence::fromString(QStringLiteral("F5")).isBareTextKey());
        QVERIFY(!QTuiKeySequence::fromString(QStringLiteral("Escape")).isBareTextKey());
    }

    // -----------------------------------------------------------------------
    // QTuiAction registry / cached sequence
    // -----------------------------------------------------------------------
    void actionRegistryTracksLifetime()
    {
        const int before = QTuiAction::registeredActions().size();
        {
            QTuiAction a(QStringLiteral("Save"), QStringLiteral("Ctrl+S"));
            QCOMPARE(QTuiAction::registeredActions().size(), before + 1);
            QVERIFY(QTuiAction::registeredActions().contains(&a));
            QVERIFY(a.shortcutSequence().matches(Qt::Key_S, Qt::ControlModifier));
        }
        QCOMPARE(QTuiAction::registeredActions().size(), before);
    }

    void actionSetShortcutReparses()
    {
        QTuiAction a;
        QVERIFY(!a.shortcutSequence().isValid());
        a.setShortcut(QStringLiteral("Ctrl+Q"));
        QVERIFY(a.shortcutSequence().matches(Qt::Key_Q, Qt::ControlModifier));
        a.setShortcut(QStringLiteral("garbage"));
        QVERIFY(!a.shortcutSequence().isValid());
    }

    // -----------------------------------------------------------------------
    // Global dispatch through QTuiMainWindow
    // -----------------------------------------------------------------------
    void globalShortcutTriggersAction()
    {
        Env env;
        QTuiAction save(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), &env.window);
        QSignalSpy spy(&save, &QTuiAction::triggered);

        env.sendKey(Qt::Key_S, Qt::ControlModifier);
        QCOMPARE(spy.count(), 1);

        // Exact modifier matching: Ctrl+Shift+S must not fire Ctrl+S
        env.sendKey(Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(spy.count(), 1);

        // Bare S must not fire either
        env.sendKey(Qt::Key_S, Qt::NoModifier, QStringLiteral("s"));
        QCOMPARE(spy.count(), 1);
    }

    void shortcutFiresRegardlessOfFocusedWidget()
    {
        Env env;
        QTuiLineEdit edit(&env.window);
        edit.setGeometry(QRect(0, 0, 20, 1));
        edit.setFocus();
        QVERIFY(edit.hasFocus());

        QTuiAction save(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), &env.window);
        QSignalSpy spy(&save, &QTuiAction::triggered);

        // Modified shortcut fires even while an editor has focus
        env.sendKey(Qt::Key_S, Qt::ControlModifier);
        QCOMPARE(spy.count(), 1);
    }

    void disabledActionDoesNotFire()
    {
        Env env;
        QTuiAction save(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), &env.window);
        save.setEnabled(false);
        QSignalSpy spy(&save, &QTuiAction::triggered);

        env.sendKey(Qt::Key_S, Qt::ControlModifier);
        QCOMPARE(spy.count(), 0);
    }

    void bareLetterShortcutYieldsToTextEditing()
    {
        Env env;
        QTuiLineEdit edit(&env.window);
        edit.setGeometry(QRect(0, 0, 20, 1));
        QTuiPushButton button(QStringLiteral("Go"), &env.window);
        button.setGeometry(QRect(0, 2, 10, 3));

        QTuiAction quit(QStringLiteral("Quit"), QStringLiteral("Q"), &env.window);
        QSignalSpy spy(&quit, &QTuiAction::triggered);

        // While the line edit has focus, "q" is text, not a shortcut
        edit.setFocus();
        env.sendKey(Qt::Key_Q, Qt::NoModifier, QStringLiteral("q"));
        QCOMPARE(spy.count(), 0);
        QCOMPARE(edit.text(), QStringLiteral("q"));

        // While a button has focus, the bare-letter shortcut fires
        button.setFocus();
        env.sendKey(Qt::Key_Q, Qt::NoModifier, QStringLiteral("q"));
        QCOMPARE(spy.count(), 1);
    }

    void functionKeyShortcutFiresInEditor()
    {
        Env env;
        QTuiLineEdit edit(&env.window);
        edit.setGeometry(QRect(0, 0, 20, 1));
        edit.setFocus();

        QTuiAction refresh(QStringLiteral("Refresh"), QStringLiteral("F5"), &env.window);
        QSignalSpy spy(&refresh, &QTuiAction::triggered);

        env.sendKey(Qt::Key_F5, Qt::NoModifier);
        QCOMPARE(spy.count(), 1);
    }

    void altMnemonicOpensMenuWithoutBarFocus()
    {
        Env env;
        auto *menuBar = new QTuiMenuBar(&env.window);
        env.window.setMenuBar(menuBar);
        QTuiMenu *fileMenu = menuBar->addMenu(QStringLiteral("&File"));
        QTuiMenu *editMenu = menuBar->addMenu(QStringLiteral("&Edit"));

        // Focus an unrelated widget: the mnemonic must still work
        QTuiPushButton button(QStringLiteral("Go"), &env.window);
        button.setGeometry(QRect(0, 5, 10, 3));
        button.setFocus();
        QVERIFY(!menuBar->hasFocus());

        QVERIFY(!fileMenu->isVisible());
        env.sendKey(Qt::Key_F, Qt::AltModifier, QStringLiteral("f"));
        QVERIFY(fileMenu->isVisible());
        QVERIFY(!editMenu->isVisible());

        env.sendKey(Qt::Key_E, Qt::AltModifier, QStringLiteral("e"));
        QVERIFY(editMenu->isVisible());
    }

    void unmatchedKeyReachesFocusedWidget()
    {
        Env env;
        QTuiLineEdit edit(&env.window);
        edit.setGeometry(QRect(0, 0, 20, 1));
        edit.setFocus();

        QTuiAction save(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), &env.window);
        QSignalSpy spy(&save, &QTuiAction::triggered);

        env.sendKey(Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
        QCOMPARE(spy.count(), 0);
        QCOMPARE(edit.text(), QStringLiteral("a"));
    }

    void hiddenOwnerDisablesShortcut()
    {
        // An action on a hidden widget (for example a page of a tab that
        // is not current) must not take the key; a menu action works
        // while its menu is closed, if the menu bar is visible
        Env env;
        auto *bar = new QTuiMenuBar;
        env.window.setMenuBar(bar);
        QTuiAction *quit = bar->addMenu(QStringLiteral("File"))
                               ->addAction(QStringLiteral("Quit"), QStringLiteral("Ctrl+Q"));
        QSignalSpy quitSpy(quit, &QTuiAction::triggered);

        QTuiPushButton hiddenPage(QString(), &env.window);
        hiddenPage.setVisible(false);
        QTuiAction copy(QStringLiteral("Copy"), QStringLiteral("Ctrl+C"), &hiddenPage);
        QSignalSpy copySpy(&copy, &QTuiAction::triggered);

        env.sendKey(Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(copySpy.count(), 0);
        hiddenPage.setVisible(true);
        env.sendKey(Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(copySpy.count(), 1);

        env.sendKey(Qt::Key_Q, Qt::ControlModifier);
        QCOMPARE(quitSpy.count(), 1);
    }

    void ctrlZActionWinsOverSuspend()
    {
        // Ctrl+Z suspends only when no action uses it: an undo action
        // bound to Ctrl+Z must still fire
        Env env;
        QTuiAction undo(QStringLiteral("Undo"), QStringLiteral("Ctrl+Z"), &env.window);
        QSignalSpy spy(&undo, &QTuiAction::triggered);

        env.sendKey(Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_APPLESS_MAIN(TestShortcutDispatch)
#include "test_shortcut_dispatch.moc"
