// QTuiKeySequenceEdit: the keys go through the main window, as the keys of
// a terminal do, so the tests check that the record gets a key before the
// shortcut dispatch, the dialog code and the focus chain use it.
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiKeySequenceEdit.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

namespace {

// An application, a main window with a line edit and a key sequence edit,
// and a helper that sends a key to the window
struct Env {
    Env()
        : app(argc(), argv())
        , other(new QTuiLineEdit(&window))
        , edit(new QTuiKeySequenceEdit(&window))
    {
        window.setGeometry(QRect(0, 0, 80, 24));
        window.setVisible(true);
        app.setMainWindow(&window);
        other->setGeometry(QRect(0, 1, 30, 1));
        edit->setGeometry(QRect(0, 2, 30, 1));
        edit->setFocus();
    }

    void sendKey(int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                 const QString &text = QString())
    {
        TuiKeyEvent ev(key, mods, text);
        QCoreApplication::sendEvent(&window, &ev);
    }

    QString editText()
    {
        QCoreApplication::processEvents();
        QString text;
        for (int x = 0; x < 30; ++x)
            text += app.screenBuffer()->backCell(x, 2).text();
        return text.trimmed();
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
    QTuiLineEdit *other;
    QTuiKeySequenceEdit *edit;
};

} // namespace

class TestKeySequenceEdit : public QObject {
    Q_OBJECT

private slots:
    void toStringRoundTrip()
    {
        const QStringList texts = {
            QStringLiteral("Ctrl+S"), QStringLiteral("Ctrl+Alt+Shift+F12"), QStringLiteral("Q"),
            QStringLiteral("Meta+7"), QStringLiteral("Shift+Tab"), QStringLiteral("Enter"),
            QStringLiteral("Esc"), QStringLiteral("Alt+PgDown"), QStringLiteral("Del"),
            QStringLiteral("Space"), QStringLiteral("Ins"), QStringLiteral("Backspace"),
        };
        for (const QString &text : texts) {
            const QTuiKeySequence seq = QTuiKeySequence::fromString(text);
            QVERIFY2(seq.isValid(), qPrintable(text));
            QCOMPARE(seq.toString(), text);
            QCOMPARE(QTuiKeySequence::fromString(seq.toString()), seq);
        }
        QCOMPARE(QTuiKeySequence().toString(), QString());
        // Punctuation has no name
        QCOMPARE(QTuiKeySequence(Qt::Key_Plus, Qt::ControlModifier).toString(), QString());
    }

    void setAndClear()
    {
        Env env;
        QSignalSpy changed(env.edit, &QTuiKeySequenceEdit::keySequenceChanged);
        const QTuiKeySequence seq(Qt::Key_S, Qt::ControlModifier);
        env.edit->setKeySequence(seq);
        env.edit->setKeySequence(seq); // no change, no signal
        QCOMPARE(changed.count(), 1);
        QCOMPARE(env.edit->keySequence(), seq);
        QCOMPARE(env.editText(), QStringLiteral("Ctrl+S"));

        env.edit->clear();
        QCOMPARE(changed.count(), 2);
        QVERIFY(!env.edit->keySequence().isValid());
        QCOMPARE(env.editText(), QStringLiteral("Enter: record"));
    }

    // A shortcut of an action fires while the field only has focus, and
    // goes to the record after Enter
    void recordTakesShortcut()
    {
        Env env;
        QTuiAction action(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), &env.window);
        QSignalSpy triggered(&action, &QTuiAction::triggered);
        QSignalSpy changed(env.edit, &QTuiKeySequenceEdit::keySequenceChanged);
        QSignalSpy finished(env.edit, &QTuiKeySequenceEdit::editingFinished);

        env.sendKey(Qt::Key_S, Qt::ControlModifier);
        QCOMPARE(triggered.count(), 1);
        QVERIFY(!env.edit->keySequence().isValid());

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QVERIFY(env.edit->isRecording());
        QCOMPARE(env.editText(), QStringLiteral("Press a key"));

        env.sendKey(Qt::Key_S, Qt::ControlModifier);
        QCOMPARE(triggered.count(), 1);
        QVERIFY(!env.edit->isRecording());
        QCOMPARE(env.edit->keySequence(), QTuiKeySequence(Qt::Key_S, Qt::ControlModifier));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(finished.count(), 1);
        QCOMPARE(env.editText(), QStringLiteral("Ctrl+S"));

        // The record is over: the shortcut works again
        env.sendKey(Qt::Key_S, Qt::ControlModifier);
        QCOMPARE(triggered.count(), 2);
    }

    void recordTakesNavigationKeys()
    {
        Env env;
        // F10, Enter and a letter with Shift, as InputParser sends them
        env.sendKey(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        env.sendKey(Qt::Key_F10);
        QCOMPARE(env.edit->keySequence(), QTuiKeySequence(Qt::Key_F10, Qt::NoModifier));

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QCOMPARE(env.edit->keySequence().toString(), QStringLiteral("Enter"));

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_A, Qt::ShiftModifier, QStringLiteral("A"));
        QCOMPARE(env.edit->keySequence().toString(), QStringLiteral("Shift+A"));

        // Punctuation has no name: the record waits for the next key
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_Plus, Qt::NoModifier, QStringLiteral("+"));
        QVERIFY(env.edit->isRecording());
        env.sendKey(Qt::Key_Up);
        QCOMPARE(env.edit->keySequence().toString(), QStringLiteral("Up"));
    }

    // The user is never trapped: Tab and Shift+Tab leave the field, also
    // during the record
    void tabLeavesTheField()
    {
        Env env;
        const QTuiKeySequence seq(Qt::Key_K, Qt::ControlModifier);
        env.edit->setKeySequence(seq);

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QVERIFY(env.edit->isRecording());
        env.sendKey(Qt::Key_Tab, Qt::NoModifier, QStringLiteral("\t"));
        QVERIFY(!env.edit->isRecording());
        QVERIFY(!env.edit->hasFocus());
        QVERIFY(env.other->hasFocus());
        QCOMPARE(env.edit->keySequence(), seq);

        env.edit->setFocus();
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_Backtab, Qt::ShiftModifier);
        QVERIFY(!env.edit->isRecording());
        QVERIFY(!env.edit->hasFocus());
        QCOMPARE(env.edit->keySequence(), seq);

        // The keys go to the new focus widget, not to the old record
        env.other->setFocus();
        env.sendKey(Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
        QCOMPARE(env.other->text(), QStringLiteral("x"));
        QCOMPARE(env.edit->keySequence(), seq);
    }

    void escapeCancelsAndBackspaceClears()
    {
        Env env;
        const QTuiKeySequence seq(Qt::Key_K, Qt::ControlModifier);
        env.edit->setKeySequence(seq);
        QSignalSpy finished(env.edit, &QTuiKeySequenceEdit::editingFinished);

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_Escape);
        QVERIFY(!env.edit->isRecording());
        QVERIFY(env.edit->hasFocus());
        QCOMPARE(env.edit->keySequence(), seq);
        QCOMPARE(finished.count(), 0);

        // Backspace clears during the record
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_Backspace);
        QVERIFY(!env.edit->isRecording());
        QVERIFY(!env.edit->keySequence().isValid());
        QCOMPARE(finished.count(), 1);

        // Delete clears without a record; Ctrl+Delete is a combination
        env.edit->setKeySequence(seq);
        env.sendKey(Qt::Key_Delete);
        QVERIFY(!env.edit->keySequence().isValid());
        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        env.sendKey(Qt::Key_Delete, Qt::ControlModifier);
        QCOMPARE(env.edit->keySequence().toString(), QStringLiteral("Ctrl+Del"));
    }

    // In a modal dialog, Escape during the record stops the record and
    // does not reject the dialog; the next Escape rejects it
    void escapeInDialog()
    {
        Env env;
        QTuiDialog dialog;
        auto *edit = new QTuiKeySequenceEdit(&dialog);
        auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        column->addWidget(edit);
        dialog.setLayout(column);
        dialog.open();
        QVERIFY(edit->hasFocus());

        env.sendKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
        QVERIFY(edit->isRecording());
        env.sendKey(Qt::Key_Escape);
        QVERIFY(!edit->isRecording());
        QVERIFY(dialog.isVisible());

        env.sendKey(Qt::Key_Escape);
        QVERIFY(!dialog.isVisible());
    }

    void clickStartsRecord()
    {
        Env env;
        env.other->setFocus();
        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(3, 2));
        QCoreApplication::sendEvent(&env.window, &press);
        QVERIFY(env.edit->hasFocus());
        QVERIFY(env.edit->isRecording());

        // A click on another widget stops the record
        TuiMouseEvent away(TuiMouseEvent::Press, Qt::LeftButton, QPoint(3, 1));
        QCoreApplication::sendEvent(&env.window, &away);
        QVERIFY(!env.edit->isRecording());
    }
};

QTEST_APPLESS_MAIN(TestKeySequenceEdit)
#include "test_qtui_keysequenceedit.moc"
