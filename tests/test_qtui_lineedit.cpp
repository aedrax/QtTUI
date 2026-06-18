#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiClipboard.h>

using namespace QtTUI;

// Helper: send a key event to a line edit
static void sendKey(QTuiLineEdit &le, int key, const QString &text = QString())
{
    TuiKeyEvent ev(key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(&le, &ev);
}

// Helper: generate a random printable ASCII char
static rc::Gen<QChar> genPrintableChar()
{
    return rc::gen::map(rc::gen::inRange(32, 127), [](int c) {
        return QChar(c);
    });
}

// Helper: generate a random printable ASCII string
static rc::Gen<QString> genPrintableString()
{
    return rc::gen::map(
        rc::gen::container<std::vector<int>>(rc::gen::inRange(32, 127)),
        [](const std::vector<int> &chars) {
            QString s;
            s.reserve(static_cast<int>(chars.size()));
            for (int c : chars)
                s.append(QChar(c));
            return s;
        });
}

class TestQTuiLineEdit : public QObject {
    Q_OBJECT
private slots:

    // Feature: qttui-framework, Property 26: LineEdit text editing operations
    void textEditingOperations()
    {
        rc::check("Insert at P places char at P and moves cursor to P+1; "
                   "Backspace at P>0 removes P-1; Delete at P<L removes P",
            []() {
                QTuiLineEdit le;
                le.setGeometry(QRect(0, 0, 80, 1));

                // Start with a random string and cursor position
                const auto initial = *genPrintableString();
                le.setText(initial);
                const int len = initial.length();
                const auto pos = *rc::gen::inRange(0, len + 1);
                le.setCursorPosition(pos);

                // Pick an operation
                const auto op = *rc::gen::inRange(0, 3);

                if (op == 0) {
                    // Insert
                    const auto ch = *genPrintableChar();
                    sendKey(le, ch.unicode(), QString(ch));
                    QString expected = initial;
                    expected.insert(pos, ch);
                    RC_ASSERT(le.text() == expected);
                    RC_ASSERT(le.cursorPosition() == pos + 1);
                } else if (op == 1) {
                    // Backspace
                    sendKey(le, Qt::Key_Backspace);
                    if (pos > 0) {
                        QString expected = initial;
                        expected.remove(pos - 1, 1);
                        RC_ASSERT(le.text() == expected);
                        RC_ASSERT(le.cursorPosition() == pos - 1);
                    } else {
                        RC_ASSERT(le.text() == initial);
                        RC_ASSERT(le.cursorPosition() == 0);
                    }
                } else {
                    // Delete
                    sendKey(le, Qt::Key_Delete);
                    if (pos < len) {
                        QString expected = initial;
                        expected.remove(pos, 1);
                        RC_ASSERT(le.text() == expected);
                        RC_ASSERT(le.cursorPosition() == pos);
                    } else {
                        RC_ASSERT(le.text() == initial);
                        RC_ASSERT(le.cursorPosition() == pos);
                    }
                }
            });
    }

    // Feature: qttui-framework, Property 27: LineEdit cursor movement
    void cursorMovement()
    {
        rc::check("Left->max(0,P-1), Right->min(L,P+1), Home->0, End->L",
            []() {
                QTuiLineEdit le;
                le.setGeometry(QRect(0, 0, 80, 1));

                const auto text = *genPrintableString();
                le.setText(text);
                const int len = text.length();
                const auto pos = *rc::gen::inRange(0, len + 1);
                le.setCursorPosition(pos);

                const auto dir = *rc::gen::inRange(0, 4);

                if (dir == 0) {
                    // Left
                    sendKey(le, Qt::Key_Left);
                    RC_ASSERT(le.cursorPosition() == qMax(0, pos - 1));
                } else if (dir == 1) {
                    // Right
                    sendKey(le, Qt::Key_Right);
                    RC_ASSERT(le.cursorPosition() == qMin(len, pos + 1));
                } else if (dir == 2) {
                    // Home
                    sendKey(le, Qt::Key_Home);
                    RC_ASSERT(le.cursorPosition() == 0);
                } else {
                    // End
                    sendKey(le, Qt::Key_End);
                    RC_ASSERT(le.cursorPosition() == len);
                }
            });
    }

    // Feature: qttui-framework, Property 28: LineEdit textChanged signal
    void textChangedSignal()
    {
        rc::check("Any text modification emits textChanged with new text equal to text()",
            []() {
                QTuiLineEdit le;
                le.setGeometry(QRect(0, 0, 80, 1));

                const auto initial = *genPrintableString();
                le.setText(initial);
                const int len = initial.length();
                const auto pos = *rc::gen::inRange(0, len + 1);
                le.setCursorPosition(pos);

                QSignalSpy spy(&le, &QTuiLineEdit::textChanged);

                // Pick a modifying operation
                const auto op = *rc::gen::inRange(0, 3);

                if (op == 0) {
                    // Insert
                    const auto ch = *genPrintableChar();
                    sendKey(le, ch.unicode(), QString(ch));
                    RC_ASSERT(spy.count() == 1);
                    RC_ASSERT(spy.last().at(0).toString() == le.text());
                } else if (op == 1) {
                    // Backspace (only modifies if pos > 0)
                    sendKey(le, Qt::Key_Backspace);
                    if (pos > 0) {
                        RC_ASSERT(spy.count() == 1);
                        RC_ASSERT(spy.last().at(0).toString() == le.text());
                    } else {
                        RC_ASSERT(spy.count() == 0);
                    }
                } else {
                    // Delete (only modifies if pos < len)
                    sendKey(le, Qt::Key_Delete);
                    if (pos < len) {
                        RC_ASSERT(spy.count() == 1);
                        RC_ASSERT(spy.last().at(0).toString() == le.text());
                    } else {
                        RC_ASSERT(spy.count() == 0);
                    }
                }
            });
    }

    // Feature: qttui-framework, Property 29: LineEdit returnPressed signal
    void returnPressedSignal()
    {
        rc::check("Enter emits returnPressed and text remains unchanged",
            []() {
                QTuiLineEdit le;
                le.setGeometry(QRect(0, 0, 80, 1));

                const auto text = *genPrintableString();
                le.setText(text);

                QSignalSpy spy(&le, &QTuiLineEdit::returnPressed);

                sendKey(le, Qt::Key_Return);

                RC_ASSERT(spy.count() == 1);
                RC_ASSERT(le.text() == text);
            });
    }

    // Feature: qttui-framework, Property 30: LineEdit text getter/setter round-trip
    void textRoundTrip()
    {
        rc::check("setText(S) then text() returns S",
            []() {
                QTuiLineEdit le;
                const auto s = *genPrintableString();
                le.setText(s);
                RC_ASSERT(le.text() == s);
            });
    }

    // Non-BMP characters (e.g. emoji) arrive from the parser as a full
    // surrogate pair in event->text(); the whole pair must be inserted.
    void emojiInsert_producesFullSurrogatePair()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));

        const QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80"); // U+1F600 GRINNING FACE
        QCOMPARE(emoji.length(), 2); // sanity: it is a surrogate pair in UTF-16

        sendKey(le, 0, emoji);
        QCOMPARE(le.text(), emoji);
        QCOMPARE(le.cursorPosition(), 2); // cursor advanced past both code units

        // A following ASCII char lands after the pair, not between the surrogates
        sendKey(le, 'a', QStringLiteral("a"));
        QCOMPARE(le.text(), emoji + QStringLiteral("a"));
        QCOMPARE(le.cursorPosition(), 3);
    }

    void emojiInsert_midText()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));
        le.setText(QStringLiteral("ab"));
        le.setCursorPosition(1);

        const QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80");
        sendKey(le, 0, emoji);
        QCOMPARE(le.text(), QStringLiteral("a") + emoji + QStringLiteral("b"));
        QCOMPARE(le.cursorPosition(), 3);
    }

    // ------------------------------------------------------------------
    // Unicode width: typing CJK moves the terminal cursor by COLUMNS (two
    // per character) while cursorPosition() stays in code units
    // ------------------------------------------------------------------
    void cjkTyping_cursorInColumns()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));
        le.setFocus();

        const QChar ji(0x65E5), hon(0x672C), go(0x8A9E); // 日 本 語
        sendKey(le, 0, QString(ji));
        sendKey(le, 0, QString(hon));
        sendKey(le, 0, QString(go));

        QCOMPARE(le.text(), QString() + ji + hon + go);
        QCOMPARE(le.cursorPosition(), 3);                  // 3 code units
        QCOMPARE(le.terminalCursorPos(), QPoint(6, 0));    // 6 columns
    }

    void cjkPaint_wideCellsWithContinuations()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 10, 1));
        le.setText(QString::fromUtf8("日本"));

        ScreenBuffer buf(10, 1);
        le.paintEvent(buf);

        QCOMPARE(buf.backCell(0, 0).ch, char32_t(0x65E5));
        QCOMPARE(int(buf.backCell(0, 0).width), 2);
        QVERIFY(buf.backCell(1, 0).isContinuation());
        QCOMPARE(buf.backCell(2, 0).ch, char32_t(0x672C));
        QVERIFY(buf.backCell(3, 0).isContinuation());
        QCOMPARE(buf.backCell(4, 0).ch, U' ');
    }

    void backspaceDeletesWholeEmoji()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));

        const QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80"); // U+1F600
        le.setText(QStringLiteral("a") + emoji + QStringLiteral("b"));
        sendKey(le, Qt::Key_End);

        sendKey(le, Qt::Key_Backspace); // removes 'b'
        QCOMPARE(le.text(), QStringLiteral("a") + emoji);
        QCOMPARE(le.cursorPosition(), 3);

        sendKey(le, Qt::Key_Backspace); // removes the WHOLE surrogate pair
        QCOMPARE(le.text(), QStringLiteral("a"));
        QCOMPARE(le.cursorPosition(), 1);
    }

    void deleteRemovesWholeEmoji()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));

        const QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80");
        le.setText(emoji + QStringLiteral("b"));
        sendKey(le, Qt::Key_Home);

        sendKey(le, Qt::Key_Delete); // removes the WHOLE surrogate pair
        QCOMPARE(le.text(), QStringLiteral("b"));
        QCOMPARE(le.cursorPosition(), 0);
    }

    void leftRightMoveByWholeEmoji()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));

        const QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80");
        le.setText(QStringLiteral("a") + emoji + QStringLiteral("b"));
        sendKey(le, Qt::Key_End);
        QCOMPARE(le.cursorPosition(), 4);

        sendKey(le, Qt::Key_Left);
        QCOMPARE(le.cursorPosition(), 3); // before 'b'
        sendKey(le, Qt::Key_Left);
        QCOMPARE(le.cursorPosition(), 1); // skipped the whole pair
        sendKey(le, Qt::Key_Right);
        QCOMPARE(le.cursorPosition(), 3); // skipped it forward too
    }

    void backspaceDeletesCombiningSequence()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 80, 1));

        // "aé" : 'a' then e-with-combining-acute (one cluster)
        le.setText(QString::fromUtf8("ae\xCC\x81"));
        sendKey(le, Qt::Key_End);
        QCOMPARE(le.cursorPosition(), 3);

        sendKey(le, Qt::Key_Backspace); // removes base + mark together
        QCOMPARE(le.text(), QStringLiteral("a"));
        QCOMPARE(le.cursorPosition(), 1);
    }

    void pasteInsertsOneLineWithoutReturnPressed()
    {
        QTuiLineEdit le;
        le.setText(QStringLiteral("ab"));
        QSignalSpy returnSpy(&le, &QTuiLineEdit::returnPressed);

        TuiPasteEvent paste(QStringLiteral("x\ny\tz\n"));
        QCoreApplication::sendEvent(&le, &paste);

        QCOMPARE(le.text(), QStringLiteral("abx y z "));
        QCOMPARE(le.cursorPosition(), 8);
        QCOMPARE(returnSpy.count(), 0);
    }

    void ctrlVPastesClipboard()
    {
        QTuiApplication::clipboard()->setText(QStringLiteral("two\nlines"));
        QTuiLineEdit le;
        TuiKeyEvent ev(Qt::Key_V, Qt::ControlModifier);
        QCoreApplication::sendEvent(&le, &ev);
        QCOMPARE(le.text(), QStringLiteral("two lines"));
    }

    void echoModePasswordAndNoEcho()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 10, 1));
        le.setEchoMode(QTuiLineEdit::Password);
        le.setText(QString::fromUtf8("ab\xF0\x9F\x98\x80")); // a, b, an emoji
        QCOMPARE(le.text(), QString::fromUtf8("ab\xF0\x9F\x98\x80"));
        // One mask character per character, also for the 2-cell emoji
        QCOMPARE(le.displayText(), QString(3, QChar(0x2022)));

        ScreenBuffer buf(10, 1);
        le.paintEvent(buf);
        QCOMPARE(buf.backCell(0, 0).ch, U'\u2022');
        QCOMPARE(buf.backCell(2, 0).ch, U'\u2022');
        QCOMPARE(buf.backCell(3, 0).ch, U' ');

        // Editing works on the real text
        TuiKeyEvent back(Qt::Key_Backspace, Qt::NoModifier);
        QCoreApplication::sendEvent(&le, &back);
        QCOMPARE(le.text(), QStringLiteral("ab"));

        le.setEchoMode(QTuiLineEdit::NoEcho);
        QVERIFY(le.displayText().isEmpty());
        ScreenBuffer none(10, 1);
        le.paintEvent(none);
        QCOMPARE(none.backCell(0, 0).ch, U' ');
    }

    void placeholderShowsOnlyWhileEmpty()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 12, 1));
        le.setPlaceholderText(QStringLiteral("Search"));
        ScreenBuffer empty(12, 1);
        le.paintEvent(empty);
        QCOMPARE(empty.backCell(0, 0).ch, U'S');
        QCOMPARE(empty.backCell(5, 0).ch, U'h');
        QVERIFY(le.text().isEmpty());

        le.setText(QStringLiteral("x"));
        ScreenBuffer typed(12, 1);
        le.paintEvent(typed);
        QCOMPARE(typed.backCell(0, 0).ch, U'x');
        QCOMPARE(typed.backCell(1, 0).ch, U' ');
    }
};

QTEST_MAIN(TestQTuiLineEdit)
#include "test_qtui_lineedit.moc"