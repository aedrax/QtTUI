#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiClipboard.h>
#include <QtTUI/QTuiPlainTextEdit.h>

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

class TestQTuiTextEdit : public QObject {
    Q_OBJECT
private slots:

    void inheritsQTuiWidget()
    {
        QTuiTextEdit te;
        QVERIFY(qobject_cast<QTuiWidget *>(&te) != nullptr);
    }

    void defaultConstruction()
    {
        QTuiTextEdit te;
        QCOMPARE(te.toPlainText(), QString());
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 0);
        QCOMPARE(te.isReadOnly(), false);
        QCOMPARE(te.hasSelection(), false);
    }

    void setPlainText_basic()
    {
        QTuiTextEdit te;
        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        te.setPlainText("Hello\nWorld");
        QCOMPARE(te.toPlainText(), QStringLiteral("Hello\nWorld"));
        QCOMPARE(spy.count(), 1);
        // Cursor resets to 0,0
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 0);
    }

    void setPlainText_empty()
    {
        QTuiTextEdit te;
        te.setPlainText("Some text");
        te.setPlainText("");
        QCOMPARE(te.toPlainText(), QString());
    }

    void insertCharacter()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        sendKey(te, Qt::Key_A, Qt::NoModifier, "a");
        sendKey(te, Qt::Key_B, Qt::NoModifier, "b");
        sendKey(te, Qt::Key_C, Qt::NoModifier, "c");

        QCOMPARE(te.toPlainText(), QStringLiteral("abc"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 3);
        QCOMPARE(spy.count(), 3); // 3 chars inserted = 3 textChanged signals
    }

    void insertCharacter_midLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("ac");

        // Move cursor to position 1 (between 'a' and 'c')
        sendKey(te, Qt::Key_Right, Qt::NoModifier);

        sendKey(te, Qt::Key_B, Qt::NoModifier, "b");
        QCOMPARE(te.toPlainText(), QStringLiteral("abc"));
        QCOMPARE(te.cursorColumn(), 2);
    }

    void deleteBackward_midLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("abc");

        // Move to end
        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 3);

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);
        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("ab"));
        QCOMPARE(te.cursorColumn(), 2);
        QCOMPARE(spy.count(), 1);
    }

    void deleteBackward_atLineStart_mergesLines()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello\nWorld");

        // Move to start of second line
        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        sendKey(te, Qt::Key_Home, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 0);

        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("HelloWorld"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 5);
    }

    void deleteBackward_atDocStart_noop()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("abc");

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);
        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("abc"));
        QCOMPARE(spy.count(), 0);
    }

    void insertNewLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("HelloWorld");

        // Move cursor to position 5
        for (int i = 0; i < 5; ++i)
            sendKey(te, Qt::Key_Right, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 5);

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);
        sendKey(te, Qt::Key_Return, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("Hello\nWorld"));
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void cursorMovement_arrows()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Line1\nLine2\nLine3");

        // Right arrow
        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 1);

        // Down arrow
        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 1);

        // Left arrow
        sendKey(te, Qt::Key_Left, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 0);

        // Up arrow
        sendKey(te, Qt::Key_Up, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 0);
    }

    void cursorMovement_homeEnd()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello\nWorld");

        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 5);

        sendKey(te, Qt::Key_Home, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 0);
    }

    void cursorMovement_pageUpDown()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 3)); // 3 visible rows
        te.setFocus();
        te.setPlainText("L0\nL1\nL2\nL3\nL4\nL5\nL6\nL7\nL8\nL9");

        // PageDown should move by page size (3 rows)
        sendKey(te, Qt::Key_PageDown, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 3);

        // PageUp should move back
        sendKey(te, Qt::Key_PageUp, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 0);
    }

    void cursorMovement_clampToLineLength()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("LongLine\nHi");

        // Move to end of first line (col 8)
        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 8);

        // Move down to shorter line, column should clamp
        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 2); // "Hi" has length 2
    }

    void scrolling_cursorBelowViewport()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 3)); // 3 visible rows
        te.setFocus();
        te.setPlainText("L0\nL1\nL2\nL3\nL4");

        // Move down past visible area
        for (int i = 0; i < 4; ++i)
            sendKey(te, Qt::Key_Down, Qt::NoModifier);

        QCOMPARE(te.cursorRow(), 4);
        // After scrollToVisible, the cursor should be visible
        // We can't directly check m_scrollRow, but we can verify via paint
        // that the widget doesn't crash and cursor is at expected position
    }

    void textChanged_emittedOnEdit()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        sendKey(te, Qt::Key_X, Qt::NoModifier, "x");
        QCOMPARE(spy.count(), 1);

        sendKey(te, Qt::Key_Return, Qt::NoModifier);
        QCOMPARE(spy.count(), 2);

        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(spy.count(), 3);
    }

    void textChanged_notEmittedOnMovement()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        sendKey(te, Qt::Key_Left, Qt::NoModifier);
        sendKey(te, Qt::Key_Home, Qt::NoModifier);
        sendKey(te, Qt::Key_End, Qt::NoModifier);
        sendKey(te, Qt::Key_Up, Qt::NoModifier);
        sendKey(te, Qt::Key_Down, Qt::NoModifier);

        QCOMPARE(spy.count(), 0);
    }

    void readOnly_blocksInsertion()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Original");
        te.setReadOnly(true);

        QCOMPARE(te.isReadOnly(), true);

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        sendKey(te, Qt::Key_A, Qt::NoModifier, "a");
        QCOMPARE(te.toPlainText(), QStringLiteral("Original"));
        QCOMPARE(spy.count(), 0);
    }

    void readOnly_blocksBackspace()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Original");
        te.setReadOnly(true);

        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("Original"));
        QCOMPARE(spy.count(), 0);
    }

    void readOnly_blocksNewline()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Original");
        te.setReadOnly(true);

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);

        sendKey(te, Qt::Key_Return, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("Original"));
        QCOMPARE(spy.count(), 0);
    }

    void readOnly_allowsNavigation()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello\nWorld");
        te.setReadOnly(true);

        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 1);

        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 1);
    }

    void selection_shiftRight()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        QCOMPARE(te.hasSelection(), false);

        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);

        QCOMPARE(te.hasSelection(), true);
        QCOMPARE(te.selectedText(), QStringLiteral("Hel"));
    }

    void selection_shiftDown()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Line1\nLine2\nLine3");

        sendKey(te, Qt::Key_Down, Qt::ShiftModifier);

        QCOMPARE(te.hasSelection(), true);
        // Selection from (0,0) to (1,0) = "Line1\n"
        QCOMPARE(te.selectedText(), QStringLiteral("Line1\n"));
    }

    void selection_clearedOnNonShiftMove()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        // Create selection
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.hasSelection(), true);

        // Non-shift move clears selection
        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        QCOMPARE(te.hasSelection(), false);
    }

    void selection_clearedOnEdit()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.hasSelection(), true);

        // Typing clears selection
        sendKey(te, Qt::Key_X, Qt::NoModifier, "x");
        QCOMPARE(te.hasSelection(), false);
    }

    void paintEvent_doesNotCrash()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 5));
        te.setFocus();
        te.setPlainText("Line1\nLine2\nLine3\nLine4\nLine5\nLine6");

        ScreenBuffer buf(20, 5);
        te.paintEvent(buf);
        // Just verify no crash; check first cell has content
        QCOMPARE(buf.backCell(0, 0).ch, U'L');
    }

    void horizontalScroll_typingPastRightEdge()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 5, 3)); // 5 visible columns
        te.setFocus();

        const QString typed = QStringLiteral("abcdefgh");
        for (QChar ch : typed)
            sendKey(te, ch.unicode(), Qt::NoModifier, QString(ch));

        QCOMPARE(te.cursorColumn(), 8);
        // Cursor must remain visible: scrolled so cursor sits on the last column
        QCOMPARE(te.terminalCursorPos(), QPoint(4, 0));

        // The viewport shows the tail of the line ("efgh" + cursor cell)
        ScreenBuffer buf(5, 3);
        te.paintEvent(buf);
        QCOMPARE(buf.backCell(0, 0).ch, U'e');
        QCOMPARE(buf.backCell(3, 0).ch, U'h');
        QCOMPARE(buf.backCell(4, 0).ch, U' '); // cursor past end of text
        QVERIFY(buf.backCell(4, 0).inverse);                // rendered as the cursor
    }

    void horizontalScroll_homeScrollsBack()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 5, 3));
        te.setFocus();

        const QString typed = QStringLiteral("abcdefgh");
        for (QChar ch : typed)
            sendKey(te, ch.unicode(), Qt::NoModifier, QString(ch));

        sendKey(te, Qt::Key_Home, Qt::NoModifier);
        QCOMPARE(te.terminalCursorPos(), QPoint(0, 0));

        ScreenBuffer buf(5, 3);
        te.paintEvent(buf);
        QCOMPARE(buf.backCell(0, 0).ch, U'a');
        QCOMPARE(buf.backCell(4, 0).ch, U'e');
    }

    void deleteKey_removesCharRightOfCursor()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("abc");

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);
        sendKey(te, Qt::Key_Delete, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("bc"));
        QCOMPARE(te.cursorColumn(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void deleteKey_atLineEnd_mergesWithNextLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello\nWorld");

        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 5);

        sendKey(te, Qt::Key_Delete, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("HelloWorld"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 5);
    }

    void deleteKey_atDocEnd_noop()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("abc");

        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);
        sendKey(te, Qt::Key_Delete, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("abc"));
        QCOMPARE(spy.count(), 0);
    }

    void selection_backspaceDeletesSelection()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("Hel"));

        QSignalSpy spy(&te, &QTuiTextEdit::textChanged);
        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("lo"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 0);
        QCOMPARE(te.hasSelection(), false);
        QCOMPARE(spy.count(), 1);
    }

    void selection_deleteKeyDeletesSelection()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("He"));

        sendKey(te, Qt::Key_Delete, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("llo"));
        QCOMPARE(te.cursorColumn(), 0);
        QCOMPARE(te.hasSelection(), false);
    }

    void selection_typingReplacesSelection()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("Hel"));

        sendKey(te, Qt::Key_X, Qt::NoModifier, "x");
        QCOMPARE(te.toPlainText(), QStringLiteral("xlo"));
        QCOMPARE(te.cursorColumn(), 1);
        QCOMPARE(te.hasSelection(), false);
    }

    void selection_multiLineBackspaceDeletes()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("abc\ndef\nghi");

        // Cursor to (0,2), then select down+right => (0,2)..(1,3) = "c\ndef"
        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        sendKey(te, Qt::Key_Down, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("c\ndef"));

        sendKey(te, Qt::Key_Backspace, Qt::NoModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("ab\nghi"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 2);
        QCOMPARE(te.hasSelection(), false);
    }

    void selection_multiLineTypingReplaces()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("abc\ndef\nghi");

        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        sendKey(te, Qt::Key_Right, Qt::NoModifier);
        sendKey(te, Qt::Key_Down, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.selectedText(), QStringLiteral("c\ndef"));

        sendKey(te, Qt::Key_X, Qt::NoModifier, "X");
        QCOMPARE(te.toPlainText(), QStringLiteral("abX\nghi"));
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 3);
    }

    void stickyColumn_preservedAcrossShortLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("LongLine\nHi\nAnother");

        sendKey(te, Qt::Key_End, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 8);

        // Down onto the short line clamps the visible cursor...
        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 2);

        // ...but the desired column survives to the next line
        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 2);
        QCOMPARE(te.cursorColumn(), 7); // "Another" length, clamped from 8

        // And going back up restores the full column on the long line
        sendKey(te, Qt::Key_Up, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 2);
        sendKey(te, Qt::Key_Up, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 8);
    }

    void stickyColumn_resetByHorizontalMove()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("LongLine\nHi");

        sendKey(te, Qt::Key_End, Qt::NoModifier);
        sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 2);

        // Horizontal move resets the desired column
        sendKey(te, Qt::Key_Left, Qt::NoModifier);
        QCOMPARE(te.cursorColumn(), 1);
        sendKey(te, Qt::Key_Up, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 1);
    }

    void mousePress_placesCursor()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(2, 1, 20, 5)); // offset geometry
        te.setFocus();
        te.setPlainText("Hello\nWorld");

        sendMouse(te, TuiMouseEvent::Press, 5, 2); // widget-relative (3, 1)
        QCOMPARE(te.cursorRow(), 1);
        QCOMPARE(te.cursorColumn(), 3);
    }

    void mousePress_clampsToLineLength()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(2, 1, 20, 5));
        te.setFocus();
        te.setPlainText("Hello\nWorld");

        sendMouse(te, TuiMouseEvent::Press, 12, 1); // widget-relative col 10, row 0
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 5); // clamped to "Hello" length
    }

    void mousePress_accountsForScroll()
    {
        // Vertical scroll offset
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 3));
        te.setFocus();
        te.setPlainText("L0\nL1\nL2\nL3\nL4\nL5");
        for (int i = 0; i < 5; ++i)
            sendKey(te, Qt::Key_Down, Qt::NoModifier);
        QCOMPARE(te.cursorRow(), 5); // scrolled so rows 3..5 are visible

        sendMouse(te, TuiMouseEvent::Press, 0, 0); // top visible row = document row 3
        QCOMPARE(te.cursorRow(), 3);
        QCOMPARE(te.cursorColumn(), 0);

        // Horizontal scroll offset
        QTuiTextEdit te2;
        te2.setGeometry(QRect(0, 0, 5, 1));
        te2.setFocus();
        const QString typed = QStringLiteral("abcdefgh");
        for (QChar ch : typed)
            sendKey(te2, ch.unicode(), Qt::NoModifier, QString(ch));

        sendMouse(te2, TuiMouseEvent::Press, 1, 0); // screen col 1 = text col 5
        QCOMPARE(te2.cursorColumn(), 5);
    }

    void mousePress_ignoresNonPress()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        // Hover motion (no button) and a release do not move the cursor; a
        // Move with the left button down is a drag (see dragSelects)
        TuiMouseEvent hover(TuiMouseEvent::Move, Qt::NoButton, QPoint(3, 0));
        QCoreApplication::sendEvent(&te, &hover);
        sendMouse(te, TuiMouseEvent::Release, 3, 0);
        QCOMPARE(te.cursorRow(), 0);
        QCOMPARE(te.cursorColumn(), 0);
    }

    void mousePress_clearsSelection()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();
        te.setPlainText("Hello");

        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        sendKey(te, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(te.hasSelection(), true);

        sendMouse(te, TuiMouseEvent::Press, 0, 0);
        QCOMPARE(te.hasSelection(), false);
    }

    void typing_nonBmpCharacterKeepsSurrogatePair()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 40, 10));
        te.setFocus();

        const QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80"); // U+1F600, surrogate pair
        QCOMPARE(emoji.length(), 2);

        sendKey(te, 0, Qt::NoModifier, emoji);
        QCOMPARE(te.toPlainText(), emoji);
        QCOMPARE(te.cursorColumn(), 2); // advanced past both UTF-16 code units
    }

    // ===================================================================
    // Property-based tests (rapidcheck)
    // ===================================================================

    // For any random printable char inserted at any valid cursor position,
    // the resulting text contains that char at the correct position and
    // cursor advances by 1.
    void pbt_textInsertion()
    {
        rc::check("Inserting a printable char at any valid position places it correctly and advances cursor",
            []() {
                QTuiTextEdit te;
                te.setGeometry(QRect(0, 0, 80, 20));
                te.setFocus();

                // Generate a short initial text (1-3 lines of printable chars)
                const auto lineCount = *rc::gen::inRange(1, 4);
                QStringList lines;
                for (int i = 0; i < lineCount; ++i) {
                    const auto lineChars = *rc::gen::container<std::vector<int>>(
                        rc::gen::inRange(32, 127));
                    QString line;
                    for (int c : lineChars)
                        line.append(QChar(c));
                    lines.append(line);
                }
                const QString initial = lines.join(QLatin1Char('\n'));
                te.setPlainText(initial);

                // Pick a random valid row and column
                const auto row = *rc::gen::inRange(0, lineCount);
                const auto col = *rc::gen::inRange(0, static_cast<int>(lines.at(row).length()) + 1);

                // Navigate to position: down to row, then End+Home to reset, then right to col
                for (int r = 0; r < row; ++r) {
                    TuiKeyEvent ev(Qt::Key_Down, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                {
                    TuiKeyEvent ev(Qt::Key_Home, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                for (int c = 0; c < col; ++c) {
                    TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }

                RC_ASSERT(te.cursorRow() == row);
                RC_ASSERT(te.cursorColumn() == col);

                // Generate a printable char and insert it
                const auto ch = *rc::gen::map(rc::gen::inRange(32, 127), [](int c) {
                    return QChar(c);
                });

                TuiKeyEvent ev(ch.unicode(), Qt::NoModifier, QString(ch));
                QCoreApplication::sendEvent(&te, &ev);

                // Verify cursor advanced
                RC_ASSERT(te.cursorRow() == row);
                RC_ASSERT(te.cursorColumn() == col + 1);

                // Verify the char is at the correct position in the line
                QStringList resultLines = te.toPlainText().split(QLatin1Char('\n'));
                RC_ASSERT(resultLines.at(row).at(col) == ch);

                // Verify the line grew by exactly 1
                RC_ASSERT(resultLines.at(row).length() == lines.at(row).length() + 1);
            });
    }

    // Backspace at col > 0 removes the character before cursor;
    // at col 0 of row > 0 merges with previous line.
    void pbt_backspaceDeletion()
    {
        rc::check("Backspace removes char before cursor or merges lines at line start",
            []() {
                QTuiTextEdit te;
                te.setGeometry(QRect(0, 0, 80, 20));
                te.setFocus();

                // Generate 1-4 lines of printable chars
                const auto lineCount = *rc::gen::inRange(1, 5);
                QStringList lines;
                for (int i = 0; i < lineCount; ++i) {
                    const auto lineChars = *rc::gen::container<std::vector<int>>(
                        rc::gen::inRange(32, 127));
                    QString line;
                    for (int c : lineChars)
                        line.append(QChar(c));
                    lines.append(line);
                }
                const QString initial = lines.join(QLatin1Char('\n'));
                te.setPlainText(initial);

                // Pick a random valid position (not row=0, col=0 since that's a no-op)
                const auto row = *rc::gen::inRange(0, lineCount);
                const auto col = *rc::gen::inRange(0, static_cast<int>(lines.at(row).length()) + 1);
                RC_PRE(row > 0 || col > 0); // Skip the no-op case

                // Navigate to position
                for (int r = 0; r < row; ++r) {
                    TuiKeyEvent ev(Qt::Key_Down, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                {
                    TuiKeyEvent ev(Qt::Key_Home, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                for (int c = 0; c < col; ++c) {
                    TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }

                const QString beforeText = te.toPlainText();

                // Press backspace
                TuiKeyEvent ev(Qt::Key_Backspace, Qt::NoModifier);
                QCoreApplication::sendEvent(&te, &ev);

                if (col > 0) {
                    // Character before cursor removed, cursor moves back by 1
                    RC_ASSERT(te.cursorRow() == row);
                    RC_ASSERT(te.cursorColumn() == col - 1);
                    QStringList resultLines = te.toPlainText().split(QLatin1Char('\n'));
                    // Same number of lines
                    RC_ASSERT(resultLines.size() == lineCount);
                    // Line is shorter by 1
                    RC_ASSERT(resultLines.at(row).length() == lines.at(row).length() - 1);
                } else {
                    // col == 0, row > 0: merge with previous line
                    RC_ASSERT(te.cursorRow() == row - 1);
                    RC_ASSERT(te.cursorColumn() == static_cast<int>(lines.at(row - 1).length()));
                    QStringList resultLines = te.toPlainText().split(QLatin1Char('\n'));
                    // One fewer line
                    RC_ASSERT(resultLines.size() == lineCount - 1);
                    // Merged line = previous + current
                    RC_ASSERT(resultLines.at(row - 1) == lines.at(row - 1) + lines.at(row));
                }
            });
    }

    // Enter at any cursor position splits the current line correctly.
    void pbt_newlineInsertion()
    {
        rc::check("Enter splits the current line at cursor position",
            []() {
                QTuiTextEdit te;
                te.setGeometry(QRect(0, 0, 80, 20));
                te.setFocus();

                // Generate 1-3 lines
                const auto lineCount = *rc::gen::inRange(1, 4);
                QStringList lines;
                for (int i = 0; i < lineCount; ++i) {
                    const auto lineChars = *rc::gen::container<std::vector<int>>(
                        rc::gen::inRange(32, 127));
                    QString line;
                    for (int c : lineChars)
                        line.append(QChar(c));
                    lines.append(line);
                }
                te.setPlainText(lines.join(QLatin1Char('\n')));

                // Pick a random position
                const auto row = *rc::gen::inRange(0, lineCount);
                const auto col = *rc::gen::inRange(0, static_cast<int>(lines.at(row).length()) + 1);

                // Navigate to position
                for (int r = 0; r < row; ++r) {
                    TuiKeyEvent ev(Qt::Key_Down, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                {
                    TuiKeyEvent ev(Qt::Key_Home, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                for (int c = 0; c < col; ++c) {
                    TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }

                // Press Enter
                TuiKeyEvent ev(Qt::Key_Return, Qt::NoModifier);
                QCoreApplication::sendEvent(&te, &ev);

                // Cursor should be at start of new line
                RC_ASSERT(te.cursorRow() == row + 1);
                RC_ASSERT(te.cursorColumn() == 0);

                QStringList resultLines = te.toPlainText().split(QLatin1Char('\n'));
                // One more line
                RC_ASSERT(resultLines.size() == lineCount + 1);
                // The original line was split: left part stays, right part goes to new line
                RC_ASSERT(resultLines.at(row) == lines.at(row).left(col));
                RC_ASSERT(resultLines.at(row + 1) == lines.at(row).mid(col));
            });
    }

    // Arrow keys, Home, End move cursor within valid bounds.
    void pbt_cursorMovement()
    {
        rc::check("Cursor movement keys keep cursor within valid bounds",
            []() {
                QTuiTextEdit te;
                te.setGeometry(QRect(0, 0, 80, 20));
                te.setFocus();

                // Generate 1-5 lines
                const auto lineCount = *rc::gen::inRange(1, 6);
                QStringList lines;
                for (int i = 0; i < lineCount; ++i) {
                    const auto lineChars = *rc::gen::container<std::vector<int>>(
                        rc::gen::inRange(32, 127));
                    QString line;
                    for (int c : lineChars)
                        line.append(QChar(c));
                    lines.append(line);
                }
                te.setPlainText(lines.join(QLatin1Char('\n')));

                // Navigate to a random starting position
                const auto startRow = *rc::gen::inRange(0, lineCount);
                const auto startCol = *rc::gen::inRange(0, static_cast<int>(lines.at(startRow).length()) + 1);
                for (int r = 0; r < startRow; ++r) {
                    TuiKeyEvent ev(Qt::Key_Down, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                {
                    TuiKeyEvent ev(Qt::Key_Home, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                for (int c = 0; c < startCol; ++c) {
                    TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }

                // Pick a random movement key
                const auto dir = *rc::gen::inRange(0, 6);
                int key = Qt::Key_Left;
                if (dir == 0) key = Qt::Key_Left;
                else if (dir == 1) key = Qt::Key_Right;
                else if (dir == 2) key = Qt::Key_Up;
                else if (dir == 3) key = Qt::Key_Down;
                else if (dir == 4) key = Qt::Key_Home;
                else key = Qt::Key_End;

                TuiKeyEvent ev(key, Qt::NoModifier);
                QCoreApplication::sendEvent(&te, &ev);

                // Cursor must be within valid bounds
                const int resultRow = te.cursorRow();
                const int resultCol = te.cursorColumn();
                RC_ASSERT(resultRow >= 0);
                RC_ASSERT(resultRow < lineCount);
                RC_ASSERT(resultCol >= 0);
                RC_ASSERT(resultCol <= static_cast<int>(lines.at(resultRow).length()));

                // Text must be unchanged (movement doesn't modify text)
                RC_ASSERT(te.toPlainText() == lines.join(QLatin1Char('\n')));
            });
    }

    // In read-only mode, no editing operation changes the text content.
    void pbt_readOnlyMode()
    {
        rc::check("Read-only mode blocks all editing operations",
            []() {
                QTuiTextEdit te;
                te.setGeometry(QRect(0, 0, 80, 20));
                te.setFocus();

                // Generate initial text
                const auto lineCount = *rc::gen::inRange(1, 4);
                QStringList lines;
                for (int i = 0; i < lineCount; ++i) {
                    const auto lineChars = *rc::gen::container<std::vector<int>>(
                        rc::gen::inRange(32, 127));
                    QString line;
                    for (int c : lineChars)
                        line.append(QChar(c));
                    lines.append(line);
                }
                const QString initial = lines.join(QLatin1Char('\n'));
                te.setPlainText(initial);
                te.setReadOnly(true);

                // Navigate to a random position
                const auto row = *rc::gen::inRange(0, lineCount);
                for (int r = 0; r < row; ++r) {
                    TuiKeyEvent ev(Qt::Key_Down, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                const auto col = *rc::gen::inRange(0, static_cast<int>(lines.at(row).length()) + 1);
                {
                    TuiKeyEvent ev(Qt::Key_Home, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }
                for (int c = 0; c < col; ++c) {
                    TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }

                // Pick a random editing operation
                const auto op = *rc::gen::inRange(0, 3);
                if (op == 0) {
                    // Try inserting a char
                    const auto ch = *rc::gen::map(rc::gen::inRange(32, 127), [](int c) {
                        return QChar(c);
                    });
                    TuiKeyEvent ev(ch.unicode(), Qt::NoModifier, QString(ch));
                    QCoreApplication::sendEvent(&te, &ev);
                } else if (op == 1) {
                    // Try backspace
                    TuiKeyEvent ev(Qt::Key_Backspace, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                } else {
                    // Try enter
                    TuiKeyEvent ev(Qt::Key_Return, Qt::NoModifier);
                    QCoreApplication::sendEvent(&te, &ev);
                }

                // Text must be unchanged
                RC_ASSERT(te.toPlainText() == initial);
            });
    }

    // For any string, setPlainText then toPlainText returns the same string.
    void pbt_setPlainTextRoundTrip()
    {
        rc::check("setPlainText(s) then toPlainText() returns s",
            []() {
                QTuiTextEdit te;

                // Generate a string that may contain newlines
                const auto lineCount = *rc::gen::inRange(1, 6);
                QStringList lines;
                for (int i = 0; i < lineCount; ++i) {
                    const auto lineChars = *rc::gen::container<std::vector<int>>(
                        rc::gen::inRange(32, 127));
                    QString line;
                    for (int c : lineChars)
                        line.append(QChar(c));
                    lines.append(line);
                }
                const QString text = lines.join(QLatin1Char('\n'));

                te.setPlainText(text);
                RC_ASSERT(te.toPlainText() == text);
            });
    }

    void pasteInsertsLinesAtCursor()
    {
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("AB"));
        // Cursor after "A"
        TuiKeyEvent home(Qt::Key_Home, Qt::NoModifier);
        QCoreApplication::sendEvent(&te, &home);
        TuiKeyEvent right(Qt::Key_Right, Qt::NoModifier);
        QCoreApplication::sendEvent(&te, &right);

        TuiPasteEvent paste(QStringLiteral("1\n2\n3"));
        QCoreApplication::sendEvent(&te, &paste);
        QCOMPARE(te.toPlainText(), QStringLiteral("A1\n2\n3B"));
    }

    void pasteIgnoredWhenReadOnly()
    {
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("keep"));
        te.setReadOnly(true);
        TuiPasteEvent paste(QStringLiteral("x"));
        QCoreApplication::sendEvent(&te, &paste);
        QCOMPARE(te.toPlainText(), QStringLiteral("keep"));
    }

    void clipboardCopyCutPaste()
    {
        QTuiApplication::clipboard()->clear();
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("hello world"));
        auto send = [&te](int key, Qt::KeyboardModifiers mods) {
            TuiKeyEvent ev(key, mods);
            QCoreApplication::sendEvent(&te, &ev);
        };

        // Select "hello" with Shift+Right from the line start
        send(Qt::Key_Home, Qt::NoModifier);
        for (int i = 0; i < 5; ++i)
            send(Qt::Key_Right, Qt::ShiftModifier);

        send(Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QTuiApplication::clipboard()->text(), QStringLiteral("hello"));
        QCOMPARE(te.toPlainText(), QStringLiteral("hello world"));

        send(Qt::Key_X, Qt::ControlModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral(" world"));

        send(Qt::Key_End, Qt::NoModifier);
        send(Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral(" worldhello"));
    }

    void clipboardOsc52Sequence()
    {
        // "hi" in base64 is "aGk="
        QCOMPARE(QTuiClipboard::osc52Sequence(QStringLiteral("hi")),
                 QByteArray("\x1b]52;c;aGk=\a"));
    }

    void plainTextEditAppendAndLimit()
    {
        QTuiPlainTextEdit log;
        log.setReadOnly(true);
        log.setMaximumBlockCount(3);
        for (int i = 1; i <= 5; ++i)
            log.appendPlainText(QStringLiteral("line %1").arg(i));
        QCOMPARE(log.blockCount(), 3);
        QCOMPARE(log.toPlainText(), QStringLiteral("line 3\nline 4\nline 5"));

        log.setMaximumBlockCount(2);
        QCOMPARE(log.toPlainText(), QStringLiteral("line 4\nline 5"));
    }

    void plainTextEditAppendIsAnUndoStep()
    {
        QTuiPlainTextEdit edit;
        edit.setPlainText(QStringLiteral("a"));
        edit.appendPlainText(QStringLiteral("b"));
        QVERIFY(edit.isUndoAvailable());
        edit.undo();
        QCOMPARE(edit.toPlainText(), QStringLiteral("a"));

        QTuiPlainTextEdit log;
        log.setMaximumBlockCount(5);
        log.appendPlainText(QStringLiteral("b"));
        QVERIFY(!log.isUndoAvailable());
    }

    void plainTextEditFollowsTailOnlyAtTheEnd()
    {
        QTuiPlainTextEdit log;
        log.setGeometry(QRect(0, 0, 20, 3));
        for (int i = 1; i <= 10; ++i)
            log.appendPlainText(QStringLiteral("line %1").arg(i));
        // The cursor follows the last line, so the view shows the tail
        QCOMPARE(log.cursorRow(), 9);
        ScreenBuffer buf(20, 3);
        log.paintEvent(buf);
        QCOMPARE(buf.backCell(5, 2).ch, U'1'); // "line 10"

        // After the user moves up, new lines do not move the view
        TuiKeyEvent up(Qt::Key_Up, Qt::NoModifier);
        QCoreApplication::sendEvent(&log, &up);
        log.appendPlainText(QStringLiteral("line 11"));
        QCOMPARE(log.cursorRow(), 8);
    }

    void undoRedo()
    {
        QTuiTextEdit te;
        auto send = [&te](int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                          const QString &text = QString()) {
            TuiKeyEvent ev(key, mods, text);
            QCoreApplication::sendEvent(&te, &ev);
        };
        for (const QChar ch : QStringLiteral("xy"))
            send(ch.toUpper().unicode(), Qt::NoModifier, QString(ch));
        send(Qt::Key_Return);
        send(Qt::Key_Z, Qt::NoModifier, QStringLiteral("z"));
        QCOMPARE(te.toPlainText(), QStringLiteral("xy\nz"));

        send(Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("xy\n"));
        send(Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("xy"));
        send(Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(te.toPlainText(), QString());

        send(Qt::Key_Y, Qt::ControlModifier);
        send(Qt::Key_Y, Qt::ControlModifier);
        QCOMPARE(te.toPlainText(), QStringLiteral("xy\n"));
        QCOMPARE(te.cursorRow(), 1);
    }

    void ctrlASelectsAll()
    {
        QTuiTextEdit te;
        te.setPlainText(QStringLiteral("one\ntwo"));
        TuiKeyEvent ctrlA(Qt::Key_A, Qt::ControlModifier);
        QCoreApplication::sendEvent(&te, &ctrlA);
        QVERIFY(te.hasSelection());
        QCOMPARE(te.selectedText(), QStringLiteral("one\ntwo"));
        // Typing replaces everything
        TuiKeyEvent x(Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
        QCoreApplication::sendEvent(&te, &x);
        QCOMPARE(te.toPlainText(), QStringLiteral("x"));
    }

    void dragSelects()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 5));
        te.setPlainText(QStringLiteral("abc\ndef"));
        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(1, 0));
        QCoreApplication::sendEvent(&te, &press);
        TuiMouseEvent drag(TuiMouseEvent::Move, Qt::LeftButton, QPoint(2, 1));
        QCoreApplication::sendEvent(&te, &drag);
        QCOMPARE(te.selectedText(), QStringLiteral("bc\nde"));
    }
};

QTEST_MAIN(TestQTuiTextEdit)
#include "test_qtui_textedit.moc"
