#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>

using namespace QtTUI;

// Feature: widget-rendering-overhaul, Property 21: Ellipsis Truncation

class TestEllipsisTruncation : public QObject {
    Q_OBJECT
private slots:

    // **Validates: Requirements 9.1, 6.3, 7.3, 8.4**
    void ellipsisTruncation()
    {
        static constexpr char32_t ellipsis = 0x2026;

        rc::check("Ellipsis truncation produces correct output for all string/width combinations",
            [&]() {
                // Generate a random string (length 0..100) of printable ASCII
                const auto len = *rc::gen::inRange(0, 101);
                QString text;
                text.reserve(len);
                for (int i = 0; i < len; ++i) {
                    text.append(QChar(*rc::gen::inRange(0x20, 0x7F)));
                }

                // Generate a random maxWidth in range [-5, 150]
                const auto maxWidth = *rc::gen::inRange(-5, 151);

                // Create a style cell with distinctive attributes
                Cell style;
                style.foreground = Color(100, 150, 200);
                style.background = Color(10, 20, 30);
                style.bold = true;
                style.italic = false;

                // Use a buffer large enough to hold any output
                const int bufWidth = 200;
                const int bufHeight = 5;
                ScreenBuffer buffer(bufWidth, bufHeight);

                const int startX = 0;
                const int startY = 0;

                renderTextTruncated(buffer, startX, startY, text, maxWidth, style);

                if (maxWidth <= 0) {
                    // Nothing should be written — all cells at row 0 should be default
                    Cell defaultCell;
                    for (int x = 0; x < bufWidth; ++x) {
                        RC_ASSERT(buffer.backCell(x, startY) == defaultCell);
                    }
                } else if (len <= maxWidth) {
                    // Full string written without truncation
                    for (int i = 0; i < len; ++i) {
                        const Cell &c = buffer.backCell(startX + i, startY);
                        RC_ASSERT(c.ch == text.at(i).unicode());
                        RC_ASSERT(c.foreground == style.foreground);
                        RC_ASSERT(c.background == style.background);
                        RC_ASSERT(c.bold == style.bold);
                        RC_ASSERT(c.italic == style.italic);
                    }
                    // Cells beyond the text should be untouched (default)
                    if (len < bufWidth) {
                        Cell defaultCell;
                        RC_ASSERT(buffer.backCell(startX + len, startY) == defaultCell);
                    }
                } else if (maxWidth == 1) {
                    // Only ellipsis written
                    const Cell &c = buffer.backCell(startX, startY);
                    RC_ASSERT(c.ch == ellipsis);
                    RC_ASSERT(c.foreground == style.foreground);
                    RC_ASSERT(c.background == style.background);
                    RC_ASSERT(c.bold == style.bold);
                    // Cell at position 1 should be untouched
                    Cell defaultCell;
                    RC_ASSERT(buffer.backCell(startX + 1, startY) == defaultCell);
                } else {
                    // len > maxWidth and maxWidth >= 2
                    // Output is exactly maxWidth characters
                    // First maxWidth-1 chars match prefix of text
                    for (int i = 0; i < maxWidth - 1; ++i) {
                        const Cell &c = buffer.backCell(startX + i, startY);
                        RC_ASSERT(c.ch == text.at(i).unicode());
                        RC_ASSERT(c.foreground == style.foreground);
                        RC_ASSERT(c.background == style.background);
                        RC_ASSERT(c.bold == style.bold);
                    }
                    // Last character is ellipsis
                    const Cell &last = buffer.backCell(startX + maxWidth - 1, startY);
                    RC_ASSERT(last.ch == ellipsis);
                    RC_ASSERT(last.foreground == style.foreground);
                    RC_ASSERT(last.background == style.background);
                    RC_ASSERT(last.bold == style.bold);
                    // Cell beyond maxWidth should be untouched
                    if (maxWidth < bufWidth) {
                        Cell defaultCell;
                        RC_ASSERT(buffer.backCell(startX + maxWidth, startY) == defaultCell);
                    }
                }
            });
    }

    // ------------------------------------------------------------------
    // Unicode: truncation operates on display COLUMNS, wide glyphs write a
    // glyph + continuation pair, combining marks attach to the base cell
    // ------------------------------------------------------------------
    void wideTextFitsWithoutTruncation()
    {
        ScreenBuffer buffer(20, 1);
        Cell style;
        const QString cjk = QString::fromUtf8("日本"); // 4 columns

        renderTextTruncated(buffer, 0, 0, cjk, 4, style);

        QCOMPARE(buffer.backCell(0, 0).ch, char32_t(0x65E5)); // 日
        QVERIFY(buffer.backCell(1, 0).isContinuation());
        QCOMPARE(buffer.backCell(2, 0).ch, char32_t(0x672C)); // 本
        QVERIFY(buffer.backCell(3, 0).isContinuation());
        QCOMPARE(buffer.backCell(4, 0), Cell()); // untouched beyond text
    }

    void wideTextTruncatedAtCleanBoundary()
    {
        ScreenBuffer buffer(20, 1);
        Cell style;
        const QString cjk = QString::fromUtf8("日本語"); // 6 columns

        // Width 5: 日(0-1) 本(2-3) then the ellipsis at column 4
        renderTextTruncated(buffer, 0, 0, cjk, 5, style);

        QCOMPARE(buffer.backCell(0, 0).ch, char32_t(0x65E5));
        QVERIFY(buffer.backCell(1, 0).isContinuation());
        QCOMPARE(buffer.backCell(2, 0).ch, char32_t(0x672C));
        QVERIFY(buffer.backCell(3, 0).isContinuation());
        QCOMPARE(buffer.backCell(4, 0).ch, char32_t(0x2026)); // …
        QCOMPARE(buffer.backCell(5, 0), Cell());
    }

    void wideCharStraddlingBoundaryBecomesSpace()
    {
        ScreenBuffer buffer(20, 1);
        Cell style;
        const QString cjk = QString::fromUtf8("日本語"); // 6 columns

        // Width 4: 日(0-1); 本 would straddle column 3 (the ellipsis column),
        // so column 2 becomes a space and the ellipsis lands at column 3
        renderTextTruncated(buffer, 0, 0, cjk, 4, style);

        QCOMPARE(buffer.backCell(0, 0).ch, char32_t(0x65E5));
        QVERIFY(buffer.backCell(1, 0).isContinuation());
        QCOMPARE(buffer.backCell(2, 0).ch, U' ');
        QCOMPARE(int(buffer.backCell(2, 0).width), 1);
        QCOMPARE(buffer.backCell(3, 0).ch, char32_t(0x2026));
        QCOMPARE(buffer.backCell(4, 0), Cell());
    }

    void combiningMarkAttachesToBaseCell()
    {
        ScreenBuffer buffer(20, 1);
        Cell style;
        const QString text = QString::fromUtf8("e\xCC\x81x"); // e + U+0301, then x (2 columns)

        renderTextTruncated(buffer, 0, 0, text, 10, style);

        QCOMPARE(buffer.backCell(0, 0).ch, U'e');
        QCOMPARE(buffer.backCell(0, 0).combining, QString(QChar(0x0301)));
        QCOMPARE(buffer.backCell(1, 0).ch, U'x'); // combining mark got no cell
        QCOMPARE(buffer.backCell(2, 0), Cell());
    }

    void emojiCountsTwoColumns()
    {
        ScreenBuffer buffer(20, 1);
        Cell style;
        const QString text = QString::fromUtf8("a\xF0\x9F\x98\x80" "b"); // a😀b: 4 columns

        // Width 3: a(0); 😀 straddles the ellipsis column -> space at 1, … at 2
        renderTextTruncated(buffer, 0, 0, text, 3, style);
        QCOMPARE(buffer.backCell(0, 0).ch, U'a');
        QCOMPARE(buffer.backCell(1, 0).ch, U' ');
        QCOMPARE(buffer.backCell(2, 0).ch, char32_t(0x2026));

        // Width 4: fits exactly
        ScreenBuffer buffer2(20, 1);
        renderTextTruncated(buffer2, 0, 0, text, 4, style);
        QCOMPARE(buffer2.backCell(0, 0).ch, U'a');
        QCOMPARE(buffer2.backCell(1, 0).ch, char32_t(0x1F600));
        QCOMPARE(int(buffer2.backCell(1, 0).width), 2);
        QVERIFY(buffer2.backCell(2, 0).isContinuation());
        QCOMPARE(buffer2.backCell(3, 0).ch, U'b');
    }
};

QTEST_MAIN(TestEllipsisTruncation)
#include "test_ellipsis_truncation.moc"
