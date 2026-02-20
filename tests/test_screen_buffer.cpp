#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/ScreenBuffer.h>

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
                buf.backCell(x, y).character = QChar('X');
                buf.backCell(x, y).bold = true;
                buf.backCell(x, y).foreground = Color(42, 100, 200);

                // Front buffer must be unchanged
                const Cell &frontAfter = buf.frontCell(x, y);
                RC_ASSERT(frontAfter == frontBefore);

                // After swap, front should reflect the change
                buf.swapBuffers();
                RC_ASSERT(buf.frontCell(x, y).character == QChar('X'));
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
};

QTEST_MAIN(TestScreenBuffer)
#include "test_screen_buffer.moc"
