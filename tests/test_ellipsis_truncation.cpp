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
        static const QChar ellipsis = QChar(0x2026);

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
                        RC_ASSERT(c.character == text.at(i));
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
                    RC_ASSERT(c.character == ellipsis);
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
                        RC_ASSERT(c.character == text.at(i));
                        RC_ASSERT(c.foreground == style.foreground);
                        RC_ASSERT(c.background == style.background);
                        RC_ASSERT(c.bold == style.bold);
                    }
                    // Last character is ellipsis
                    const Cell &last = buffer.backCell(startX + maxWidth - 1, startY);
                    RC_ASSERT(last.character == ellipsis);
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
};

QTEST_MAIN(TestEllipsisTruncation)
#include "test_ellipsis_truncation.moc"
