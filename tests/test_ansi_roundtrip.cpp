#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

class TestAnsiRoundtrip : public QObject {
    Q_OBJECT
private slots:

    // Feature: widget-rendering-overhaul, Property 30: SGR Round-Trip
    void sgrRoundTrip()
    {
        rc::check("parseSgrSequence(sgrSequence(cell)) produces a Cell equivalent to the original",
            []() {
                // Generate random RGB values for foreground
                const auto fgR = *rc::gen::inRange(0, 256);
                const auto fgG = *rc::gen::inRange(0, 256);
                const auto fgB = *rc::gen::inRange(0, 256);

                // Generate random RGB values for background
                const auto bgR = *rc::gen::inRange(0, 256);
                const auto bgG = *rc::gen::inRange(0, 256);
                const auto bgB = *rc::gen::inRange(0, 256);

                // Generate random boolean attributes
                const auto bold = *rc::gen::arbitrary<bool>();
                const auto italic = *rc::gen::arbitrary<bool>();
                const auto underline = *rc::gen::arbitrary<bool>();
                const auto strikethrough = *rc::gen::arbitrary<bool>();
                const auto inverse = *rc::gen::arbitrary<bool>();

                // Build the Cell
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

                // Serialize then parse
                QByteArray serialized = AnsiRenderer::sgrSequence(cell);
                Cell parsed = AnsiRenderer::parseSgrSequence(serialized);

                // Assert round-trip equivalence for colors
                RC_ASSERT(parsed.foreground.r == cell.foreground.r);
                RC_ASSERT(parsed.foreground.g == cell.foreground.g);
                RC_ASSERT(parsed.foreground.b == cell.foreground.b);
                RC_ASSERT(parsed.background.r == cell.background.r);
                RC_ASSERT(parsed.background.g == cell.background.g);
                RC_ASSERT(parsed.background.b == cell.background.b);

                // Assert round-trip equivalence for attributes
                RC_ASSERT(parsed.bold == cell.bold);
                RC_ASSERT(parsed.italic == cell.italic);
                RC_ASSERT(parsed.underline == cell.underline);
                RC_ASSERT(parsed.strikethrough == cell.strikethrough);
                RC_ASSERT(parsed.inverse == cell.inverse);
            });
    }

    // Feature: widget-rendering-overhaul, Property 31: CUP Round-Trip
    void cupRoundTrip()
    {
        rc::check("parseCupSequence(cursorPosition(x, y)) produces the original (x, y)",
            []() {
                // Generate random 0-based coordinates within valid terminal bounds
                const auto x = *rc::gen::inRange(0, 10000);
                const auto y = *rc::gen::inRange(0, 10000);

                // Serialize to CUP sequence
                QByteArray serialized = AnsiRenderer::cursorPosition(x, y);

                // Parse back to 0-based QPoint
                QPoint parsed = AnsiRenderer::parseCupSequence(serialized);

                // Assert round-trip equivalence
                RC_ASSERT(parsed.x() == x);
                RC_ASSERT(parsed.y() == y);
            });
    }

    // Feature: widget-rendering-overhaul, Property 32: Valid ANSI Output
    void validAnsiOutput()
    {
        rc::check("AnsiRenderer::render() output contains only valid ANSI escape sequences and printable characters",
            []() {
                // Generate a random small buffer
                const auto w = *rc::gen::inRange(2, 31);
                const auto h = *rc::gen::inRange(2, 21);

                ScreenBuffer buf(w, h);
                buf.swapBuffers(); // sync front and back

                // Randomly modify some cells in the back buffer
                const auto numChanges = *rc::gen::inRange(0, w * h + 1);
                for (int i = 0; i < numChanges; ++i) {
                    const auto cx = *rc::gen::inRange(0, w);
                    const auto cy = *rc::gen::inRange(0, h);

                    // Random printable ASCII character
                    const auto ch = *rc::gen::inRange(32, 127);
                    buf.backCell(cx, cy).setChar(char32_t(ch));

                    // Random colors
                    buf.backCell(cx, cy).foreground = Color(
                        static_cast<uint8_t>(*rc::gen::inRange(0, 256)),
                        static_cast<uint8_t>(*rc::gen::inRange(0, 256)),
                        static_cast<uint8_t>(*rc::gen::inRange(0, 256)));
                    buf.backCell(cx, cy).background = Color(
                        static_cast<uint8_t>(*rc::gen::inRange(0, 256)),
                        static_cast<uint8_t>(*rc::gen::inRange(0, 256)),
                        static_cast<uint8_t>(*rc::gen::inRange(0, 256)));

                    // Random attributes
                    buf.backCell(cx, cy).bold = *rc::gen::arbitrary<bool>();
                    buf.backCell(cx, cy).italic = *rc::gen::arbitrary<bool>();
                    buf.backCell(cx, cy).underline = *rc::gen::arbitrary<bool>();
                    buf.backCell(cx, cy).strikethrough = *rc::gen::arbitrary<bool>();
                    buf.backCell(cx, cy).inverse = *rc::gen::arbitrary<bool>();
                }

                // Render
                AnsiRenderer renderer;
                QByteArray output = renderer.render(buf);

                // If no diff, output should be empty (valid trivially)
                if (output.isEmpty())
                    return;

                // Validate the output contains only valid ANSI sequences and printable chars.
                // Valid sequences are CSI sequences: ESC [ <params> <final byte>
                // where params are digits and semicolons, final byte is a letter.
                int i = 0;
                while (i < output.size()) {
                    if (output[i] == '\x1b') {
                        // Start of escape sequence
                        RC_ASSERT(i + 1 < output.size());
                        RC_ASSERT(output[i + 1] == '['); // Must be CSI (ESC [)
                        i += 2;

                        // Parse parameter bytes: digits and semicolons
                        while (i < output.size()
                               && ((output[i] >= '0' && output[i] <= '9') || output[i] == ';')) {
                            ++i;
                        }

                        // Must end with a valid final byte (letter in 0x40-0x7E range)
                        RC_ASSERT(i < output.size());
                        char finalByte = output[i];
                        RC_ASSERT(finalByte >= 0x40 && finalByte <= 0x7E);

                        // Specifically, we expect only SGR ('m'), CUP ('H'), or
                        // cursor movement ('A', 'B', 'C', 'D') as valid final bytes
                        RC_ASSERT(finalByte == 'm' || finalByte == 'H'
                                  || finalByte == 'A' || finalByte == 'B'
                                  || finalByte == 'C' || finalByte == 'D');
                        ++i;
                    } else {
                        // Must be a printable character (space through tilde, 0x20-0x7E)
                        RC_ASSERT(static_cast<unsigned char>(output[i]) >= 0x20
                                  && static_cast<unsigned char>(output[i]) <= 0x7E);
                        ++i;
                    }
                }
            });
    }

    // Unit test: empty diff produces empty ANSI output (Requirement 13.4)
    void emptyDiffProducesEmptyOutput()
    {
        ScreenBuffer buf(10, 10);
        buf.swapBuffers(); // front == back, no diff

        AnsiRenderer renderer;
        QByteArray result = renderer.render(buf);

        QVERIFY(result.isEmpty());
    }
};

QTEST_MAIN(TestAnsiRoundtrip)
#include "test_ansi_roundtrip.moc"
