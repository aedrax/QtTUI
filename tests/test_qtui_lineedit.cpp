#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

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
};

QTEST_MAIN(TestQTuiLineEdit)
#include "test_qtui_lineedit.moc"