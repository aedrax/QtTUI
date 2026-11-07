// QTuiSyntaxHighlighter: the formats in the paint, the block state from
// line to line, and the rule that the text edit asks only for the lines that
// it paints and keeps only the block states.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QtTUI/QTuiPlainTextEdit.h>
#include <QtTUI/QTuiSyntaxHighlighter.h>
#include <QtTUI/QTuiTextEdit.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static const Color kGreen(0, 200, 0);
static const Color kRed(200, 0, 0);

// Colors C block comments, which can continue on the next lines, and makes
// digits bold and underlined
class CommentHighlighter : public QTuiSyntaxHighlighter {
public:
    using QTuiSyntaxHighlighter::QTuiSyntaxHighlighter;

    int calls = 0;
    QStringList seen;
    Color commentColor = kGreen;

protected:
    void highlightBlock(const QString &text) override
    {
        ++calls;
        seen.append(text);

        Cell digit;
        digit.foreground = kRed;
        digit.bold = true;
        digit.underline = true;
        digit.background = kRed; // the text edit does not use the background
        for (int i = 0; i < text.length(); ++i) {
            if (text.at(i).isDigit())
                setFormat(i, 1, digit);
        }

        bool inComment = previousBlockState() == 1;
        int start = inComment ? 0 : text.indexOf(QLatin1String("/*"));
        while (start >= 0) {
            const int end = text.indexOf(QLatin1String("*/"), inComment ? start : start + 2);
            if (end < 0) {
                setFormat(start, text.length() - start, commentColor);
                setCurrentBlockState(1);
                return;
            }
            setFormat(start, end + 2 - start, commentColor);
            inComment = false;
            start = text.indexOf(QLatin1String("/*"), end + 2);
        }
        setCurrentBlockState(0);
    }
};

// Calls setFormat() with ranges that are not in the text
class WildHighlighter : public QTuiSyntaxHighlighter {
public:
    using QTuiSyntaxHighlighter::QTuiSyntaxHighlighter;

protected:
    void highlightBlock(const QString &) override
    {
        setFormat(-5, 7, kGreen);            // covers indices 0 and 1
        setFormat(4, INT_MAX, kRed);         // to the end of the line
        setFormat(100, 5, kGreen);
        setFormat(2, -3, kGreen);
        QCOMPARE(currentBlockState(), -1);
    }
};

static void sendKey(QTuiTextEdit &w, int key, const QString &text = QString())
{
    TuiKeyEvent ev(key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(&w, &ev);
}

static Color fgAt(QTuiTextEdit &te, int x, int y)
{
    ScreenBuffer buffer(te.geometry().width(), te.geometry().height());
    te.paintEvent(buffer);
    return buffer.backCell(x, y).foreground;
}

class TestSyntaxHighlighter : public QObject {
    Q_OBJECT
private slots:

    void formatsShowInThePaint()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 2));
        te.setPlainText(QStringLiteral("a /* b */ c 7"));
        auto *hl = new CommentHighlighter(&te);
        QCOMPARE(hl->textEdit(), &te);
        QCOMPARE(hl->parent(), &te);
        const ThemeProperties theme = te.themeProperties();

        ScreenBuffer buffer(20, 2);
        te.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).foreground, theme.foreground);
        for (int x = 2; x <= 8; ++x)
            QCOMPARE(buffer.backCell(x, 0).foreground, kGreen);
        QCOMPARE(buffer.backCell(9, 0).foreground, theme.foreground);
        QCOMPARE(buffer.backCell(4, 0).ch, U' ');
        QVERIFY(!buffer.backCell(4, 0).bold);

        // The format with a cell style: the text attributes, not the background
        QCOMPARE(buffer.backCell(12, 0).ch, U'7');
        QCOMPARE(buffer.backCell(12, 0).foreground, kRed);
        QVERIFY(buffer.backCell(12, 0).bold);
        QVERIFY(buffer.backCell(12, 0).underline);
        QCOMPARE(buffer.backCell(12, 0).background, theme.background);
        // The cells after the text have the theme colors
        QCOMPARE(buffer.backCell(15, 0).foreground, theme.foreground);
        QCOMPARE(buffer.backCell(0, 1).foreground, theme.foreground);
    }

    void cursorAndSelectionStay()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 20, 2));
        te.setFocus();
        te.setPlainText(QStringLiteral("/* all comment */"));
        ScreenBuffer plain(20, 2);
        te.selectAll();
        te.paintEvent(plain);

        new CommentHighlighter(&te);
        ScreenBuffer colored(20, 2);
        te.paintEvent(colored);
        for (int x = 0; x < 20; ++x) {
            QCOMPARE(colored.backCell(x, 0).foreground, plain.backCell(x, 0).foreground);
            QCOMPARE(colored.backCell(x, 0).background, plain.backCell(x, 0).background);
            QCOMPARE(colored.backCell(x, 0).inverse, plain.backCell(x, 0).inverse);
        }
    }

    void asksOnlyForThePaintedLines()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 12, 3));
        QStringList lines;
        for (int i = 0; i < 100; ++i)
            lines.append(QStringLiteral("line %1").arg(i));
        te.setPlainText(lines.join(QLatin1Char('\n')));
        auto *hl = new CommentHighlighter(&te);
        QCOMPARE(hl->calls, 0); // nothing before a paint

        ScreenBuffer buffer(12, 3);
        te.paintEvent(buffer);
        QCOMPARE(hl->seen, QStringList({QStringLiteral("line 0"), QStringLiteral("line 1"),
                                        QStringLiteral("line 2")}));

        // The text edit keeps no formats: each paint asks again, but only
        // for the lines that show
        hl->seen.clear();
        te.paintEvent(buffer);
        QCOMPARE(hl->seen.size(), 3);

        // After a jump, the lines above the view give their block state one
        // time. The next paint asks for the painted lines only.
        QVERIFY(te.find(QStringLiteral("line 90")));
        hl->seen.clear();
        te.paintEvent(buffer);
        QCOMPARE(hl->seen.size(), 88 - 3 + 3); // lines 3 to 87 for the state, 88 to 90 to paint
        hl->seen.clear();
        te.paintEvent(buffer);
        QCOMPARE(hl->seen, QStringList({QStringLiteral("line 88"), QStringLiteral("line 89"),
                                        QStringLiteral("line 90")}));
    }

    void blockStateGoesToTheNextLines()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 4));
        te.setFocus();
        te.setPlainText(QStringLiteral("one\ntwo\nend */ x\nfour"));
        new CommentHighlighter(&te);
        const Color normal = te.themeProperties().foreground;
        QCOMPARE(fgAt(te, 0, 1), normal);

        // Open a comment on the first line: the lines below change
        sendKey(te, Qt::Key_Slash, QStringLiteral("/"));
        sendKey(te, Qt::Key_Asterisk, QStringLiteral("*"));
        QCOMPARE(te.toPlainText().left(5), QStringLiteral("/*one"));
        QCOMPARE(fgAt(te, 3, 0), kGreen);
        QCOMPARE(fgAt(te, 0, 1), kGreen);
        QCOMPARE(fgAt(te, 5, 2), kGreen);  // the "/" of "*/"
        QCOMPARE(fgAt(te, 7, 2), normal);  // "x" after the comment
        QCOMPARE(fgAt(te, 0, 3), normal);

        // Remove it: the lines change back
        sendKey(te, Qt::Key_Backspace);
        QCOMPARE(fgAt(te, 0, 1), normal);
        QCOMPARE(fgAt(te, 5, 2), normal);

        // Undo and redo use the correct states too
        te.undo();
        QCOMPARE(fgAt(te, 0, 1), kGreen);
        te.redo();
        QCOMPARE(fgAt(te, 0, 1), normal);
    }

    void stateFromAboveTheView()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 2));
        te.setFocus();
        QStringList lines{QStringLiteral("/* open")};
        for (int i = 1; i < 40; ++i)
            lines.append(QStringLiteral("text"));
        te.setPlainText(lines.join(QLatin1Char('\n')));
        auto *hl = new CommentHighlighter(&te);

        QVERIFY(te.find(QStringLiteral("text"), QTuiTextEdit::FindBackward) == false);
        te.selectAll(); // the cursor and the view go to the end
        sendKey(te, Qt::Key_Right);
        QCOMPARE(fgAt(te, 0, 1), kGreen);

        // An edit in the view does not ask for the lines above it again
        hl->seen.clear();
        sendKey(te, Qt::Key_X, QStringLiteral("x"));
        QCOMPARE(fgAt(te, 4, 1), kGreen);
        QCOMPARE(hl->seen.size(), 2);

        // An edit above the view does: close the comment on line 0
        for (int i = 0; i < 60; ++i)
            sendKey(te, Qt::Key_PageUp);
        sendKey(te, Qt::Key_End);
        sendKey(te, Qt::Key_Asterisk, QStringLiteral("*"));
        sendKey(te, Qt::Key_Slash, QStringLiteral("/"));
        for (int i = 0; i < 60; ++i)
            sendKey(te, Qt::Key_PageDown);
        QCOMPARE(fgAt(te, 0, 1), te.themeProperties().foreground);
    }

    void newTextAndAppendUseNewStates()
    {
        QTuiPlainTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 3));
        te.setPlainText(QStringLiteral("/* open\nb"));
        new CommentHighlighter(&te);
        const Color normal = te.themeProperties().foreground;
        QCOMPARE(fgAt(te, 0, 1), kGreen);

        te.setPlainText(QStringLiteral("a\nb"));
        QCOMPARE(fgAt(te, 0, 1), normal);

        // An appended line continues the state of the last line
        te.setPlainText(QStringLiteral("/* open"));
        QCOMPARE(fgAt(te, 0, 0), kGreen);
        te.appendPlainText(QStringLiteral("more"));
        QCOMPARE(fgAt(te, 0, 1), kGreen);

        // The line limit removes the line with the "/*": no comment is left
        te.setMaximumBlockCount(2);
        te.appendPlainText(QStringLiteral("last"));
        QCOMPARE(te.toPlainText(), QStringLiteral("more\nlast"));
        QCOMPARE(fgAt(te, 0, 0), normal);
        QCOMPARE(fgAt(te, 0, 1), normal);
    }

    void oneCallForAWrappedLine()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 6, 3));
        te.setLineWrapMode(QTuiTextEdit::WidgetWidth);
        te.setPlainText(QStringLiteral("ab /* cd ef */ g"));
        auto *hl = new CommentHighlighter(&te);

        // Rows: "ab /* ", "cd ef ", "*/ g"
        ScreenBuffer buffer(6, 3);
        te.paintEvent(buffer);
        QCOMPARE(hl->calls, 1);
        const Color normal = te.themeProperties().foreground;
        QCOMPARE(buffer.backCell(0, 0).foreground, normal);
        QCOMPARE(buffer.backCell(3, 0).foreground, kGreen);
        QCOMPARE(buffer.backCell(0, 1).foreground, kGreen);
        QCOMPARE(buffer.backCell(1, 2).foreground, kGreen);
        QCOMPARE(buffer.backCell(3, 2).foreground, normal);
    }

    void rehighlightRepaints()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 2));
        te.setPlainText(QStringLiteral("/* c */"));
        auto *hl = new CommentHighlighter(&te);
        QCOMPARE(fgAt(te, 0, 0), kGreen);

        te.clearDirtyRegions();
        hl->commentColor = kRed;
        hl->rehighlight();
        QVERIFY(te.isDirty());
        QCOMPARE(fgAt(te, 0, 0), kRed);
    }

    void deletedHighlighterGivesPlainText()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 2));
        te.setPlainText(QStringLiteral("/* c */"));
        auto *first = new CommentHighlighter(&te);
        QCOMPARE(fgAt(te, 0, 0), kGreen);

        // A new highlighter replaces the first one
        te.clearDirtyRegions();
        auto *second = new WildHighlighter(&te);
        QVERIFY(te.isDirty());
        first->calls = 0;
        QCOMPARE(fgAt(te, 0, 0), kGreen);
        QCOMPARE(first->calls, 0);
        first->rehighlight(); // no effect, and no crash
        delete first;

        te.clearDirtyRegions();
        delete second;
        QVERIFY(te.isDirty());
        QCOMPARE(fgAt(te, 0, 0), te.themeProperties().foreground);
    }

    void setFormatClampsTheRange()
    {
        QTuiTextEdit te;
        te.setGeometry(QRect(0, 0, 10, 2));
        te.setPlainText(QStringLiteral("abcdefgh"));
        new WildHighlighter(&te);
        const Color normal = te.themeProperties().foreground;

        ScreenBuffer buffer(10, 2);
        te.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).foreground, kGreen);
        QCOMPARE(buffer.backCell(1, 0).foreground, kGreen);
        QCOMPARE(buffer.backCell(2, 0).foreground, normal);
        QCOMPARE(buffer.backCell(3, 0).foreground, normal);
        QCOMPARE(buffer.backCell(4, 0).foreground, kRed);
        QCOMPARE(buffer.backCell(7, 0).foreground, kRed);
        QCOMPARE(buffer.backCell(8, 0).foreground, normal);
    }
};

QTEST_MAIN(TestSyntaxHighlighter)
#include "test_syntax_highlighter.moc"
