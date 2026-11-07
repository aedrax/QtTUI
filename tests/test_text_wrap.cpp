// Word wrap of the text edits: setLineWrapMode(WidgetWidth) shows a long
// line as more than one visual row. The cursor, the mouse and the scroll
// position go by visual row.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QtTUI/QTuiPlainTextEdit.h>
#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiTextEdit &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

static void sendMouse(QTuiTextEdit &w, TuiMouseEvent::MouseEventType type, int x, int y)
{
    TuiMouseEvent ev(type, Qt::LeftButton, QPoint(x, y));
    QCoreApplication::sendEvent(&w, &ev);
}

// The text of one row of the buffer, without the spaces at the end
static QString rowText(const ScreenBuffer &buffer, int y)
{
    QString text;
    for (int x = 0; x < buffer.width(); ++x) {
        if (!buffer.backCell(x, y).isContinuation())
            text += buffer.backCell(x, y).text();
    }
    while (text.endsWith(QLatin1Char(' ')))
        text.chop(1);
    return text;
}

static QStringList paintRows(QTuiTextEdit &te)
{
    ScreenBuffer buffer(te.geometry().width(), te.geometry().height());
    te.paintEvent(buffer);
    QStringList rows;
    for (int y = 0; y < buffer.height(); ++y)
        rows.append(rowText(buffer, y));
    return rows;
}

class TestTextWrap : public QObject {
    Q_OBJECT
private slots:

    void defaultIsNoWrap()
    {
        QTuiTextEdit te;
        QCOMPARE(te.lineWrapMode(), QTuiTextEdit::NoWrap);
        te.setGeometry(QRect(0, 0, 10, 3));
        te.setPlainText(QStringLiteral("hello world foo bar"));
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("hello worl"), QString(), QString()}));
    }

    void wrapsAfterASpace()
    {
        QTuiPlainTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 4));
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        QCOMPARE(te.lineWrapMode(), QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar\nnext"));
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("hello"), QStringLiteral("world foo"),
                                             QStringLiteral("bar"), QStringLiteral("next")}));
        // The text does not change
        QCOMPARE(te.toPlainText(), QStringLiteral("hello world foo bar\nnext"));
    }

    void longWordBreaksAtTheEdge()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 4, 3));
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("abcdefghij"));
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("abcd"), QStringLiteral("efgh"),
                                             QStringLiteral("ij")}));
    }

    void wideCharacterGoesToTheNextRow()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 3, 2));
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("ab世c"));
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("ab"), QStringLiteral("世c")}));

        // A widget of one column: each character gets a row, also a wide one
        te.setGeometry(QRect(0, 0, 1, 3));
        te.setPlainText(QStringLiteral("世ab"));
        sendKey(te, Qt::Key_Down);
        QCOMPARE(te.cursorColumn(), 1);
        sendKey(te, Qt::Key_Down);
        QCOMPARE(te.cursorColumn(), 2);
        paintRows(te);
    }

    void upAndDownMoveByVisualRow()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 4));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar\nnext"));

        // Rows: "hello " (0), "world foo " (6), "bar" (16), then line 1
        sendKey(te, Qt::Key_Right);
        sendKey(te, Qt::Key_Right);
        sendKey(te, Qt::Key_Down);
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 8);
        sendKey(te, Qt::Key_Down);
        QCOMPARE(te.cursorColumn(), 18);
        sendKey(te, Qt::Key_Down);
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 2);
        sendKey(te, Qt::Key_Up);
        sendKey(te, Qt::Key_Up);
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 8);

        // The column stays when the cursor goes through a short row
        sendKey(te, Qt::Key_End);
        QCOMPARE(te.cursorColumn(), 15);
        sendKey(te, Qt::Key_Down);
        QCOMPARE(te.cursorColumn(), 19);
        sendKey(te, Qt::Key_Up);
        QCOMPARE(te.cursorColumn(), 15);
    }

    void homeAndEndUseTheVisualRow()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 4));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar"));
        sendKey(te, Qt::Key_Down);
        sendKey(te, Qt::Key_Right);
        sendKey(te, Qt::Key_Home);
        QCOMPARE(te.cursorColumn(), 6);
        // The end of a row that continues is its last character
        sendKey(te, Qt::Key_End);
        QCOMPARE(te.cursorColumn(), 15);
        sendKey(te, Qt::Key_Down);
        sendKey(te, Qt::Key_End);
        QCOMPARE(te.cursorColumn(), 19);
        sendKey(te, Qt::Key_Home, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("bar"));
    }

    void mouseUsesTheVisualRow()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(5, 2, 10, 4));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar\nnext"));

        sendMouse(te, TuiMouseEvent::Press, 5 + 2, 2 + 1);
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 8);
        sendMouse(te, TuiMouseEvent::Move, 5 + 1, 2 + 2);
        QCOMPARE(te.cursorColumn(), 17);
        QCOMPARE(te.selectedText(), QStringLiteral("rld foo b"));

        // A press after the end of a row goes to the end of that row
        sendMouse(te, TuiMouseEvent::Press, 5 + 9, 2 + 0);
        QCOMPARE(te.cursorColumn(), 5);
        sendMouse(te, TuiMouseEvent::Press, 5 + 1, 2 + 3);
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 1);
    }

    void viewScrollsByVisualRow()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 2));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar\nnext"));
        QCOMPARE(te.terminalCursorPos(), QPoint(0, 0));

        sendKey(te, Qt::Key_Down);
        sendKey(te, Qt::Key_Down);
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("world foo"), QStringLiteral("bar")}));
        QCOMPARE(te.terminalCursorPos(), QPoint(0, 1));
        sendKey(te, Qt::Key_Down);
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("bar"), QStringLiteral("next")}));

        sendKey(te, Qt::Key_Up);
        sendKey(te, Qt::Key_Up);
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("world foo"), QStringLiteral("bar")}));
        QCOMPARE(te.terminalCursorPos(), QPoint(0, 0));

        // A page is the height of the widget in visual rows
        sendKey(te, Qt::Key_PageUp);
        QCOMPARE(te.cursorColumn(), 0);
        sendKey(te, Qt::Key_PageDown);
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 16);
    }

    void cursorPastTheEdgeShowsInTheLastColumn()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 5, 3));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world"));

        // The space after "hello" is past the edge of the first row
        sendKey(te, Qt::Key_End);
        QCOMPARE(te.cursorColumn(), 5);
        QCOMPARE(te.terminalCursorPos(), QPoint(4, 0));
        ScreenBuffer buffer(5, 3);
        te.paintEvent(buffer);
        QVERIFY(buffer.backCell(4, 0).inverse);
        QCOMPARE(buffer.backCell(4, 0).ch, U'o');
        QVERIFY(!buffer.backCell(0, 1).inverse);

        // The end of a line that fills the row
        sendKey(te, Qt::Key_Down);
        sendKey(te, Qt::Key_End);
        QCOMPARE(te.cursorColumn(), 11);
        QCOMPARE(te.terminalCursorPos(), QPoint(4, 1));
        te.paintEvent(buffer);
        QVERIFY(buffer.backCell(4, 1).inverse);
        QVERIFY(!buffer.backCell(4, 0).inverse);
    }

    void selectionShowsOnWrappedRows()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 3));
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("hello world foo bar"));
        sendKey(te, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("hello "));

        ScreenBuffer buffer(10, 3);
        te.paintEvent(buffer);
        const Cell plain = buffer.backCell(0, 2);
        // "hello" is selected; "world" and the cells after "hello " are not
        QVERIFY(buffer.backCell(0, 0).background != plain.background);
        QCOMPARE(buffer.backCell(8, 0).background, plain.background);
        QCOMPARE(buffer.backCell(0, 1).background, plain.background);
    }

    void modeChangeResetsTheHorizontalScroll()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 3));
        te.setFocus();
        te.setPlainText(QStringLiteral("hello world foo bar"));
        sendKey(te, Qt::Key_End);
        QVERIFY(paintRows(te).first() != QStringLiteral("hello worl"));

        te.clearDirtyRegions();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        QVERIFY(te.isDirty());
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("hello"), QStringLiteral("world foo"),
                                             QStringLiteral("bar")}));
        QCOMPARE(te.terminalCursorPos(), QPoint(3, 2)); // after "bar"

        te.clearDirtyRegions();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        QVERIFY(!te.isDirty());
        te.setLineWrapMode(QTuiTextEdit::NoWrap);
        QVERIFY(te.isDirty());
    }

    void typingRewrapsTheLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 6, 3));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        for (const QChar ch : QStringLiteral("ab cd ef"))
            sendKey(te, ch == QLatin1Char(' ') ? Qt::Key_Space : Qt::Key_A, Qt::NoModifier, QString(ch));
        QCOMPARE(te.toPlainText(), QStringLiteral("ab cd ef"));
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("ab cd"), QStringLiteral("ef"), QString()}));
        QCOMPARE(te.terminalCursorPos(), QPoint(2, 1));
    }

    void narrowerWidgetKeepsAValidView()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 4, 2));
        te.setFocus();
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("abcdefghijkl"));
        sendKey(te, Qt::Key_Down);
        sendKey(te, Qt::Key_Down);
        // The view starts at the second row of the line. A wider widget has
        // one row only; the paint must not read past the rows.
        te.setGeometry(QRect(0, 0, 20, 2));
        QCOMPARE(paintRows(te), QStringList({QStringLiteral("abcdefghijkl"), QString()}));
        QCOMPARE(te.terminalCursorPos(), QPoint(8, 0));
    }
};

QTEST_MAIN(TestTextWrap)
#include "test_text_wrap.moc"
