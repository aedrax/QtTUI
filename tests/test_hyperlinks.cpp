// OSC 8 hyperlinks: the link table of ScreenBuffer, the link id in Cell,
// the diff, and the escape sequences of AnsiRenderer.
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/ScreenBuffer.h>

#include <QtTest/QtTest>

using namespace QtTUI;

static const QByteArray kClose = "\x1b]8;;\x1b\\";

static QByteArray linkOpen(const char *url)
{
    return QByteArray("\x1b]8;;") + url + "\x1b\\";
}

class TestHyperlinks : public QObject {
    Q_OBJECT
private slots:
    void init() { AnsiRenderer::setColorMode(AnsiRenderer::TrueColor); }

    void cellStaysSmall()
    {
        // The link id uses padding bytes of Cell on a 64-bit platform
        if (sizeof(void *) == 8)
            QCOMPARE(sizeof(Cell), size_t(48));
        QCOMPARE(Cell().link, quint16(0));
    }

    void linkTable()
    {
        ScreenBuffer buffer(10, 1);
        QCOMPARE(buffer.linkId(QString()), quint16(0));
        QVERIFY(buffer.linkUrl(0).isNull());

        const quint16 a = buffer.linkId(QStringLiteral("https://a.example/"));
        const quint16 b = buffer.linkId(QStringLiteral("https://b.example/"));
        QVERIFY(a != 0);
        QVERIFY(b != 0);
        QVERIFY(a != b);
        // The same URL always gets the same id, so a repaint changes no cell
        QCOMPARE(buffer.linkId(QStringLiteral("https://a.example/")), a);
        QCOMPARE(buffer.linkUrl(a), QStringLiteral("https://a.example/"));
        QVERIFY(buffer.linkUrl(999).isNull());

        // The ids stay valid after a resize
        buffer.resize(20, 2);
        QCOMPARE(buffer.linkUrl(b), QStringLiteral("https://b.example/"));
    }

    void urlHasNoControlCharacters()
    {
        ScreenBuffer buffer(10, 1);
        // ESC \ would end the OSC 8 sequence, and BEL too
        const QString hostile = QString::fromUtf8("https://x.example/\x1b\\\x1b[2J\x07\n");
        const quint16 id = buffer.linkId(hostile);
        QCOMPARE(buffer.linkUrl(id), QStringLiteral("https://x.example/\\[2J"));
        // Only control characters: no link
        QCOMPARE(buffer.linkId(QString::fromUtf8("\x1b\x07")), quint16(0));
    }

    void diffSeesALinkChange()
    {
        ScreenBuffer buffer(10, 1);
        const Cell style;
        renderText(buffer, 0, 0, QStringLiteral("abc"), 10, style);
        buffer.swapBuffers();
        QVERIFY(buffer.diff().isEmpty());

        // Same text and colors, but now with a link
        QCOMPARE(renderText(buffer, 0, 0, QStringLiteral("abc"), 10, style,
                            QStringLiteral("https://a.example/")), 3);
        QCOMPARE(buffer.diff(), QVector<QPoint>({QPoint(0, 0), QPoint(1, 0), QPoint(2, 0)}));
        buffer.swapBuffers();

        // The same link again: no change
        renderText(buffer, 0, 0, QStringLiteral("abc"), 10, style,
                   QStringLiteral("https://a.example/"));
        QVERIFY(buffer.diff().isEmpty());

        // A different URL: a change
        renderText(buffer, 0, 0, QStringLiteral("abc"), 10, style,
                   QStringLiteral("https://b.example/"));
        QCOMPARE(int(buffer.diff().size()), 3);
        buffer.swapBuffers();

        // Text without a link removes the link
        renderText(buffer, 0, 0, QStringLiteral("abc"), 10, style);
        QCOMPARE(buffer.backCell(0, 0).link, quint16(0));
        QCOMPARE(int(buffer.diff().size()), 3);
    }

    void wideCharacterCarriesTheLinkInBothCells()
    {
        ScreenBuffer buffer(10, 1);
        renderText(buffer, 0, 0, QStringLiteral("中"), 10, Cell(),
                   QStringLiteral("https://a.example/"));
        QVERIFY(buffer.backCell(0, 0).link != 0);
        QCOMPARE(buffer.backCell(1, 0).link, buffer.backCell(0, 0).link);
    }

    void rendererOpensAndClosesTheLink()
    {
        ScreenBuffer buffer(10, 1);
        buffer.swapBuffers();
        renderText(buffer, 0, 0, QStringLiteral("ab"), 10, Cell(),
                   QStringLiteral("https://a.example/"));
        renderText(buffer, 2, 0, QStringLiteral("c"), 10, Cell());

        AnsiRenderer renderer;
        QCOMPARE(renderer.render(buffer),
                 QByteArray("\x1b[1;1H") + AnsiRenderer::sgrSequence(Cell())
                     + linkOpen("https://a.example/") + "ab" + kClose + "c" + "\x1b[0m");
    }

    void rendererClosesTheLinkAtTheRunEnd()
    {
        ScreenBuffer buffer(10, 2);
        buffer.swapBuffers();
        // Two rows are two runs. Each run opens and closes its link.
        renderText(buffer, 0, 0, QStringLiteral("ab"), 10, Cell(),
                   QStringLiteral("https://a.example/"));
        renderText(buffer, 0, 1, QStringLiteral("cd"), 10, Cell(),
                   QStringLiteral("https://a.example/"));

        AnsiRenderer renderer;
        QCOMPARE(renderer.render(buffer),
                 QByteArray("\x1b[1;1H") + AnsiRenderer::sgrSequence(Cell())
                     + linkOpen("https://a.example/") + "ab" + kClose
                     + "\x1b[2;1H" + linkOpen("https://a.example/") + "cd" + kClose
                     + "\x1b[0m");
    }

    void rendererChangesFromOneLinkToTheNext()
    {
        ScreenBuffer buffer(10, 1);
        buffer.swapBuffers();
        renderText(buffer, 0, 0, QStringLiteral("a"), 10, Cell(),
                   QStringLiteral("https://a.example/"));
        renderText(buffer, 1, 0, QStringLiteral("b"), 10, Cell(),
                   QStringLiteral("https://b.example/"));

        AnsiRenderer renderer;
        QCOMPARE(renderer.render(buffer),
                 QByteArray("\x1b[1;1H") + AnsiRenderer::sgrSequence(Cell())
                     + linkOpen("https://a.example/") + "a"
                     + linkOpen("https://b.example/") + "b" + kClose + "\x1b[0m");
    }

    void noLinkNoOsc8()
    {
        ScreenBuffer buffer(10, 1);
        buffer.swapBuffers();
        renderText(buffer, 0, 0, QStringLiteral("abc"), 10, Cell());
        AnsiRenderer renderer;
        QVERIFY(!renderer.render(buffer).contains("\x1b]8"));
    }
};

QTEST_APPLESS_MAIN(TestHyperlinks)
#include "test_hyperlinks.moc"
