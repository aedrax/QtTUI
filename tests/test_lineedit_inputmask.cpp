// QTuiLineEdit input mask: the mask characters, the separators, the case
// conversion, the blank character, and text(), displayText() and
// hasAcceptableInput() with a mask. The masks of the QLineEdit documentation
// are the test data.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiClipboard.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiValidator.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiLineEdit &le, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&le, &ev);
}

// Types the characters one key at a time
static void type(QTuiLineEdit &le, const QString &text)
{
    for (const QChar ch : text)
        sendKey(le, ch.unicode(), Qt::NoModifier, QString(ch));
}

// Rejects each text that has a 7
class NoSevenValidator : public QTuiValidator {
public:
    State validate(QString &input, int &) const override
    {
        return input.contains(QLatin1Char('7')) ? Invalid : Acceptable;
    }
};

class TestLineEditInputMask : public QObject {
    Q_OBJECT
private slots:

    void defaultHasNoMask()
    {
        QTuiLineEdit le;
        QCOMPARE(le.inputMask(), QString());
        QVERIFY(le.hasAcceptableInput());
    }

    void ipAddressMask()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("000.000.000.000;_"));
        QCOMPARE(le.inputMask(), QStringLiteral("000.000.000.000;_"));
        QCOMPARE(le.displayText(), QStringLiteral("___.___.___.___"));
        QCOMPARE(le.text(), QStringLiteral("..."));
        QCOMPARE(le.maxLength(), 15);
        QCOMPARE(le.cursorPosition(), 0);
        QVERIFY(le.hasAcceptableInput()); // each position can stay empty

        // A typed "." goes to the position after the next "."
        type(le, QStringLiteral("192.168.1.1"));
        QCOMPARE(le.displayText(), QStringLiteral("192.168.1__.1__"));
        QCOMPARE(le.text(), QStringLiteral("192.168.1.1"));
        QVERIFY(le.hasAcceptableInput());

        // A letter has no position
        type(le, QStringLiteral("x"));
        QCOMPARE(le.text(), QStringLiteral("192.168.1.1"));
    }

    void macAddressMask()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("HH:HH:HH:HH:HH:HH;_"));
        type(le, QStringLiteral("0a1B"));
        QCOMPARE(le.displayText(), QStringLiteral("0a:1B:__:__:__:__"));
        QCOMPARE(le.cursorPosition(), 6); // after the separator
        QVERIFY(!le.hasAcceptableInput()); // H needs a character
        type(le, QStringLiteral("g"));
        QCOMPARE(le.displayText(), QStringLiteral("0a:1B:__:__:__:__"));
        type(le, QStringLiteral("2c3d4e5f"));
        QCOMPARE(le.text(), QStringLiteral("0a:1B:2c:3d:4e:5f"));
        QVERIFY(le.hasAcceptableInput());
        // The mask is full
        type(le, QStringLiteral("1"));
        QCOMPARE(le.text(), QStringLiteral("0a:1B:2c:3d:4e:5f"));
    }

    void isoDateMaskWithSpaceBlanks()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("0000-00-00"));
        QCOMPARE(le.displayText(), QStringLiteral("    -  -  "));
        QCOMPARE(le.text(), QStringLiteral("--"));
        type(le, QStringLiteral("20261110"));
        QCOMPARE(le.displayText(), QStringLiteral("2026-11-10"));
        QCOMPARE(le.text(), QStringLiteral("2026-11-10"));
    }

    void licenseMaskMakesUppercase()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral(">AAAAA-AAAAA-AAAAA-AAAAA-AAAAA;#"));
        type(le, QStringLiteral("abcde1f"));
        QCOMPARE(le.displayText(), QStringLiteral("ABCDE-F####-#####-#####-#####"));
        QCOMPARE(le.text(), QStringLiteral("ABCDE-F---"));
        QVERIFY(!le.hasAcceptableInput());
    }

    void caseConversionAndEscape()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral(">AA<AA!AA"));
        type(le, QStringLiteral("abCDeF"));
        QCOMPARE(le.text(), QStringLiteral("ABcdeF"));

        // An escaped mask character is a separator; ; after \ is not the end
        le.setInputMask(QStringLiteral("\\A99\\;9;*"));
        QCOMPARE(le.displayText(), QStringLiteral("A**;*"));
        QCOMPARE(le.cursorPosition(), 1);
        type(le, QStringLiteral("123"));
        QCOMPARE(le.text(), QStringLiteral("A12;3"));

        // The reserved characters are not in the mask
        QTuiLineEdit reserved;
        reserved.setInputMask(QStringLiteral("[9]{9}"));
        QCOMPARE(reserved.displayText(), QStringLiteral("  "));
    }

    void maskCharacters_data()
    {
        QTest::addColumn<QString>("mask");
        QTest::addColumn<QString>("typed");
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("acceptable");

        QTest::newRow("A letter") << "AA" << "1a_b" << "ab" << true;
        QTest::newRow("A needs a character") << "AA" << "a" << "a" << false;
        QTest::newRow("a can stay empty") << "Aa" << "a" << "a" << true;
        QTest::newRow("N letter or digit") << "NN" << "-a1" << "a1" << true;
        QTest::newRow("n can stay empty") << "nN" << "" << "" << false;
        QTest::newRow("X not the blank") << "XX" << " !a" << "!a" << true;
        QTest::newRow("x can stay empty") << "xx" << "?" << "?" << true;
        QTest::newRow("9 digit") << "99" << "a12" << "12" << true;
        QTest::newRow("9 needs a character") << "99" << "1" << "1" << false;
        QTest::newRow("D not zero") << "DD" << "0102" << "12" << true;
        QTest::newRow("d not zero") << "dd" << "05" << "5" << true;
        QTest::newRow("# sign") << "#99" << "-12" << "-12" << true;
        QTest::newRow("# plus") << "#9" << "+5" << "+5" << true;
        QTest::newRow("H hex") << "HHH" << "gF0a" << "F0a" << true;
        QTest::newRow("h can stay empty") << "hH" << "" << "" << false;
        QTest::newRow("B binary") << "BBBB" << "1021" << "101" << false;
        QTest::newRow("b can stay empty") << "bbbb" << "1021" << "101" << true;
    }

    void maskCharacters()
    {
        QFETCH(QString, mask);
        QFETCH(QString, typed);
        QFETCH(QString, text);
        QFETCH(bool, acceptable);

        QTuiLineEdit le;
        le.setInputMask(mask);
        type(le, typed);
        QCOMPARE(le.text(), text);
        QCOMPARE(le.hasAcceptableInput(), acceptable);
    }

    void backspaceAndDeleteMakeBlanks()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("99-99;_"));
        type(le, QStringLiteral("1234"));
        QCOMPARE(le.displayText(), QStringLiteral("12-34"));
        QCOMPARE(le.cursorPosition(), 5);

        sendKey(le, Qt::Key_Backspace);
        QCOMPARE(le.displayText(), QStringLiteral("12-3_"));
        QCOMPARE(le.cursorPosition(), 4);
        sendKey(le, Qt::Key_Backspace);
        QCOMPARE(le.cursorPosition(), 3);
        // Backspace goes over the separator; the separator stays
        sendKey(le, Qt::Key_Backspace);
        QCOMPARE(le.displayText(), QStringLiteral("1_-__"));
        QCOMPARE(le.cursorPosition(), 1);
        QCOMPARE(le.text(), QStringLiteral("1-"));

        // Delete makes a blank; the text after it does not move
        type(le, QStringLiteral("234"));
        sendKey(le, Qt::Key_Home);
        sendKey(le, Qt::Key_Delete);
        QCOMPARE(le.displayText(), QStringLiteral("_2-34"));
        QCOMPARE(le.cursorPosition(), 0);
    }

    void selectionAndPaste()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("99-99;_"));
        le.setText(QStringLiteral("1234"));
        QCOMPARE(le.displayText(), QStringLiteral("12-34"));

        // A typed character replaces the selection; the separator stays
        le.selectAll();
        type(le, QStringLiteral("9"));
        QCOMPARE(le.displayText(), QStringLiteral("9_-__"));
        QCOMPARE(le.cursorPosition(), 1);

        le.selectAll();
        QTuiApplication::clipboard()->setText(QStringLiteral("56-78"));
        le.paste();
        QCOMPARE(le.displayText(), QStringLiteral("56-78"));

        // Ctrl+U removes the characters before the cursor
        sendKey(le, Qt::Key_U, Qt::ControlModifier);
        QCOMPARE(le.displayText(), QStringLiteral("__-__"));
        QCOMPARE(le.cursorPosition(), 0);
    }

    void setTextUsesTheMask()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("000.000.000.000;_"));
        QSignalSpy spy(&le, &QTuiLineEdit::textChanged);
        le.setText(QStringLiteral("10.0.0.1"));
        QCOMPARE(le.displayText(), QStringLiteral("10_.0__.0__.1__"));
        QCOMPARE(le.text(), QStringLiteral("10.0.0.1"));
        // The signal has text(), without the blanks
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("10.0.0.1"));

        type(le, QStringLiteral("x")); // no position takes it
        QCOMPARE(spy.count(), 1);
    }

    void undoRestoresTheBlanks()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("99-99;_"));
        type(le, QStringLiteral("12"));
        QCOMPARE(le.displayText(), QStringLiteral("12-__"));
        le.undo();
        QCOMPARE(le.displayText(), QStringLiteral("__-__"));
        le.redo();
        QCOMPARE(le.displayText(), QStringLiteral("12-__"));
    }

    void emptyMaskRemovesTheMask()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("99-99;_"));
        le.setMaxLength(2); // the mask sets the length
        QCOMPARE(le.maxLength(), 5);
        type(le, QStringLiteral("123"));
        le.setInputMask(QString());
        QCOMPARE(le.text(), QStringLiteral("12-3"));
        QCOMPARE(le.displayText(), QStringLiteral("12-3"));
        QCOMPARE(le.maxLength(), 32767);
        type(le, QStringLiteral("abc"));
        QCOMPARE(le.text(), QStringLiteral("12-3abc"));

        // A new mask keeps the text that it takes
        le.setInputMask(QStringLiteral("99-99;_"));
        QCOMPARE(le.displayText(), QStringLiteral("12-3_"));
    }

    void focusPutsTheCursorOnTheFirstPosition()
    {
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("(999) 999"));
        QCOMPARE(le.cursorPosition(), 1);
        le.setText(QStringLiteral("123"));
        QCOMPARE(le.cursorPosition(), 9);
        le.setFocus();
        QCOMPARE(le.cursorPosition(), 1);
    }

    void validatorGetsTheTextWithoutBlanks()
    {
        NoSevenValidator validator;
        QTuiLineEdit le;
        le.setInputMask(QStringLiteral("99-99;_"));
        le.setValidator(&validator);
        type(le, QStringLiteral("1723"));
        QCOMPARE(le.displayText(), QStringLiteral("12-3_"));
        QVERIFY(!le.hasAcceptableInput()); // the mask is not full

        QSignalSpy returned(&le, &QTuiLineEdit::returnPressed);
        sendKey(le, Qt::Key_Return);
        QCOMPARE(returned.count(), 0);
        type(le, QStringLiteral("4"));
        QVERIFY(le.hasAcceptableInput());
        sendKey(le, Qt::Key_Return);
        QCOMPARE(returned.count(), 1);
    }

    void paintShowsTheTemplateAndMarksDirty()
    {
        QTuiLineEdit le;
        le.setGeometry(QRect(0, 0, 8, 1));
        le.clearDirtyRegions();
        le.setInputMask(QStringLiteral("99-99;_"));
        QVERIFY(le.isDirty());

        le.clearDirtyRegions();
        type(le, QStringLiteral("1"));
        QVERIFY(le.isDirty());
        ScreenBuffer buffer(8, 1);
        le.paintEvent(buffer);
        QString row;
        for (int x = 0; x < 8; ++x)
            row += buffer.backCell(x, 0).text();
        QCOMPARE(row, QStringLiteral("1_-__   "));
    }
};

QTEST_MAIN(TestLineEditInputMask)
#include "test_lineedit_inputmask.moc"
