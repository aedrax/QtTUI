// Emoji clusters: ZWJ sequences, flags, variation selector 16 and skin
// tone modifiers are one cluster of 2 columns.
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/ScreenBuffer.h>

#include <QtTest/QtTest>

using namespace QtTUI;

static QString s(std::initializer_list<char32_t> codePoints)
{
    return QString::fromUcs4(codePoints.begin(), int(codePoints.size()));
}

static const char32_t ZWJ = 0x200D;
static const char32_t VS16 = 0xFE0F;

// man, ZWJ, woman, ZWJ, girl
static const QString kFamily = s({0x1F468, ZWJ, 0x1F469, ZWJ, 0x1F467});
// regional indicators D and E
static const QString kFlag = s({0x1F1E9, 0x1F1EA});
// heavy black heart (text presentation by default) with VS16
static const QString kHeart = s({0x2764, VS16});
// thumbs up with a medium skin tone
static const QString kThumb = s({0x1F44D, 0x1F3FD});

static int clusterCount(const QString &text)
{
    int n = 0;
    int i = 0;
    QtTuiUnicode::Cluster c;
    while (QtTuiUnicode::nextCluster(text, i, c))
        ++n;
    return n;
}

class TestUnicodeClusters : public QObject {
    Q_OBJECT
private slots:
    void width_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("clusters");
        QTest::addColumn<int>("columns");

        QTest::newRow("zwj family") << kFamily << 1 << 2;
        QTest::newRow("flag") << kFlag << 1 << 2;
        QTest::newRow("two flags") << (kFlag + s({0x1F1EB, 0x1F1F7})) << 2 << 4;
        QTest::newRow("flag and one indicator") << (kFlag + s({0x1F1EB})) << 2 << 3;
        QTest::newRow("heart text") << s({0x2764}) << 1 << 1;
        QTest::newRow("heart vs16") << kHeart << 1 << 2;
        QTest::newRow("skin tone") << kThumb << 1 << 2;
        QTest::newRow("skin tone on a narrow base") << s({0x261D, 0x1F3FD}) << 1 << 2;
        QTest::newRow("rainbow flag") << s({0x1F3F3, VS16, ZWJ, 0x1F308}) << 1 << 2;
        QTest::newRow("zwj with skin tones")
            << s({0x1F9D1, 0x1F3FD, ZWJ, 0x1F91D, ZWJ, 0x1F9D1, 0x1F3FB}) << 1 << 2;
        QTest::newRow("heart on fire") << s({0x2764, VS16, ZWJ, 0x1F525}) << 1 << 2;
        QTest::newRow("wide emoji") << s({0x1F600}) << 1 << 2;
        QTest::newRow("two wide emoji") << s({0x1F600, 0x1F601}) << 2 << 4;

        // No join without an emoji base
        QTest::newRow("letter and skin tone") << s({U'a', 0x1F3FD}) << 2 << 3;
        QTest::newRow("letters with zwj") << s({U'a', ZWJ, U'b'}) << 2 << 2;
        QTest::newRow("emoji zwj letter") << s({0x1F468, ZWJ, U'b'}) << 2 << 3;
        QTest::newRow("letter vs16") << s({U'a', VS16}) << 1 << 1;
        QTest::newRow("keycap") << s({U'1', VS16, 0x20E3}) << 1 << 1;
        QTest::newRow("cjk") << s({0x4E2D, 0x6587}) << 2 << 4;
        QTest::newRow("accent") << s({U'e', 0x0301}) << 1 << 1;

        // Ambiguous-width characters stay narrow
        QTest::newRow("ambiguous plus-minus") << s({0x00B1}) << 1 << 1;
        QTest::newRow("ambiguous circled one") << s({0x2460}) << 1 << 1;
        QTest::newRow("ambiguous alpha") << s({0x03B1}) << 1 << 1;
    }

    void width()
    {
        QFETCH(QString, text);
        QFETCH(int, clusters);
        QFETCH(int, columns);
        QCOMPARE(clusterCount(text), clusters);
        QCOMPARE(QtTuiUnicode::stringWidth(text), columns);
    }

    void clusterFields()
    {
        int i = 0;
        QtTuiUnicode::Cluster c;
        QVERIFY(QtTuiUnicode::nextCluster(kFamily, i, c));
        QCOMPARE(c.base, char32_t(0x1F468));
        QCOMPARE(c.combining, kFamily.mid(2));
        QCOMPARE(c.index, 0);
        QCOMPARE(c.length, int(kFamily.size()));
        QCOMPARE(c.width, 2);
        QCOMPARE(i, int(kFamily.size()));
    }

    void clusterWidthOfParts()
    {
        QCOMPARE(QtTuiUnicode::clusterWidth(0x2764, QString()), 1);
        QCOMPARE(QtTuiUnicode::clusterWidth(0x2764, s({VS16})), 2);
        QCOMPARE(QtTuiUnicode::clusterWidth(0x1F1E9, s({0x1F1EA})), 2);
        QCOMPARE(QtTuiUnicode::clusterWidth(U'e', s({0x0301})), 1);
        QCOMPARE(QtTuiUnicode::clusterWidth(0x4E2D, QString()), 2);
        // charWidth() does not change: these are zero-width code points
        QCOMPARE(QtTuiUnicode::charWidth(ZWJ), 0);
        QCOMPARE(QtTuiUnicode::charWidth(VS16), 0);
    }

    void cursorMovement()
    {
        // 'a' at index 0, the family at 1 to 8, 'b' at 9
        const QString text = QLatin1Char('a') + kFamily + QLatin1Char('b');
        QCOMPARE(int(text.size()), 10);

        QCOMPARE(QtTuiUnicode::nextClusterIndex(text, 0), 1);
        QCOMPARE(QtTuiUnicode::nextClusterIndex(text, 1), 9);
        QCOMPARE(QtTuiUnicode::nextClusterIndex(text, 4), 9); // from the middle
        QCOMPARE(QtTuiUnicode::nextClusterIndex(text, 9), 10);

        QCOMPARE(QtTuiUnicode::prevClusterIndex(text, 10), 9);
        QCOMPARE(QtTuiUnicode::prevClusterIndex(text, 9), 1);
        QCOMPARE(QtTuiUnicode::prevClusterIndex(text, 4), 1); // from the middle
        QCOMPARE(QtTuiUnicode::prevClusterIndex(text, 1), 0);

        QCOMPARE(QtTuiUnicode::columnOfIndex(text, 1), 1);
        QCOMPARE(QtTuiUnicode::columnOfIndex(text, 4), 1);
        QCOMPARE(QtTuiUnicode::columnOfIndex(text, 9), 3);
        QCOMPARE(QtTuiUnicode::columnOfIndex(text, 10), 4);

        QCOMPARE(QtTuiUnicode::indexOfColumn(text, 0), 0);
        QCOMPARE(QtTuiUnicode::indexOfColumn(text, 1), 1);
        QCOMPARE(QtTuiUnicode::indexOfColumn(text, 2), 1); // second column of the family
        QCOMPARE(QtTuiUnicode::indexOfColumn(text, 3), 9);
        QCOMPARE(QtTuiUnicode::indexOfColumn(text, 4), 10);

        // A flag: one step for the two indicators
        const QString flags = kFlag + kFlag;
        QCOMPARE(QtTuiUnicode::nextClusterIndex(flags, 0), 4);
        QCOMPARE(QtTuiUnicode::prevClusterIndex(flags, 8), 4);
    }

    void renderTextCells_data()
    {
        QTest::addColumn<QString>("cluster");
        QTest::newRow("zwj family") << kFamily;
        QTest::newRow("flag") << kFlag;
        QTest::newRow("heart vs16") << kHeart;
        QTest::newRow("skin tone") << kThumb;
    }

    void renderTextCells()
    {
        QFETCH(QString, cluster);
        ScreenBuffer buffer(10, 1);
        const QString text = QLatin1Char('A') + cluster + QLatin1Char('B');
        QCOMPARE(renderText(buffer, 0, 0, text, 10, Cell()), 4);

        QCOMPARE(buffer.backCell(0, 0).text(), QStringLiteral("A"));
        // One glyph cell with the complete cluster, and its continuation
        QCOMPARE(buffer.backCell(1, 0).width, quint8(2));
        QCOMPARE(buffer.backCell(1, 0).text(), cluster);
        QVERIFY(buffer.backCell(2, 0).isContinuation());
        QCOMPARE(buffer.backCell(3, 0).text(), QStringLiteral("B"));
        QCOMPARE(buffer.backCell(4, 0).text(), QStringLiteral(" "));
    }

    void renderTextDoesNotSplitACluster()
    {
        ScreenBuffer buffer(10, 1);
        // One column is not sufficient for a flag
        QCOMPARE(renderText(buffer, 0, 0, kFlag, 1, Cell()), 0);
        QCOMPARE(buffer.backCell(0, 0).text(), QStringLiteral(" "));
        // The ellipsis replaces the cluster, not a part of it
        renderTextTruncated(buffer, 0, 0, QLatin1Char('A') + kFamily + QLatin1Char('B'), 3, Cell());
        QCOMPARE(buffer.backCell(0, 0).text(), QStringLiteral("A"));
        QCOMPARE(buffer.backCell(1, 0).text(), QStringLiteral(" "));
        QCOMPARE(buffer.backCell(2, 0).text(), QStringLiteral("…"));
    }

    void rendererWritesTheCompleteCluster()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::TrueColor);
        ScreenBuffer buffer(10, 1);
        buffer.swapBuffers();
        renderText(buffer, 0, 0, kFlag + QLatin1Char('B'), 10, Cell());

        AnsiRenderer renderer;
        const QByteArray out = renderer.render(buffer);
        QCOMPARE(int(out.count(kFlag.toUtf8())), 1);
        // Terminals do not agree on the width of such a cluster. The
        // renderer clears the two columns first, and it sets the cursor
        // again for the next cell.
        QCOMPARE(out, QByteArray("\x1b[1;1H") + AnsiRenderer::sgrSequence(Cell())
                          + "  \x1b[1;1H" + kFlag.toUtf8() + "\x1b[1;3H" + "B\x1b[0m");
    }

    void rendererKeepsOneRunForPlainWideCharacters()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::TrueColor);
        ScreenBuffer buffer(10, 1);
        buffer.swapBuffers();
        renderText(buffer, 0, 0, QStringLiteral("中") + QLatin1Char('B'), 10, Cell());

        AnsiRenderer renderer;
        QCOMPARE(renderer.render(buffer),
                 QByteArray("\x1b[1;1H") + AnsiRenderer::sgrSequence(Cell())
                     + QByteArray("\xe4\xb8\xad") + "B\x1b[0m");
    }
};

QTEST_APPLESS_MAIN(TestUnicodeClusters)
#include "test_unicode_clusters.moc"
