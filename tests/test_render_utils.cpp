#include <QtTest/QtTest>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>

using namespace QtTUI;

// The fill, border, text and window frame helpers of RenderUtils

static QString rowText(const ScreenBuffer &buffer, int y)
{
    QString s;
    for (int x = 0; x < buffer.width(); ++x)
        s += buffer.backCell(x, y).text();
    return s;
}

static ScreenBuffer dottedBuffer(int width, int height)
{
    ScreenBuffer buffer(width, height);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            buffer.backCell(x, y).setChar(U'.');
    return buffer;
}

class TestRenderUtils : public QObject {
    Q_OBJECT
private slots:
    void fillRectWithNegativeWidthWritesNothing()
    {
        // QRect::intersected() normalizes a negative size; fillRect() must not
        // fill backward from x
        ScreenBuffer buffer = dottedBuffer(10, 1);
        fillRect(buffer, QRect(8, 0, -5, 1), cellStyle(Color(1, 2, 3), Color(4, 5, 6)));
        QCOMPARE(rowText(buffer, 0), QStringLiteral(".........."));
    }

    void fillRectClipsToTheBuffer()
    {
        ScreenBuffer buffer = dottedBuffer(5, 2);
        fillRect(buffer, QRect(3, 1, 10, 4), cellStyle(Color(1, 2, 3), Color(4, 5, 6)));
        QCOMPARE(rowText(buffer, 0), QStringLiteral("....."));
        QCOMPARE(rowText(buffer, 1), QStringLiteral("...  "));
        QCOMPARE(buffer.backCell(4, 1).background, Color(4, 5, 6));
    }

    void renderTextStopsBeforeAWideCharacterThatDoesNotFit()
    {
        ScreenBuffer buffer = dottedBuffer(10, 1);
        // "ab" and the first wide character fill 4 columns; the second wide
        // character needs columns 4 and 5, but maxWidth is 5
        const int written = renderText(buffer, 0, 0, QStringLiteral("ab漢字"), 5, Cell());
        QCOMPARE(written, 4);
        QCOMPARE(buffer.backCell(2, 0).width, quint8(2));
        QVERIFY(buffer.backCell(3, 0).isContinuation());
        QCOMPARE(buffer.backCell(4, 0).ch, char32_t(U'.'));
    }

    void renderTextStopsAtTheBufferEdge()
    {
        ScreenBuffer buffer = dottedBuffer(4, 1);
        QCOMPARE(renderText(buffer, 2, 0, QStringLiteral("hello"), 10, Cell()), 2);
        QCOMPARE(rowText(buffer, 0), QStringLiteral("..he"));
    }

    void drawBorderOnOneAndTwoRows()
    {
        ScreenBuffer buffer = dottedBuffer(4, 3);
        drawBorder(buffer, QRect(0, 0, 4, 1), QStringLiteral("single"), Cell());
        QCOMPARE(rowText(buffer, 0), QStringLiteral("┌──┐"));

        drawBorder(buffer, QRect(0, 1, 4, 2), QStringLiteral("single"), Cell());
        QCOMPARE(rowText(buffer, 1), QStringLiteral("┌──┐"));
        QCOMPARE(rowText(buffer, 2), QStringLiteral("└──┘"));
    }

    void drawWindowFrameShowsButtonsThatFit()
    {
        ThemeProperties theme;
        ScreenBuffer buffer = dottedBuffer(9, 3);
        drawWindowFrame(buffer, QRect(0, 0, 9, 3), theme, QStringLiteral("Title"),
                        QStringLiteral("[□][X]"));
        QCOMPARE(rowText(buffer, 0), QStringLiteral("┌─[□][X]┐"));
        QCOMPARE(rowText(buffer, 1), QStringLiteral("│       │"));

        // Too narrow for the buttons: only the title shows
        buffer = dottedBuffer(8, 3);
        drawWindowFrame(buffer, QRect(0, 0, 8, 3), theme, QStringLiteral("Title"),
                        QStringLiteral("[□][X]"));
        QCOMPARE(rowText(buffer, 0), QStringLiteral("┌──────┐"));
    }
};

QTEST_MAIN(TestRenderUtils)
#include "test_render_utils.moc"
