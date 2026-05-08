#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

// Counts ScreenBuffer out-of-bounds warnings while installed (and swallows
// them so test output stays clean). RAII so a failing QCOMPARE cannot leave
// the counting handler installed for subsequent tests.
static int s_oobWarningCount = 0;

static void countingMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (type == QtWarningMsg && msg.contains(QLatin1String("out of bounds")))
        ++s_oobWarningCount;
}

struct WarningCounterScope {
    QtMessageHandler previous;
    WarningCounterScope()
    {
        s_oobWarningCount = 0;
        previous = qInstallMessageHandler(countingMessageHandler);
    }
    ~WarningCounterScope() { qInstallMessageHandler(previous); }
};

class TestScreenBufferBounds : public QObject {
    Q_OBJECT
private slots:
    // Feature: widget-rendering-overhaul, Property 35: ScreenBuffer Bounds Safety
    void boundsSafety()
    {
        rc::check("Out-of-bounds backCell/frontCell return default Cell without modifying buffer",
            []() {
                const auto w = *rc::gen::inRange(1, 50);
                const auto h = *rc::gen::inRange(1, 50);
                ScreenBuffer buf(w, h);

                // Write known content into every in-bounds cell so we can detect modifications
                const Cell marker = []() {
                    Cell c;
                    c.setChar(U'M');
                    c.foreground = Color(10, 20, 30);
                    c.background = Color(40, 50, 60);
                    c.bold = true;
                    return c;
                }();

                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        buf.backCell(x, y) = marker;
                    }
                }

                // Snapshot the buffer state
                QVector<Cell> snapshot;
                snapshot.reserve(w * h);
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        snapshot.append(buf.backCell(x, y));
                    }
                }

                // Generate an out-of-bounds coordinate.
                // We pick from four categories: negative x, negative y, x >= w, y >= h
                const int category = *rc::gen::inRange(0, 4);
                int oobX = 0, oobY = 0;
                switch (category) {
                case 0: // negative x
                    oobX = *rc::gen::inRange(-500, 0);
                    oobY = *rc::gen::inRange(-500, h + 500);
                    break;
                case 1: // negative y
                    oobX = *rc::gen::inRange(-500, w + 500);
                    oobY = *rc::gen::inRange(-500, 0);
                    break;
                case 2: // x >= width
                    oobX = *rc::gen::inRange(w, w + 500);
                    oobY = *rc::gen::inRange(0, h);
                    break;
                case 3: // y >= height
                    oobX = *rc::gen::inRange(0, w);
                    oobY = *rc::gen::inRange(h, h + 500);
                    break;
                }

                // backCell (mutable) must return default Cell
                const Cell defaultCell;
                Cell &returned = buf.backCell(oobX, oobY);
                RC_ASSERT(returned == defaultCell);

                // frontCell must also return default Cell
                const Cell &frontReturned = buf.frontCell(oobX, oobY);
                RC_ASSERT(frontReturned == defaultCell);

                // const backCell overload must return default Cell
                const ScreenBuffer &constBuf = buf;
                const Cell &constReturned = constBuf.backCell(oobX, oobY);
                RC_ASSERT(constReturned == defaultCell);

                // Verify no in-bounds cell was modified
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        RC_ASSERT(buf.backCell(x, y) == snapshot[y * w + x]);
                    }
                }
            });
    }

    // Unit test: writing to the returned default cell does not corrupt the buffer
    void writeToDefaultCellIsHarmless()
    {
        ScreenBuffer buf(5, 5);

        // Fill with known content
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 5; ++x)
                buf.backCell(x, y).setChar(U'A');

        // Write to the out-of-bounds returned reference
        Cell &oob = buf.backCell(100, 100);
        oob.setChar(U'Z');
        oob.bold = true;
        oob.foreground = Color(255, 0, 0);

        // All in-bounds cells must still be 'A'
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 5; ++x)
                QCOMPARE(buf.backCell(x, y).ch, U'A');
    }

    // The sentinel cell returned for OOB accesses is shared; it must be reset
    // on every OOB access so one caller's OOB write cannot leak into another
    // caller's OOB read.
    void oobWriteDoesNotLeakIntoOobRead()
    {
        ScreenBuffer buf(3, 3);

        // Dirty the sentinel through the mutable OOB path
        Cell &oobWrite = buf.backCell(50, 50);
        oobWrite.setChar(U'Z');
        oobWrite.bold = true;
        oobWrite.foreground = Color(255, 0, 0);

        // Every subsequent OOB access (any overload, any buffer) must see a
        // default-constructed cell again.
        const ScreenBuffer &constBuf = buf;
        QVERIFY(constBuf.backCell(50, 50) == Cell());
        QVERIFY(buf.frontCell(50, 50) == Cell());
        QVERIFY(buf.backCell(-1, -1) == Cell());

        ScreenBuffer other(2, 2);
        QVERIFY(other.frontCell(99, 99) == Cell());
    }

    // OOB accesses can happen thousands of times per frame; the warning must
    // fire only once per buffer, with further warnings suppressed.
    void oobWarningEmittedOncePerBuffer()
    {
        WarningCounterScope scope;

        ScreenBuffer buf(4, 4);
        const ScreenBuffer &constBuf = buf;
        for (int i = 0; i < 100; ++i) {
            buf.backCell(-1, -1);
            buf.frontCell(100, 100);
            constBuf.backCell(9, 9);
        }
        QCOMPARE(s_oobWarningCount, 1);

        // In-bounds accesses never warn
        buf.backCell(0, 0);
        QCOMPARE(s_oobWarningCount, 1);

        // The flag is per buffer: a fresh buffer warns once more
        ScreenBuffer other(2, 2);
        other.backCell(-5, 0);
        other.backCell(-5, 0);
        QCOMPARE(s_oobWarningCount, 2);
    }
};

QTEST_MAIN(TestScreenBufferBounds)
#include "test_screenbuffer_bounds.moc"
