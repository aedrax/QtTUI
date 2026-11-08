// Bidirectional text: QtTuiUnicode::visualOrder() puts a line in display
// order (UAX #9).
//
// Qt Core has no bidi reference. The expected values here are derived by
// hand from the rules of UAX #9, not from the implementation.
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/ScreenBuffer.h>

#include <QtTest/QtTest>

using namespace QtTUI;

static QString s(std::initializer_list<char32_t> codePoints)
{
    return QString::fromUcs4(codePoints.begin(), int(codePoints.size()));
}

// Hebrew letters (type R)
static const char32_t ALEF = 0x05D0, BET = 0x05D1, GIMEL = 0x05D2, DALET = 0x05D3,
                      HE = 0x05D4, VAV = 0x05D5;
// Arabic letter alef (type AL)
static const char32_t ARABIC_ALEF = 0x0627;
static const char32_t RLM = 0x200F;
static const char32_t ZWJ = 0x200D;

class TestBidi : public QObject {
    Q_OBJECT
private slots:
    void visualOrder_data()
    {
        QTest::addColumn<QString>("logical");
        QTest::addColumn<QString>("visual");

        QTest::newRow("empty") << QString() << QString();
        QTest::newRow("latin only") << QStringLiteral("abc (1-2)") << QStringLiteral("abc (1-2)");

        // All at level 1: one reversed run
        QTest::newRow("hebrew only") << s({ALEF, BET, GIMEL}) << s({GIMEL, BET, ALEF});

        // Paragraph level 0 (first strong is L). The Hebrew word is a
        // level 1 run. Each space is between L and R, so it stays L (N2).
        QTest::newRow("hebrew inside latin")
            << s({'a', 'b', 'c', ' ', ALEF, BET, GIMEL, ' ', 'd', 'e', 'f'})
            << s({'a', 'b', 'c', ' ', GIMEL, BET, ALEF, ' ', 'd', 'e', 'f'});

        // Paragraph level 1. The Latin word is a level 2 run: L2 reverses
        // it, then reverses the full line, so the word reads left to right.
        QTest::newRow("latin inside hebrew")
            << s({ALEF, BET, GIMEL, ' ', 'a', 'b', 'c', ' ', DALET, HE, VAV})
            << s({VAV, HE, DALET, ' ', 'a', 'b', 'c', ' ', GIMEL, BET, ALEF});

        // A European number in right-to-left text is level 2 (I2)
        QTest::newRow("number inside hebrew")
            << s({ALEF, BET, ' ', '1', '2', '3', ' ', GIMEL, DALET})
            << s({DALET, GIMEL, ' ', '1', '2', '3', ' ', BET, ALEF});
        // W4: the separator between two European numbers is a number.
        // W5: the terminator next to a European number is a number.
        QTest::newRow("decimal number") << s({ALEF, BET, ' ', '1', '0', '.', '5', '%'})
                                        << s({'1', '0', '.', '5', '%', ' ', BET, ALEF});
        QTest::newRow("number range") << s({ALEF, BET, ' ', '1', '-', '2'})
                                      << s({'1', '-', '2', ' ', BET, ALEF});
        // Paragraph level 0. The number is not after L, so it stays a
        // number at level 2 (W7 does not apply). The space between R and
        // the number is R (N1). The run "alef bet space 123" is reversed
        // as one, with the digits in order.
        QTest::newRow("number after hebrew in latin")
            << s({'a', ' ', ALEF, BET, ' ', '1', '2', '3'})
            << s({'a', ' ', '1', '2', '3', ' ', BET, ALEF});
        // W2: a European number after an Arabic letter is an Arabic
        // number. W4 does not join ES between Arabic numbers, so the hyphen
        // is a neutral between two numbers and gets R (N1): the two
        // numbers change places.
        QTest::newRow("number range after arabic")
            << s({ARABIC_ALEF, ' ', '1', '-', '2'})
            << s({'2', '-', '1', ' ', ARABIC_ALEF});
        // Arabic-Indic digits (AN) at paragraph level 0: the numbers are
        // level 2 and the space between them is level 1 (N1, a number is R)
        QTest::newRow("two arabic numbers") << s({0x0661, 0x0662, ' ', 0x0663, 0x0664})
                                            << s({0x0663, 0x0664, ' ', 0x0661, 0x0662});

        // L4: all is level 1, so the line is reversed and each bracket
        // shows as its mirror character
        QTest::newRow("brackets around hebrew")
            << s({ALEF, BET, ' ', '(', GIMEL, DALET, ')'})
            << s({'(', DALET, GIMEL, ')', ' ', BET, ALEF});
        // N0: the pair has only L in it, the opposite of the paragraph
        // direction, and the text before the pair is R. So the two
        // brackets are R. Without N0 the close bracket is L, between "abc"
        // and "def".
        QTest::newRow("bracket pair around latin in hebrew")
            << s({ALEF, BET, ' ', '(', 'a', 'b', 'c', ')', ' ', 'd', 'e', 'f'})
            << s({'d', 'e', 'f', ' ', '(', 'a', 'b', 'c', ')', ' ', BET, ALEF});
        // N0 at paragraph level 0: the text before the pair is L, so the
        // two brackets are L and do not mirror
        QTest::newRow("bracket pair around hebrew in latin")
            << s({'a', ' ', '(', ALEF, BET, ')', ' ', 'b'})
            << s({'a', ' ', '(', BET, ALEF, ')', ' ', 'b'});

        // Niqqud: shin with qamats and shin dot, lamed, vav with holam,
        // final mem. Each mark stays after its base.
        QTest::newRow("combining marks")
            << s({0x05E9, 0x05B8, 0x05C1, 0x05DC, 0x05D5, 0x05B9, 0x05DD})
            << s({0x05DD, 0x05D5, 0x05B9, 0x05DC, 0x05E9, 0x05B8, 0x05C1});

        // The family emoji is one neutral cluster between R and R (N1).
        // Its code points keep their order.
        QTest::newRow("emoji cluster inside hebrew")
            << s({ALEF, BET, ' ', 0x1F468, ZWJ, 0x1F469, ZWJ, 0x1F467, ' ', GIMEL, DALET})
            << s({DALET, GIMEL, ' ', 0x1F468, ZWJ, 0x1F469, ZWJ, 0x1F467, ' ', BET, ALEF});

        // The "!" is between R and the end of an L paragraph, so it is L (N2)
        QTest::newRow("punctuation after hebrew in latin")
            << s({'a', ' ', ALEF, BET, '!'}) << s({'a', ' ', BET, ALEF, '!'});
        // With a right-to-left mark after it, the "!" is between R and R.
        // The mark is zero-width and stays with the "!".
        QTest::newRow("right-to-left mark")
            << s({'a', ' ', ALEF, BET, '!', RLM}) << s({'a', ' ', '!', RLM, BET, ALEF});
    }

    void visualOrder()
    {
        QFETCH(QString, logical);
        QFETCH(QString, visual);
        QCOMPARE(QtTuiUnicode::visualOrder(logical), visual);
    }

    void baseDirection()
    {
        // Paragraph level 1: the letters are level 2, the "." is level 1
        QCOMPARE(QtTuiUnicode::visualOrder(QStringLiteral("abc."), Qt::RightToLeft),
                 QStringLiteral(".abc"));
        // Paragraph level 0: the "." is at the end of an L paragraph (N2)
        QCOMPARE(QtTuiUnicode::visualOrder(s({ALEF, BET, '.'}), Qt::LeftToRight),
                 s({BET, ALEF, '.'}));
        QCOMPARE(QtTuiUnicode::visualOrder(s({ALEF, BET, '.'})), s({'.', BET, ALEF}));
    }

    // Left-to-right text takes the fast path: the result shares the data
    // of the argument, so there is no allocation and no copy
    void fastPath()
    {
        const QString text = QStringLiteral("File (1-2) é中\U0001F600");
        QVERIFY(!QtTuiUnicode::hasRightToLeft(text));
        const QString out = QtTuiUnicode::visualOrder(text);
        QCOMPARE(out, text);
        QCOMPARE(out.constData(), text.constData());

        QVERIFY(QtTuiUnicode::hasRightToLeft(s({'a', ALEF})));
        QVERIFY(QtTuiUnicode::hasRightToLeft(s({'a', ARABIC_ALEF})));
        QVERIFY(QtTuiUnicode::hasRightToLeft(s({'a', RLM})));
        QVERIFY(QtTuiUnicode::hasRightToLeft(s({0x10800}))); // Cypriot syllable
        QVERIFY(QtTuiUnicode::hasRightToLeft(s({0x1E900}))); // Adlam letter
    }

    // The clusters give the source position of each cell
    void visualClustersKeepTheSourceIndex()
    {
        const auto clusters = QtTuiUnicode::visualClusters(s({'a', ALEF, 0x05B8, BET}));
        QCOMPARE(clusters.size(), 3);
        QCOMPARE(clusters.at(0).index, 0);
        QCOMPARE(clusters.at(1).base, BET);
        QCOMPARE(clusters.at(1).index, 3);
        QCOMPARE(clusters.at(2).base, ALEF);
        QCOMPARE(clusters.at(2).index, 1);
        QCOMPARE(clusters.at(2).length, 2);
    }

    // renderText() cuts the logical end, then reorders what stays
    void renderTextReorders()
    {
        ScreenBuffer buffer(8, 1);
        const QString text = s({ALEF, BET, GIMEL, DALET, HE});
        QCOMPARE(renderText(buffer, 1, 0, text, 5, Cell()), 5);
        QCOMPARE(row(buffer), s({' ', HE, DALET, GIMEL, BET, ALEF, ' ', ' '}));

        // 3 columns: dalet and he are cut, alef to gimel show reversed
        buffer = ScreenBuffer(8, 1);
        QCOMPARE(renderText(buffer, 1, 0, text, 3, Cell()), 3);
        QCOMPARE(row(buffer), s({' ', GIMEL, BET, ALEF, ' ', ' ', ' ', ' '}));
    }

    void renderTextKeepsLeftToRightText()
    {
        ScreenBuffer buffer(8, 1);
        QCOMPARE(renderText(buffer, 0, 0, QStringLiteral("abc (d)"), 8, Cell()), 7);
        QCOMPARE(row(buffer), QStringLiteral("abc (d) "));
    }

    void renderTextKeepsMarksAndWideClusters()
    {
        // alef with qamats, a wide emoji, bet: the emoji is a neutral
        // between R and R. Display order: bet, emoji, alef with its mark.
        ScreenBuffer buffer(6, 1);
        QCOMPARE(renderText(buffer, 0, 0, s({ALEF, 0x05B8, 0x1F600, BET}), 6, Cell()), 4);
        QCOMPARE(buffer.backCell(0, 0).ch, BET);
        QCOMPARE(buffer.backCell(1, 0).ch, char32_t(0x1F600));
        QCOMPARE(buffer.backCell(1, 0).width, quint8(2));
        QCOMPARE(buffer.backCell(3, 0).ch, ALEF);
        QCOMPARE(buffer.backCell(3, 0).combining, s({0x05B8}));
    }

    void renderTextWithUrlReorders()
    {
        ScreenBuffer buffer(4, 1);
        renderText(buffer, 0, 0, s({ALEF, BET}), 4, Cell(), QStringLiteral("https://example.org"));
        QCOMPARE(row(buffer), s({BET, ALEF, ' ', ' '}));
        QVERIFY(buffer.backCell(0, 0).link != 0);
        QCOMPARE(buffer.backCell(1, 0).link, buffer.backCell(0, 0).link);
    }

    // The ellipsis is the logical end of the text that stays: the left
    // end of a right-to-left line, the right end of a left-to-right line
    void renderTextTruncatedReorders()
    {
        ScreenBuffer buffer(6, 1);
        renderTextTruncated(buffer, 0, 0, s({ALEF, BET, GIMEL, DALET, HE}), 4, Cell());
        QCOMPARE(row(buffer), s({0x2026, GIMEL, BET, ALEF, ' ', ' '}));

        // Paragraph level 0. "ab alef bet gimel dalet" in 6 columns keeps
        // "ab alef bet" and the ellipsis. The ellipsis is between R and
        // the end of an L paragraph, so it is L (N2) and stays at the end.
        buffer = ScreenBuffer(6, 1);
        renderTextTruncated(buffer, 0, 0, s({'a', 'b', ' ', ALEF, BET, GIMEL, DALET}), 6, Cell());
        QCOMPARE(row(buffer), s({'a', 'b', ' ', BET, ALEF, 0x2026}));

        // Text that fits has no ellipsis
        buffer = ScreenBuffer(6, 1);
        renderTextTruncated(buffer, 0, 0, s({ALEF, BET}), 4, Cell());
        QCOMPARE(row(buffer), s({BET, ALEF, ' ', ' ', ' ', ' '}));
    }

private:
    // The characters of row 0, one for each cell
    static QString row(const ScreenBuffer &buffer)
    {
        QString out;
        for (int x = 0; x < buffer.width(); ++x)
            out.append(QChar::fromUcs4(buffer.backCell(x, 0).ch));
        return out;
    }
};

QTEST_GUILESS_MAIN(TestBidi)
#include "test_bidi.moc"
