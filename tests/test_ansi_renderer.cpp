#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/ScreenBuffer.h>
#include <QSet>

using namespace QtTUI;

class TestAnsiRenderer : public QObject {
    Q_OBJECT
private slots:
    void renderEmptyDiff()
    {
        // When front == back, render should return empty
        ScreenBuffer buf(3, 3);
        buf.swapBuffers(); // front = back, no diff
        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);
        QVERIFY(result.isEmpty());
    }

    void renderSingleChangedCell()
    {
        ScreenBuffer buf(5, 5);
        buf.swapBuffers();

        // Modify one cell in back buffer
        buf.backCell(2, 1).setChar(U'A');
        buf.backCell(2, 1).foreground = Color(255, 0, 0);

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        // Should contain CUP for position (2,1) -> 1-based (3,2)
        QVERIFY(result.contains("\x1b[2;3H"));
        // Should contain the character 'A'
        QVERIFY(result.contains('A'));
        // Should end with reset
        QVERIFY(result.endsWith("\x1b[0m"));
    }

    void sgrEmittedOnlyOnAttributeChange()
    {
        ScreenBuffer buf(20, 2);
        buf.swapBuffers();

        // Ten adjacent cells with identical attributes, then two bold ones
        for (int x = 0; x < 10; ++x) {
            Cell &c = buf.backCell(x, 0);
            c.setChar(char32_t('a' + x));
            c.foreground = Color(200, 200, 200);
        }
        for (int x = 10; x < 12; ++x) {
            Cell &c = buf.backCell(x, 0);
            c.setChar(U'B');
            c.foreground = Color(200, 200, 200);
            c.bold = true;
        }

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        // SGR sequences start with "\x1b[0;" (the trailing frame reset is
        // "\x1b[0m" and does not match). One for the plain run, one for bold.
        QCOMPARE(result.count("\x1b[0;"), 2);
        // All twelve characters still present, in order
        QVERIFY(result.contains("abcdefghij"));
        QVERIFY(result.contains("BB"));
    }

    void renderAdjacentCellsBatching()
    {
        // Adjacent changed cells on same row should produce only one CUP
        ScreenBuffer buf(10, 3);
        buf.swapBuffers();

        buf.backCell(2, 1).setChar(U'X');
        buf.backCell(3, 1).setChar(U'Y');
        buf.backCell(4, 1).setChar(U'Z');

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        // Should have exactly one CUP for position (2,1)
        QByteArray cup = AnsiRenderer::cursorPosition(2, 1);
        int cupCount = 0;
        int pos = 0;
        while ((pos = result.indexOf(cup, pos)) != -1) {
            ++cupCount;
            pos += cup.size();
        }
        QCOMPARE(cupCount, 1);

        // All three characters should be present
        QVERIFY(result.contains('X'));
        QVERIFY(result.contains('Y'));
        QVERIFY(result.contains('Z'));
    }

    void renderNonAdjacentCellsSeparateCUP()
    {
        // Non-adjacent cells on same row need separate CUP commands
        ScreenBuffer buf(10, 3);
        buf.swapBuffers();

        buf.backCell(1, 0).setChar(U'A');
        buf.backCell(5, 0).setChar(U'B');

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        // Should have two CUP commands (one for each non-adjacent cell)
        QByteArray cup1 = AnsiRenderer::cursorPosition(1, 0);
        QByteArray cup2 = AnsiRenderer::cursorPosition(5, 0);
        QVERIFY(result.contains(cup1));
        QVERIFY(result.contains(cup2));
    }

    void renderMultipleRows()
    {
        ScreenBuffer buf(5, 5);
        buf.swapBuffers();

        buf.backCell(0, 0).setChar(U'1');
        buf.backCell(0, 2).setChar(U'2');
        buf.backCell(0, 4).setChar(U'3');

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        // Should have CUP for each row
        QVERIFY(result.contains(AnsiRenderer::cursorPosition(0, 0)));
        QVERIFY(result.contains(AnsiRenderer::cursorPosition(0, 2)));
        QVERIFY(result.contains(AnsiRenderer::cursorPosition(0, 4)));
    }

    void renderOnlyDiffPositions()
    {
        // Verify render only emits for changed cells, not unchanged ones
        ScreenBuffer buf(3, 3);
        buf.swapBuffers();

        // Change only (1,1)
        buf.backCell(1, 1).setChar(U'Q');

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        // Count CUP sequences (should be exactly 1)
        int cupCount = 0;
        for (int i = 0; i < result.size() - 2; ++i) {
            if (result[i] == '\x1b' && result[i + 1] == '[') {
                // Check if it ends with 'H' (CUP)
                int j = i + 2;
                while (j < result.size() && result[j] != 'H' && result[j] != 'm'
                       && result[j] != 'A' && result[j] != 'B'
                       && result[j] != 'C' && result[j] != 'D')
                    ++j;
                if (j < result.size() && result[j] == 'H')
                    ++cupCount;
            }
        }
        QCOMPARE(cupCount, 1);
    }

    // Feature: qttui-framework, Property 6: Differential rendering correctness
    void differentialRenderingCorrectness()
    {
        rc::check("AnsiRenderer emits sequences only for positions where front != back, matching diff() exactly",
            []() {
                // 1. Generate a random small buffer size
                const auto w = *rc::gen::inRange(2, 21);
                const auto h = *rc::gen::inRange(2, 21);

                ScreenBuffer buf(w, h);
                buf.swapBuffers(); // sync front and back

                // 2. Randomly modify some cells in the back buffer with unique characters
                const auto numChanges = *rc::gen::inRange(0, w * h + 1);
                // Generate unique positions to modify
                QSet<int> chosenIndices;
                for (int i = 0; i < numChanges; ++i) {
                    const auto idx = *rc::gen::inRange(0, w * h);
                    chosenIndices.insert(idx);
                }

                int charIdx = 0;
                for (int idx : chosenIndices) {
                    int x = idx % w;
                    int y = idx / w;
                    // Use unique characters so we can verify them in output
                    char ch = 'A' + (charIdx % 26);
                    buf.backCell(x, y).setChar(char32_t(ch));
                    // Also vary color to ensure SGR differs
                    buf.backCell(x, y).foreground = Color(
                        static_cast<uint8_t>((charIdx * 37) % 256),
                        static_cast<uint8_t>((charIdx * 53) % 256),
                        static_cast<uint8_t>((charIdx * 71) % 256));
                    ++charIdx;
                }

                // 3. Get the diff positions
                QVector<QPoint> diffPositions = buf.diff();
                QSet<QPoint> diffSet(diffPositions.begin(), diffPositions.end());

                // 4. Render
                AnsiRenderer renderer;
                QByteArray output = renderer.render(buf);

                // 5. If diff is empty, render must return empty
                if (diffSet.isEmpty()) {
                    RC_ASSERT(output.isEmpty());
                    return;
                }

                // 6. For each diff position, verify the output contains the character
                //    that was written to that back buffer cell
                for (const QPoint &pos : diffPositions) {
                    const Cell &cell = buf.backCell(pos.x(), pos.y());
                    char expectedChar = static_cast<char>(cell.ch);
                    RC_ASSERT(output.contains(expectedChar));
                }

                // 7. Count CUP sequences in the output
                int cupCount = 0;
                for (int i = 0; i < output.size() - 2; ++i) {
                    if (output[i] == '\x1b' && output[i + 1] == '[') {
                        int j = i + 2;
                        while (j < output.size()
                               && ((output[j] >= '0' && output[j] <= '9') || output[j] == ';'))
                            ++j;
                        if (j < output.size() && output[j] == 'H')
                            ++cupCount;
                    }
                }

                // CUP count must be <= diff size (batching reduces CUPs)
                RC_ASSERT(cupCount <= diffPositions.size());
                // Must have at least one CUP if there are changes
                RC_ASSERT(cupCount >= 1);

                // 8. Parse all CUP sequences and verify each addressed position is in diff
                QSet<QPoint> renderedPositions;
                int parseIdx = 0;
                while (parseIdx < output.size() - 2) {
                    if (output[parseIdx] == '\x1b' && output[parseIdx + 1] == '[') {
                        int seqStart = parseIdx;
                        int j = parseIdx + 2;
                        while (j < output.size()
                               && ((output[j] >= '0' && output[j] <= '9') || output[j] == ';'))
                            ++j;
                        if (j < output.size() && output[j] == 'H') {
                            // Found a CUP sequence
                            QByteArray cupSeq = output.mid(seqStart, j - seqStart + 1);
                            QPoint cupPos = AnsiRenderer::parseCupSequence(cupSeq);
                            // This position must be in the diff set
                            RC_ASSERT(diffSet.contains(cupPos));
                            renderedPositions.insert(cupPos);

                            // Now track consecutive cells after this CUP (batched)
                            // Skip past the CUP
                            parseIdx = j + 1;
                            int currentX = cupPos.x();
                            int currentY = cupPos.y();
                            bool firstCharAfterCup = true;

                            // Walk through SGR+char pairs until next CUP or end
                            while (parseIdx < output.size()) {
                                // Check if next is an ESC sequence
                                if (output[parseIdx] == '\x1b' && parseIdx + 1 < output.size()
                                    && output[parseIdx + 1] == '[') {
                                    // Determine if it's SGR (ends with 'm') or CUP (ends with 'H')
                                    int k = parseIdx + 2;
                                    while (k < output.size()
                                           && ((output[k] >= '0' && output[k] <= '9')
                                               || output[k] == ';'))
                                        ++k;
                                    if (k < output.size() && output[k] == 'm') {
                                        // SGR sequence skip it
                                        parseIdx = k + 1;
                                        // Next should be the character
                                        if (parseIdx < output.size()
                                            && output[parseIdx] != '\x1b') {
                                            if (firstCharAfterCup) {
                                                // First SGR+char belongs to the CUP position itself
                                                firstCharAfterCup = false;
                                            } else {
                                                // Subsequent chars are batched adjacent cells
                                                currentX++;
                                                QPoint batchedPos(currentX, currentY);
                                                renderedPositions.insert(batchedPos);
                                            }
                                            parseIdx++;
                                        }
                                    } else if (k < output.size() && output[k] == 'H') {
                                        // Next CUP break out to handle it in outer loop
                                        break;
                                    } else {
                                        // Some other sequence (like reset \e[0m at end)
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            }
                            continue;
                        }
                    }
                    parseIdx++;
                }

                // 9. The set of rendered positions must exactly match the diff set
                RC_ASSERT(renderedPositions == diffSet);
            });
    }

    // Feature: qttui-framework, Property 7: TrueColor SGR encoding
    void trueColorSgrEncoding()
    {
        rc::check("For any RGB value, SGR serialization produces valid TrueColor escape sequences",
            []() {
                // Generate random RGB values for foreground and background
                const auto fgR = *rc::gen::inRange(0, 256);
                const auto fgG = *rc::gen::inRange(0, 256);
                const auto fgB = *rc::gen::inRange(0, 256);
                const auto bgR = *rc::gen::inRange(0, 256);
                const auto bgG = *rc::gen::inRange(0, 256);
                const auto bgB = *rc::gen::inRange(0, 256);

                Cell cell;
                cell.foreground = Color(
                    static_cast<uint8_t>(fgR),
                    static_cast<uint8_t>(fgG),
                    static_cast<uint8_t>(fgB));
                cell.background = Color(
                    static_cast<uint8_t>(bgR),
                    static_cast<uint8_t>(bgG),
                    static_cast<uint8_t>(bgB));

                QByteArray sgr = AnsiRenderer::sgrSequence(cell);

                // Must start with ESC[ and end with m
                RC_ASSERT(sgr.startsWith("\x1b["));
                RC_ASSERT(sgr.endsWith("m"));

                // Build expected foreground and background substrings
                QByteArray expectedFg = "38;2;"
                    + QByteArray::number(fgR) + ";"
                    + QByteArray::number(fgG) + ";"
                    + QByteArray::number(fgB);

                QByteArray expectedBg = "48;2;"
                    + QByteArray::number(bgR) + ";"
                    + QByteArray::number(bgG) + ";"
                    + QByteArray::number(bgB);

                RC_ASSERT(sgr.contains(expectedFg));
                RC_ASSERT(sgr.contains(expectedBg));
            });
    }

    // Feature: qttui-framework, Property 9: Adjacent cell batching
    void adjacentCellBatching()
    {
        rc::check("N adjacent changed cells on same row produce at most one cursor positioning command",
            []() {
                // 1. Generate a random buffer size
                const auto w = *rc::gen::inRange(5, 51);
                const auto h = *rc::gen::inRange(2, 21);

                ScreenBuffer buf(w, h);
                buf.swapBuffers(); // sync front and back

                // 2. Pick a random row and starting x position
                const auto row = *rc::gen::inRange(0, h);
                const auto startX = *rc::gen::inRange(0, w);

                // 3. Generate N adjacent cells (N >= 1) starting from startX
                const auto maxN = w - startX;
                RC_PRE(maxN >= 1);
                const auto n = *rc::gen::inRange(1, maxN + 1);

                // 4. Modify each adjacent cell with a unique character
                for (int i = 0; i < n; ++i) {
                    char ch = 'A' + (i % 26);
                    buf.backCell(startX + i, row).setChar(char32_t(ch));
                }

                // 5. Render
                AnsiRenderer renderer;
                QByteArray output = renderer.render(buf);

                // 6. Count CUP sequences (\e[...H) in the output
                int cupCount = 0;
                for (int i = 0; i < output.size() - 2; ++i) {
                    if (output[i] == '\x1b' && output[i + 1] == '[') {
                        int j = i + 2;
                        while (j < output.size()
                               && ((output[j] >= '0' && output[j] <= '9') || output[j] == ';'))
                            ++j;
                        if (j < output.size() && output[j] == 'H')
                            ++cupCount;
                    }
                }

                // 7. Assert exactly 1 CUP for this contiguous run
                RC_ASSERT(cupCount == 1);
            });
    }

    // Feature: qttui-framework, Property 33: SGR attribute serialization round-trip
    void sgrAttributeRoundTrip()
    {
        rc::check("parseSgrSequence(sgrSequence(cell)) produces equivalent attributes",
            []() {
                // 1. Generate random colors
                const auto fgR = *rc::gen::inRange(0, 256);
                const auto fgG = *rc::gen::inRange(0, 256);
                const auto fgB = *rc::gen::inRange(0, 256);
                const auto bgR = *rc::gen::inRange(0, 256);
                const auto bgG = *rc::gen::inRange(0, 256);
                const auto bgB = *rc::gen::inRange(0, 256);

                // 2. Generate random boolean attributes
                const auto bold = *rc::gen::arbitrary<bool>();
                const auto italic = *rc::gen::arbitrary<bool>();
                const auto underline = *rc::gen::arbitrary<bool>();
                const auto strikethrough = *rc::gen::arbitrary<bool>();
                const auto inverse = *rc::gen::arbitrary<bool>();

                // 3. Build the Cell
                Cell cell;
                cell.foreground = Color(
                    static_cast<uint8_t>(fgR),
                    static_cast<uint8_t>(fgG),
                    static_cast<uint8_t>(fgB));
                cell.background = Color(
                    static_cast<uint8_t>(bgR),
                    static_cast<uint8_t>(bgG),
                    static_cast<uint8_t>(bgB));
                cell.bold = bold;
                cell.italic = italic;
                cell.underline = underline;
                cell.strikethrough = strikethrough;
                cell.inverse = inverse;

                // 4. Serialize then parse
                QByteArray serialized = AnsiRenderer::sgrSequence(cell);
                Cell parsed = AnsiRenderer::parseSgrSequence(serialized);

                // 5. Assert round-trip equivalence for colors
                RC_ASSERT(parsed.foreground.r == cell.foreground.r);
                RC_ASSERT(parsed.foreground.g == cell.foreground.g);
                RC_ASSERT(parsed.foreground.b == cell.foreground.b);
                RC_ASSERT(parsed.background.r == cell.background.r);
                RC_ASSERT(parsed.background.g == cell.background.g);
                RC_ASSERT(parsed.background.b == cell.background.b);

                // 6. Assert round-trip equivalence for attributes
                RC_ASSERT(parsed.bold == cell.bold);
                RC_ASSERT(parsed.italic == cell.italic);
                RC_ASSERT(parsed.underline == cell.underline);
                RC_ASSERT(parsed.strikethrough == cell.strikethrough);
                RC_ASSERT(parsed.inverse == cell.inverse);
            });
    }

    // Feature: qttui-framework, Property 34: CUP cursor position round-trip
    void cupCursorPositionRoundTrip()
    {
        rc::check("parseCupSequence(cursorPosition(x, y)) produces same (x, y)",
            []() {
                // 1. Generate random 0-based coordinates within terminal bounds
                const auto x = *rc::gen::inRange(0, 10000);
                const auto y = *rc::gen::inRange(0, 10000);

                // 2. Serialize to CUP sequence
                QByteArray serialized = AnsiRenderer::cursorPosition(x, y);

                // 3. Parse back to 0-based QPoint
                QPoint parsed = AnsiRenderer::parseCupSequence(serialized);

                // 4. Assert round-trip equivalence
                RC_ASSERT(parsed.x() == x);
                RC_ASSERT(parsed.y() == y);
            });
    }

    // ------------------------------------------------------------------
    // Unicode cell model: a wide CJK glyph is emitted exactly once and its
    // continuation cell produces no output (the terminal advances 2 columns
    // by itself)
    // ------------------------------------------------------------------
    void wideCjkEmitsOnceAndSkipsContinuation()
    {
        ScreenBuffer buf(10, 1);
        buf.swapBuffers(); // front == back

        Cell style;
        buf.putChar(0, 0, 0x65E5, style); // 日 -> pair at (0,1)
        buf.putChar(2, 0, U'x', style);   // narrow char right after the pair

        AnsiRenderer renderer;
        const QByteArray out = renderer.render(buf);

        const QByteArray cjk = QString(QChar(0x65E5)).toUtf8();
        QCOMPARE(cjk.size(), 3); // sanity: 3-byte UTF-8
        QCOMPARE(out.count(cjk), 1); // emitted exactly once

        // The whole row renders as ONE run: a single CUP at column 1, no
        // repositioning for the continuation cell or for 'x'
        QVERIFY(out.contains("\x1b[1;1H"));
        QVERIFY(!out.contains("\x1b[1;2H")); // no run restarted at the continuation
        QVERIFY(!out.contains("\x1b[1;3H")); // 'x' continues the same run

        // No garbage between the glyph and the following narrow char
        const int cjkPos = out.indexOf(cjk);
        const int xPos = out.indexOf('x', cjkPos);
        QVERIFY(xPos == cjkPos + cjk.size()); // 'x' immediately follows (same SGR)
    }

    void emojiEmitsFourByteUtf8()
    {
        ScreenBuffer buf(10, 1);
        buf.swapBuffers();

        Cell style;
        buf.putChar(0, 0, 0x1F600, style); // 😀

        AnsiRenderer renderer;
        const QByteArray out = renderer.render(buf);

        QVERIFY(out.contains(QByteArray("\xF0\x9F\x98\x80")));
        QCOMPARE(out.count(QByteArray("\xF0\x9F\x98\x80")), 1);
        QVERIFY(!out.contains("\x1b[1;2H")); // continuation produced no run
    }

    void combiningMarksAppendedAfterBase()
    {
        ScreenBuffer buf(10, 1);
        buf.swapBuffers();

        Cell style;
        buf.putChar(0, 0, U'e', QString(QChar(0x0301)), style);

        AnsiRenderer renderer;
        const QByteArray out = renderer.render(buf);
        QVERIFY(out.contains(QByteArray("e\xCC\x81"))); // e + combining acute as one cell
    }

    // Defensive path: a lone continuation cell (its pair broken by a direct
    // cell write) must render as a space, never as garbage
    void orphanContinuationRendersAsSpace()
    {
        ScreenBuffer buf(10, 1);
        buf.swapBuffers();

        // Forge an orphan continuation directly (bypassing putChar)
        Cell &cell = buf.backCell(4, 0);
        cell.ch = 0;
        cell.width = 0;
        cell.foreground = Color(10, 20, 30);

        AnsiRenderer renderer;
        const QByteArray out = renderer.render(buf);
        QVERIFY(out.contains("\x1b[1;5H")); // positioned at the cell
        // The last payload byte before the trailing reset is a space
        const QByteArray reset = AnsiRenderer::resetAttributes();
        QVERIFY(out.endsWith(QByteArray(" ") + reset));
    }
};

QTEST_MAIN(TestAnsiRenderer)
#include "test_ansi_renderer.moc"
