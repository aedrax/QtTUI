#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QSignalSpy>

using namespace QtTUI;

// Concrete widget for buddy testing
class BuddyWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit BuddyWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
};

class TestQTuiLabel : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // Text display
    // ---------------------------------------------------------------

    void defaultConstructor()
    {
        QTuiLabel label;
        QVERIFY(label.text().isEmpty());
        QCOMPARE(label.alignment(), Qt::AlignLeft | Qt::AlignVCenter);
        QVERIFY(!label.wordWrap());
        QVERIFY(label.buddy() == nullptr);
    }

    void constructorWithText()
    {
        QTuiLabel label(QStringLiteral("Hello"));
        QCOMPARE(label.text(), QStringLiteral("Hello"));
    }

    void setTextEmitsSignal()
    {
        QTuiLabel label;
        QSignalSpy spy(&label, &QTuiLabel::textChanged);

        label.setText(QStringLiteral("New text"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("New text"));
        QCOMPARE(label.text(), QStringLiteral("New text"));
    }

    void setTextSameValueNoSignal()
    {
        QTuiLabel label(QStringLiteral("Same"));
        QSignalSpy spy(&label, &QTuiLabel::textChanged);

        label.setText(QStringLiteral("Same"));
        QCOMPARE(spy.count(), 0);
    }

    // ---------------------------------------------------------------
    // Alignment
    // ---------------------------------------------------------------

    void alignmentDefaultIsLeftVCenter()
    {
        QTuiLabel label;
        QCOMPARE(label.alignment(), Qt::AlignLeft | Qt::AlignVCenter);
    }

    void setAlignmentRoundTrip()
    {
        QTuiLabel label;

        label.setAlignment(Qt::AlignRight | Qt::AlignTop);
        QCOMPARE(label.alignment(), Qt::AlignRight | Qt::AlignTop);

        label.setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        QCOMPARE(label.alignment(), Qt::AlignHCenter | Qt::AlignBottom);

        label.setAlignment(Qt::AlignCenter);
        QCOMPARE(label.alignment(), Qt::Alignment(Qt::AlignCenter));
    }

    void paintLeftAligned()
    {
        QTuiLabel label(QStringLiteral("Hi"));
        label.setGeometry(QRect(0, 0, 10, 1));
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);

        ScreenBuffer buffer(10, 1);
        label.paintEvent(buffer);

        QCOMPARE(buffer.backCell(0, 0).ch, U'H');
        QCOMPARE(buffer.backCell(1, 0).ch, U'i');
        // Rest should be spaces
        QCOMPARE(buffer.backCell(2, 0).ch, U' ');
    }

    void paintRightAligned()
    {
        QTuiLabel label(QStringLiteral("Hi"));
        label.setGeometry(QRect(0, 0, 10, 1));
        label.setAlignment(Qt::AlignRight | Qt::AlignTop);

        ScreenBuffer buffer(10, 1);
        label.paintEvent(buffer);

        // "Hi" is 2 chars, right-aligned in 10 cols => starts at col 8
        QCOMPARE(buffer.backCell(8, 0).ch, U'H');
        QCOMPARE(buffer.backCell(9, 0).ch, U'i');
        QCOMPARE(buffer.backCell(7, 0).ch, U' ');
    }

    void paintCenterAligned()
    {
        QTuiLabel label(QStringLiteral("Hi"));
        label.setGeometry(QRect(0, 0, 10, 1));
        label.setAlignment(Qt::AlignHCenter | Qt::AlignTop);

        ScreenBuffer buffer(10, 1);
        label.paintEvent(buffer);

        // "Hi" is 2 chars, center in 10 => starts at col 4
        QCOMPARE(buffer.backCell(4, 0).ch, U'H');
        QCOMPARE(buffer.backCell(5, 0).ch, U'i');
    }

    void paintVerticalTop()
    {
        QTuiLabel label(QStringLiteral("X"));
        label.setGeometry(QRect(0, 0, 5, 5));
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);

        ScreenBuffer buffer(5, 5);
        label.paintEvent(buffer);

        QCOMPARE(buffer.backCell(0, 0).ch, U'X');
        // Row 1 should be blank
        QCOMPARE(buffer.backCell(0, 1).ch, U' ');
    }

    void paintVerticalBottom()
    {
        QTuiLabel label(QStringLiteral("X"));
        label.setGeometry(QRect(0, 0, 5, 5));
        label.setAlignment(Qt::AlignLeft | Qt::AlignBottom);

        ScreenBuffer buffer(5, 5);
        label.paintEvent(buffer);

        // Single line, bottom-aligned in 5 rows => row 4
        QCOMPARE(buffer.backCell(0, 4).ch, U'X');
        QCOMPARE(buffer.backCell(0, 0).ch, U' ');
    }

    void paintVerticalCenter()
    {
        QTuiLabel label(QStringLiteral("X"));
        label.setGeometry(QRect(0, 0, 5, 5));
        label.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        ScreenBuffer buffer(5, 5);
        label.paintEvent(buffer);

        // Single line, vcenter in 5 rows => row 2
        QCOMPARE(buffer.backCell(0, 2).ch, U'X');
    }

    // ---------------------------------------------------------------
    // Word wrap
    // ---------------------------------------------------------------

    void wordWrapDefault()
    {
        QTuiLabel label;
        QVERIFY(!label.wordWrap());
    }

    void wordWrapRoundTrip()
    {
        QTuiLabel label;
        label.setWordWrap(true);
        QVERIFY(label.wordWrap());
        label.setWordWrap(false);
        QVERIFY(!label.wordWrap());
    }

    void wordWrapRendersMultipleLines()
    {
        QTuiLabel label(QStringLiteral("Hello World"));
        label.setWordWrap(true);
        label.setGeometry(QRect(0, 0, 6, 3));
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);

        ScreenBuffer buffer(6, 3);
        label.paintEvent(buffer);

        // "Hello" on row 0, "World" on row 1
        QCOMPARE(buffer.backCell(0, 0).ch, U'H');
        QCOMPARE(buffer.backCell(4, 0).ch, U'o');
        QCOMPARE(buffer.backCell(0, 1).ch, U'W');
        QCOMPARE(buffer.backCell(4, 1).ch, U'd');
    }

    void noWrapTruncatesText()
    {
        QTuiLabel label(QStringLiteral("Hello World"));
        label.setWordWrap(false);
        label.setGeometry(QRect(0, 0, 6, 1));
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);

        ScreenBuffer buffer(6, 1);
        label.paintEvent(buffer);

        // Without wrap, only first 6 chars rendered
        QCOMPARE(buffer.backCell(0, 0).ch, U'H');
        QCOMPARE(buffer.backCell(5, 0).ch, U' ');
    }

    // ---------------------------------------------------------------
    // Buddy association
    // ---------------------------------------------------------------

    void buddyDefault()
    {
        QTuiLabel label;
        QVERIFY(label.buddy() == nullptr);
    }

    void setBuddyRoundTrip()
    {
        QTuiLabel label;
        BuddyWidget buddy;

        label.setBuddy(&buddy);
        QCOMPARE(label.buddy(), &buddy);

        label.setBuddy(nullptr);
        QVERIFY(label.buddy() == nullptr);
    }

    // ---------------------------------------------------------------
    // minimumSize
    // ---------------------------------------------------------------

    void minimumSizeNoWrap()
    {
        QTuiLabel label(QStringLiteral("Hello"));
        QCOMPARE(label.minimumSize(), QSize(5, 1));
    }

    void minimumSizeEmpty()
    {
        QTuiLabel label;
        QCOMPARE(label.minimumSize(), QSize(0, 1));
    }

    void minimumSizeMultiLine()
    {
        QTuiLabel label(QStringLiteral("Line 1\nLonger line 2\nL3"));
        QCOMPARE(label.minimumSize(), QSize(13, 3));
        label.setWordWrap(true);
        // Minimum width = longest word ("Longer" = 6)
        QCOMPARE(label.minimumSize(), QSize(6, 3));
    }

    void paintMultiLine()
    {
        QTuiLabel label(QStringLiteral("ab\ncd"));
        label.setGeometry(QRect(0, 0, 4, 2));
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);

        ScreenBuffer buffer(4, 2);
        label.paintEvent(buffer);

        QCOMPARE(buffer.backCell(0, 0).ch, U'a');
        QCOMPARE(buffer.backCell(1, 0).ch, U'b');
        QCOMPARE(buffer.backCell(2, 0).ch, U' ');
        QCOMPARE(buffer.backCell(0, 1).ch, U'c');
        QCOMPARE(buffer.backCell(1, 1).ch, U'd');
    }

    void minimumSizeWithWrap()
    {
        QTuiLabel label(QStringLiteral("Hello World"));
        label.setWordWrap(true);
        // Minimum width = longest word ("Hello" = 5)
        QCOMPARE(label.minimumSize().width(), 5);
    }

    // ---------------------------------------------------------------
    // Property-based tests with rapidcheck
    // ---------------------------------------------------------------
    void textRoundTripProperty()
    {
        rc::check("setText(t) then text() returns t, textChanged emitted",
            []() {
                QTuiLabel label;
                // Generate a non-empty ASCII string
                const auto stdStr = *rc::gen::nonEmpty(
                    rc::gen::container<std::string>(rc::gen::inRange<char>(32, 127)));
                QString text = QString::fromStdString(stdStr);

                QSignalSpy spy(&label, &QTuiLabel::textChanged);
                label.setText(text);

                RC_ASSERT(label.text() == text);
                RC_ASSERT(spy.count() == 1);
                RC_ASSERT(spy.at(0).at(0).toString() == text);
            });
    }

    void alignmentRoundTripProperty()
    {
        rc::check("setAlignment(a) then alignment() returns a",
            []() {
                QTuiLabel label;

                // Generate valid alignment combinations
                const auto hAligns = std::vector<Qt::AlignmentFlag>{
                    Qt::AlignLeft, Qt::AlignHCenter, Qt::AlignRight};
                const auto vAligns = std::vector<Qt::AlignmentFlag>{
                    Qt::AlignTop, Qt::AlignVCenter, Qt::AlignBottom};

                const auto hIdx = *rc::gen::inRange<size_t>(0, hAligns.size());
                const auto vIdx = *rc::gen::inRange<size_t>(0, vAligns.size());

                Qt::Alignment align = hAligns[hIdx] | vAligns[vIdx];
                label.setAlignment(align);
                RC_ASSERT(label.alignment() == align);
            });
    }

    void wordWrapRoundTripProperty()
    {
        rc::check("setWordWrap(v) then wordWrap() returns v",
            []() {
                QTuiLabel label;
                const auto wrap = *rc::gen::arbitrary<bool>();
                label.setWordWrap(wrap);
                RC_ASSERT(label.wordWrap() == wrap);
            });
    }

    void wordWrapNeverExceedsWidthProperty()
    {
        rc::check("With word wrap, no rendered line exceeds widget width",
            []() {
                const auto width = *rc::gen::inRange(3, 40);
                const auto height = *rc::gen::inRange(1, 20);

                // Generate text with multiple words
                const auto wordCount = *rc::gen::inRange(1, 10);
                QStringList words;
                for (int i = 0; i < wordCount; ++i) {
                    auto stdWord = *rc::gen::nonEmpty(
                        rc::gen::container<std::string>(
                            rc::gen::inRange<char>('a', 'z' + 1)));
                    QString word = QString::fromStdString(stdWord);
                    // Limit word length to something reasonable
                    if (word.length() > 20)
                        word = word.left(20);
                    words.append(word);
                }
                QString text = words.join(QLatin1Char(' '));

                QTuiLabel label(text);
                label.setWordWrap(true);
                label.setGeometry(QRect(0, 0, width, height));
                label.setAlignment(Qt::AlignLeft | Qt::AlignTop);

                ScreenBuffer buffer(width, height);
                label.paintEvent(buffer);

                // Check each row: non-space characters should not extend beyond width
                for (int row = 0; row < height; ++row) {
                    int lastNonSpace = -1;
                    for (int col = 0; col < width; ++col) {
                        if (buffer.backCell(col, row).ch != U' ')
                            lastNonSpace = col;
                    }
                    RC_ASSERT(lastNonSpace < width);
                }
            });
    }
};

QTEST_MAIN(TestQTuiLabel)
#include "test_qtui_label.moc"
