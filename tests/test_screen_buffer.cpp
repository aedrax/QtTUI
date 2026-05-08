#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/QtTuiUnicode.h>

using namespace QtTUI;

class TestScreenBuffer : public QObject {
    Q_OBJECT
private slots:
    // Feature: qttui-framework, Property 5: Back buffer isolation
    void backBufferIsolation()
    {
        rc::check("Writing to back buffer shall not change front buffer until swapBuffers()",
            []() {
                const auto w = *rc::gen::inRange(1, 50);
                const auto h = *rc::gen::inRange(1, 50);
                ScreenBuffer buf(w, h);

                // Swap once so front has known default state
                buf.swapBuffers();

                // Pick a random valid position
                const auto x = *rc::gen::inRange(0, w);
                const auto y = *rc::gen::inRange(0, h);

                // Snapshot front cell before modification
                const Cell frontBefore = buf.frontCell(x, y);

                // Modify back buffer
                buf.backCell(x, y).setChar(U'X');
                buf.backCell(x, y).bold = true;
                buf.backCell(x, y).foreground = Color(42, 100, 200);

                // Front buffer must be unchanged
                const Cell &frontAfter = buf.frontCell(x, y);
                RC_ASSERT(frontAfter == frontBefore);

                // After swap, front should reflect the change
                buf.swapBuffers();
                RC_ASSERT(buf.frontCell(x, y).ch == U'X');
                RC_ASSERT(buf.frontCell(x, y).bold == true);
                RC_ASSERT(buf.frontCell(x, y).foreground == Color(42, 100, 200));
            });
    }

    // Feature: qttui-framework, Property 8: Screen buffer resize
    void screenBufferResize()
    {
        rc::check("After resize(w, h), both buffers have w*h cells and diff() returns all positions",
            []() {
                const auto initW = *rc::gen::inRange(1, 30);
                const auto initH = *rc::gen::inRange(1, 30);
                ScreenBuffer buf(initW, initH);

                const auto newW = *rc::gen::inRange(1, 50);
                const auto newH = *rc::gen::inRange(1, 50);
                buf.resize(newW, newH);

                RC_ASSERT(buf.width() == newW);
                RC_ASSERT(buf.height() == newH);

                // After resize, diff() should return all positions (full redraw)
                auto changed = buf.diff();

                // After resize, all positions should be in diff
                RC_ASSERT(changed.size() == newW * newH);

                // Verify all positions are present
                QSet<QPoint> changedSet(changed.begin(), changed.end());
                for (int y = 0; y < newH; ++y) {
                    for (int x = 0; x < newW; ++x) {
                        RC_ASSERT(changedSet.contains(QPoint(x, y)));
                    }
                }
            });
    }

    // ------------------------------------------------------------------
    // Unicode cell model: width table sanity
    // ------------------------------------------------------------------
    void charWidthTable()
    {
        // Narrow
        QCOMPARE(QtTuiUnicode::charWidth(U'a'), 1);
        QCOMPARE(QtTuiUnicode::charWidth(U' '), 1);
        QCOMPARE(QtTuiUnicode::charWidth(0x2500), 1); // box drawing ─
        QCOMPARE(QtTuiUnicode::charWidth(0x2026), 1); // ellipsis …
        QCOMPARE(QtTuiUnicode::charWidth(0x00E9), 1); // é (precomposed)
        // Wide: CJK, Hangul, fullwidth, emoji
        QCOMPARE(QtTuiUnicode::charWidth(0x65E5), 2);  // 日
        QCOMPARE(QtTuiUnicode::charWidth(0x672C), 2);  // 本
        QCOMPARE(QtTuiUnicode::charWidth(0xAC00), 2);  // 가
        QCOMPARE(QtTuiUnicode::charWidth(0xFF21), 2);  // Ａ fullwidth A
        QCOMPARE(QtTuiUnicode::charWidth(0x1F600), 2); // 😀
        QCOMPARE(QtTuiUnicode::charWidth(0x1F98A), 2); // 🦊
        QCOMPARE(QtTuiUnicode::charWidth(0x20000), 2); // CJK ext B
        // Zero width: combining marks, ZWJ, variation selectors
        QCOMPARE(QtTuiUnicode::charWidth(0x0301), 0);  // combining acute
        QCOMPARE(QtTuiUnicode::charWidth(0x200D), 0);  // ZWJ
        QCOMPARE(QtTuiUnicode::charWidth(0xFE0F), 0);  // VS16
        // Continuation marker
        QCOMPARE(QtTuiUnicode::charWidth(0), 0);

        // String width iterates by code point (surrogate-aware)
        QCOMPARE(QtTuiUnicode::stringWidth(QStringLiteral("abc")), 3);
        QCOMPARE(QtTuiUnicode::stringWidth(QString::fromUtf8("日本語")), 6);
        QCOMPARE(QtTuiUnicode::stringWidth(QString::fromUtf8("a😀b")), 4);
        QCOMPARE(QtTuiUnicode::stringWidth(QString::fromUtf8("e\xCC\x81")), 1); // e + U+0301
    }

    void indexColumnMapping()
    {
        const QString s = QString::fromUtf8("a日😀b"); // idx: a=0, 日=1, 😀=2..3, b=4
                                                        // col: a=0, 日=1-2, 😀=3-4, b=5
        QCOMPARE(QtTuiUnicode::columnOfIndex(s, 0), 0);
        QCOMPARE(QtTuiUnicode::columnOfIndex(s, 1), 1);
        QCOMPARE(QtTuiUnicode::columnOfIndex(s, 2), 3);
        QCOMPARE(QtTuiUnicode::columnOfIndex(s, 4), 5);
        QCOMPARE(QtTuiUnicode::columnOfIndex(s, 5), 6); // end

        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 0), 0);
        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 1), 1);
        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 2), 1); // middle of 日 snaps to its start
        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 3), 2);
        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 4), 2); // middle of 😀
        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 5), 4);
        QCOMPARE(QtTuiUnicode::indexOfColumn(s, 6), 5); // past end -> size()

        QCOMPARE(QtTuiUnicode::nextClusterIndex(s, 0), 1);
        QCOMPARE(QtTuiUnicode::nextClusterIndex(s, 2), 4); // over the surrogate pair
        QCOMPARE(QtTuiUnicode::prevClusterIndex(s, 4), 2);
        QCOMPARE(QtTuiUnicode::prevClusterIndex(s, 1), 0);

        // Combining marks belong to the preceding cluster
        const QString accent = QString::fromUtf8("ae\xCC\x81x"); // a, e+U+0301, x
        QCOMPARE(QtTuiUnicode::nextClusterIndex(accent, 1), 3);
        QCOMPARE(QtTuiUnicode::prevClusterIndex(accent, 3), 1);
        QCOMPARE(QtTuiUnicode::columnOfIndex(accent, 3), 2);
    }

    // ------------------------------------------------------------------
    // Unicode cell model: wide glyph occupies glyph + continuation cell
    // ------------------------------------------------------------------
    void putCharWideOccupiesTwoCells()
    {
        ScreenBuffer buf(10, 2);
        Cell style;
        style.foreground = Color(1, 2, 3);

        buf.putChar(2, 0, 0x65E5, style); // 日

        const Cell &glyph = buf.backCell(2, 0);
        QCOMPARE(glyph.ch, char32_t(0x65E5));
        QCOMPARE(int(glyph.width), 2);
        QVERIFY(!glyph.isContinuation());
        QCOMPARE(glyph.foreground, Color(1, 2, 3));

        const Cell &cont = buf.backCell(3, 0);
        QVERIFY(cont.isContinuation());
        QCOMPARE(cont.ch, char32_t(0));
        QCOMPARE(int(cont.width), 0);
        // Continuation carries the glyph's attributes
        QCOMPARE(cont.foreground, Color(1, 2, 3));

        // Neighbors untouched
        QCOMPARE(buf.backCell(1, 0), Cell());
        QCOMPARE(buf.backCell(4, 0), Cell());
    }

    // ------------------------------------------------------------------
    // Overwriting either half of a wide pair resets the other half
    // ------------------------------------------------------------------
    void overwritingGlyphHalfResetsContinuation()
    {
        ScreenBuffer buf(10, 1);
        Cell style;
        buf.putChar(2, 0, 0x65E5, style); // pair at (2,3)

        buf.putChar(2, 0, U'x', style); // overwrite the glyph half
        QCOMPARE(buf.backCell(2, 0).ch, U'x');
        QCOMPARE(int(buf.backCell(2, 0).width), 1);
        // The orphaned continuation must have been reset to a space
        QCOMPARE(buf.backCell(3, 0).ch, U' ');
        QCOMPARE(int(buf.backCell(3, 0).width), 1);
    }

    void overwritingContinuationHalfResetsGlyph()
    {
        ScreenBuffer buf(10, 1);
        Cell style;
        buf.putChar(2, 0, 0x65E5, style); // pair at (2,3)

        buf.putChar(3, 0, U'x', style); // overwrite the continuation half
        QCOMPARE(buf.backCell(3, 0).ch, U'x');
        // The orphaned glyph must have been reset to a space
        QCOMPARE(buf.backCell(2, 0).ch, U' ');
        QCOMPARE(int(buf.backCell(2, 0).width), 1);
    }

    void wideOverwritingNeighborPairsResetsThem()
    {
        ScreenBuffer buf(10, 1);
        Cell style;
        buf.putChar(0, 0, 0x65E5, style); // pair at (0,1)
        buf.putChar(2, 0, 0x672C, style); // pair at (2,3)

        // Writing a wide glyph at 1 overwrites the continuation of the first
        // pair and the glyph of the second pair
        buf.putChar(1, 0, 0x8A9E, style); // 語 pair at (1,2)
        QCOMPARE(buf.backCell(0, 0).ch, U' ');   // first glyph reset
        QCOMPARE(buf.backCell(1, 0).ch, char32_t(0x8A9E));
        QVERIFY(buf.backCell(2, 0).isContinuation());
        QCOMPARE(buf.backCell(3, 0).ch, U' ');   // orphaned continuation reset
    }

    // ------------------------------------------------------------------
    // A wide glyph cannot straddle the right buffer edge
    // ------------------------------------------------------------------
    void wideAtLastColumnBecomesSpace()
    {
        ScreenBuffer buf(4, 1);
        Cell style;
        buf.putChar(3, 0, 0x65E5, style);
        QCOMPARE(buf.backCell(3, 0).ch, U' ');
        QCOMPARE(int(buf.backCell(3, 0).width), 1);
    }

    // ------------------------------------------------------------------
    // Combining marks ride along in the glyph cell
    // ------------------------------------------------------------------
    void putCharStoresCombiningMarks()
    {
        ScreenBuffer buf(4, 1);
        Cell style;
        buf.putChar(0, 0, U'e', QString(QChar(0x0301)), style);
        QCOMPARE(buf.backCell(0, 0).ch, U'e');
        QCOMPARE(int(buf.backCell(0, 0).width), 1);
        QCOMPARE(buf.backCell(0, 0).combining, QString(QChar(0x0301)));
        QCOMPARE(buf.backCell(0, 0).text(), QString::fromUtf8("e\xCC\x81"));
        // Combining marks participate in cell equality (diff correctness)
        Cell plain = buf.backCell(0, 0);
        plain.combining.clear();
        QVERIFY(plain != buf.backCell(0, 0));
    }

    // ------------------------------------------------------------------
    // diff() treats a wide pair atomically: if either half changed, both
    // positions are reported
    // ------------------------------------------------------------------
    void diffPairAtomicity()
    {
        ScreenBuffer buf(10, 1);
        Cell style;
        buf.putChar(2, 0, 0x65E5, style);
        buf.swapBuffers(); // front == back, no diff
        QVERIFY(buf.diff().isEmpty());

        // Touch ONLY the continuation cell (e.g. an attribute-only write)
        buf.backCell(3, 0).background = Color(9, 9, 9);
        QVector<QPoint> changed = buf.diff();
        QVERIFY(changed.contains(QPoint(2, 0))); // glyph re-emitted too
        QVERIFY(changed.contains(QPoint(3, 0)));

        // Restore, then touch ONLY the glyph cell
        buf.backCell(3, 0).background = Color(0, 0, 0);
        QVERIFY(buf.diff().isEmpty());
        buf.backCell(2, 0).bold = true;
        changed = buf.diff();
        QVERIFY(changed.contains(QPoint(2, 0)));
        QVERIFY(changed.contains(QPoint(3, 0))); // continuation dragged along
    }
};

QTEST_MAIN(TestScreenBuffer)
#include "test_screen_buffer.moc"
