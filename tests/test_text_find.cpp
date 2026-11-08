// QTuiTextEdit::find(): the search direction, the flags, the selection of
// the match and the scroll to it.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QtTUI/QTuiPlainTextEdit.h>
#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiTextEdit &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

class TestTextFind : public QObject {
    Q_OBJECT
private slots:

    void findsForwardAndSelects()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 5));
        te.setPlainText(QStringLiteral("one two\nthree two one\ntwo"));

        QVERIFY(te.find(QStringLiteral("two")));
        QCOMPARE(te.selectedText(), QStringLiteral("two"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 7); // the cursor is at the end of the match

        // The next call finds the next match, not the same one
        QVERIFY(te.find(QStringLiteral("two")));
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 9);
        QVERIFY(te.find(QStringLiteral("two")));
        QCOMPARE(te.cursorRow(), 2);
        QCOMPARE(te.cursorColumn(), 3);

        // No more matches: nothing changes, and the search does not wrap
        QVERIFY(!te.find(QStringLiteral("two")));
        QCOMPARE(te.cursorRow(), 2);
        QCOMPARE(te.selectedText(), QStringLiteral("two"));
    }

    void findsBackward()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 5));
        te.setPlainText(QStringLiteral("one two\nthree two one\ntwo"));
        QVERIFY(!te.find(QStringLiteral("two"), QTuiTextEdit::FindBackward)); // cursor at the start

        te.selectAll();
        sendKey(te, Qt::Key_Right); // cursor at the end, no selection
        QVERIFY(te.find(QStringLiteral("two"), QTuiTextEdit::FindBackward));
        QCOMPARE(te.cursorRow(), 2);
        QCOMPARE(te.cursorColumn(), 3);
        QVERIFY(te.find(QStringLiteral("two"), QTuiTextEdit::FindBackward));
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 9);
        QVERIFY(te.find(QStringLiteral("two"), QTuiTextEdit::FindBackward));
        QCOMPARE(te.cursorRow(), 0);
        QVERIFY(!te.find(QStringLiteral("two"), QTuiTextEdit::FindBackward));

        // Forward again from the first match
        QVERIFY(te.find(QStringLiteral("two")));
        QCOMPARE(te.cursorRow(), 1);
    }

    void caseSensitivity()
    {
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("alpha Alpha ALPHA"));
        QVERIFY(te.find(QStringLiteral("ALPHA")));
        QCOMPARE(te.selectedText(), QStringLiteral("alpha"));

        te.setPlainText(QStringLiteral("alpha Alpha ALPHA"));
        QVERIFY(te.find(QStringLiteral("ALPHA"), QTuiTextEdit::FindCaseSensitively));
        QCOMPARE(te.cursorColumn(), 17);
        QVERIFY(te.find(QStringLiteral("Alpha"),
                        QTuiTextEdit::FindCaseSensitively | QTuiTextEdit::FindBackward));
        QCOMPARE(te.cursorColumn(), 11);
    }

    void wholeWords()
    {
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("cat concat cat2 (cat) cat"));
        QVERIFY(te.find(QStringLiteral("cat"), QTuiTextEdit::FindWholeWords));
        QCOMPARE(te.cursorColumn(), 3);
        // "concat" and "cat2" are not the word
        QVERIFY(te.find(QStringLiteral("cat"), QTuiTextEdit::FindWholeWords));
        QCOMPARE(te.cursorColumn(), 20);
        QVERIFY(te.find(QStringLiteral("cat"), QTuiTextEdit::FindWholeWords));
        QCOMPARE(te.cursorColumn(), 25);
        QVERIFY(!te.find(QStringLiteral("cat"), QTuiTextEdit::FindWholeWords));

        QVERIFY(te.find(QStringLiteral("cat"),
                        QTuiTextEdit::FindWholeWords | QTuiTextEdit::FindBackward));
        QCOMPARE(te.cursorColumn(), 20);
        QVERIFY(te.find(QStringLiteral("cat"),
                        QTuiTextEdit::FindWholeWords | QTuiTextEdit::FindBackward));
        QCOMPARE(te.cursorColumn(), 3);
    }

    void emptyAndMissingText()
    {
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("one\ntwo"));
        QVERIFY(!te.find(QString()));
        QVERIFY(!te.find(QStringLiteral("zzz")));
        QVERIFY(!te.find(QStringLiteral("one\ntwo"))); // a match is in one line
        QVERIFY(!te.hasSelection());
        QCOMPARE(te.cursorColumn(), 0);
    }

    void scrollsToTheMatchAndRepaints()
    {
        QTuiPlainTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 3));
        te.setReadOnly(true); // find works in a read-only text edit
        QStringList lines;
        for (int i = 0; i < 30; ++i)
            lines.append(QStringLiteral("line %1").arg(i));
        lines[20] = QStringLiteral("a long line with the needle");
        te.setPlainText(lines.join(QLatin1Char('\n')));

        te.clearDirtyRegions();
        QVERIFY(te.find(QStringLiteral("needle")));
        QVERIFY(te.isDirty());

        ScreenBuffer buffer(10, 3);
        te.paintEvent(buffer);
        // The match is on the last row of the view, at the right edge
        QString row;
        for (int x = 0; x < 10; ++x)
            row += buffer.backCell(x, 2).text();
        QVERIFY2(row.contains(QStringLiteral("needle")), qPrintable(row));
        // The selected cells differ from a cell without selection
        QVERIFY(buffer.backCell(row.indexOf(QStringLiteral("needle")), 2).background
                != buffer.backCell(0, 0).background);
    }

    void keysAfterAMatch()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 3));
        te.setFocus();
        te.setPlainText(QStringLiteral("one two three"));
        QVERIFY(te.find(QStringLiteral("two")));

        // Shift+Right extends the match; a plain arrow removes the selection
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("two "));
        sendKey(te, Qt::Key_Left);
        QVERIFY(!te.hasSelection());

        // A typed character replaces the match
        QVERIFY(te.find(QStringLiteral("two"), QTuiTextEdit::FindBackward));
        TuiKeyEvent ev(Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
        QCoreApplication::sendEvent(&te, &ev);
        QCOMPARE(te.toPlainText(), QStringLiteral("one x three"));
    }

    void findInWrappedText()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 2));
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar baz qux"));
        QVERIFY(te.find(QStringLiteral("qux")));
        // The row of the match is in the view
        QVERIFY(te.terminalCursorPos() != QPoint(-1, -1));
    }
};

QTEST_MAIN(TestTextFind)
#include "test_text_find.moc"
