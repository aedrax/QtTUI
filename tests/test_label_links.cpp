// QTuiLabel links: [text](url) in Markdown and <a href> in rich text, the
// linkActivated() signal, the keys, the mouse and the paint.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

static void click(QTuiWidget &w, int x, int y, Qt::MouseButton button = Qt::LeftButton)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, button, QPoint(x, y));
    QCoreApplication::sendEvent(&w, &ev);
}

static const QString kHtml = QStringLiteral(
    "Go <a href=\"https://Example.com/Path\">here</a> or <A HREF='two'>there</A>");

class TestLabelLinks : public QObject {
    Q_OBJECT

    int m_argc = 1;
    char m_arg0[5] = "test";
    char *m_argv[2] = {m_arg0, nullptr};
    QTuiApplication *m_app = nullptr;

private slots:
    // Each test has its own application, for the events and the main window
    void init() { m_app = new QTuiApplication(m_argc, m_argv); }
    void cleanup() { delete m_app; }

    void markdownLink()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        label.setText(QStringLiteral("See [the *docs*](https://example.com/a_b_c) now"));
        QCOMPARE(label.displayText(), QStringLiteral("See the docs now"));

        // The underscores of the url are not markup
        QSignalSpy spy(&label, &QTuiLabel::linkActivated);
        sendKey(label, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("https://example.com/a_b_c"));
    }

    void bracketsThatAreNotALink()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        label.setText(QStringLiteral("[x] and a[0](1 and [](u)"));
        QCOMPARE(label.displayText(), QStringLiteral("[x] and a[0](1 and [](u)"));
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);

        // A link must close on its line
        label.setText(QStringLiteral("[a](b\n)"));
        QCOMPARE(label.displayText(), QStringLiteral("[a](b\n)"));

        // Plain text has no links
        label.setTextFormat(Qt::PlainText);
        label.setText(QStringLiteral("[a](b)"));
        QCOMPARE(label.displayText(), QStringLiteral("[a](b)"));
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);
    }

    void htmlLinksKeepTheCaseOfTheTarget()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::RichText);
        label.setText(kHtml);
        QCOMPARE(label.displayText(), QStringLiteral("Go here or there"));

        QSignalSpy spy(&label, &QTuiLabel::linkActivated);
        sendKey(label, Qt::Key_Enter);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("https://Example.com/Path"));
        sendKey(label, Qt::Key_Right);
        sendKey(label, Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("two"));

        // An <a> without href is not a link
        label.setText(QStringLiteral("<a name=\"x\">anchor</a>"));
        QCOMPARE(label.displayText(), QStringLiteral("anchor"));
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);
    }

    void labelWithLinksTakesTheFocus()
    {
        QTuiLabel label(QStringLiteral("plain"));
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);
        label.setTextFormat(Qt::RichText);
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);
        label.setText(kHtml);
        QCOMPARE(label.focusPolicy(), Qt::StrongFocus);
        label.setText(QStringLiteral("no <b>links</b>"));
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);
        label.setText(kHtml);
        label.setTextFormat(Qt::PlainText);
        QCOMPARE(label.focusPolicy(), Qt::NoFocus);
    }

    void keysMoveBetweenTheLinks()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        label.setText(QStringLiteral("[a](1) [b](2) [c](3)"));
        QSignalSpy spy(&label, &QTuiLabel::linkActivated);
        auto current = [&]() {
            sendKey(label, Qt::Key_Return);
            return spy.last().at(0).toString();
        };
        QCOMPARE(current(), QStringLiteral("1"));

        label.clearDirtyRegions();
        sendKey(label, Qt::Key_Right);
        QVERIFY(label.isDirty());
        QCOMPARE(current(), QStringLiteral("2"));
        sendKey(label, Qt::Key_Down);
        QCOMPARE(current(), QStringLiteral("3"));

        // The last link: the key changes nothing
        label.clearDirtyRegions();
        sendKey(label, Qt::Key_Right);
        QVERIFY(!label.isDirty());
        QCOMPARE(current(), QStringLiteral("3"));

        sendKey(label, Qt::Key_Left);
        QCOMPARE(current(), QStringLiteral("2"));
        sendKey(label, Qt::Key_Up);
        QCOMPARE(current(), QStringLiteral("1"));
        sendKey(label, Qt::Key_Left);
        QCOMPARE(current(), QStringLiteral("1"));

        // A new text starts at the first link
        sendKey(label, Qt::Key_Right);
        label.setText(QStringLiteral("[x](7) [y](8)"));
        QCOMPARE(current(), QStringLiteral("7"));
    }

    void paintShowsLinksUnderlinedInTheAccentColor()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::RichText);
        label.setText(kHtml); // "Go here or there"
        label.setGeometry(QRect(0, 0, 20, 1));
        const ThemeProperties theme = label.themeProperties();

        ScreenBuffer buffer(20, 1);
        label.paintEvent(buffer);
        QCOMPARE(buffer.backCell(3, 0).ch, U'h');
        for (int x : {3, 4, 5, 6, 11, 15}) {
            QVERIFY2(buffer.backCell(x, 0).underline, qPrintable(QString::number(x)));
            QCOMPARE(buffer.backCell(x, 0).foreground, theme.focusBackground);
            QCOMPARE(buffer.backCell(x, 0).background, theme.background);
        }
        for (int x : {0, 1, 2, 7, 8, 16}) {
            QVERIFY2(!buffer.backCell(x, 0).underline, qPrintable(QString::number(x)));
            QCOMPARE(buffer.backCell(x, 0).foreground, theme.foreground);
        }

        // With focus, the current link shows in the focus colors. The other
        // text keeps its colors.
        label.setFocus();
        QVERIFY(label.hasFocus());
        label.paintEvent(buffer);
        QCOMPARE(buffer.backCell(3, 0).background, theme.focusBackground);
        QCOMPARE(buffer.backCell(3, 0).foreground, theme.focusForeground);
        QVERIFY(buffer.backCell(3, 0).underline);
        QCOMPARE(buffer.backCell(11, 0).background, theme.background);
        QCOMPARE(buffer.backCell(0, 0).background, theme.background);

        sendKey(label, Qt::Key_Right);
        label.paintEvent(buffer);
        QCOMPARE(buffer.backCell(3, 0).background, theme.background);
        QCOMPARE(buffer.backCell(11, 0).background, theme.focusBackground);
    }

    void spaceInALinkIsPartOfTheLink()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        label.setText(QStringLiteral("[click here](target) x"));
        label.setGeometry(QRect(0, 0, 20, 1));
        ScreenBuffer buffer(20, 1);
        label.paintEvent(buffer);
        QVERIFY(buffer.backCell(5, 0).underline);   // the space in the link
        QVERIFY(!buffer.backCell(10, 0).underline); // the space after it

        QSignalSpy spy(&label, &QTuiLabel::linkActivated);
        click(label, 5, 0);
        QCOMPARE(spy.count(), 1);
        click(label, 10, 0);
        QCOMPARE(spy.count(), 1);
    }

    void leftClickActivatesTheLink()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::RichText);
        label.setText(kHtml); // "Go here or there"
        label.setGeometry(QRect(2, 1, 30, 1));
        QSignalSpy spy(&label, &QTuiLabel::linkActivated);

        click(label, 2 + 0, 1); // "Go"
        click(label, 2 + 7, 1); // the space after "here"
        click(label, 2 + 4, 2); // a different row
        click(label, 2 + 4, 1, Qt::RightButton);
        QCOMPARE(spy.count(), 0);

        click(label, 2 + 4, 1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("https://Example.com/Path"));

        // A click makes the link the current link
        label.clearDirtyRegions();
        click(label, 2 + 15, 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("two"));
        QVERIFY(label.isDirty());
        sendKey(label, Qt::Key_Return);
        QCOMPARE(spy.count(), 3);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("two"));
    }

    void clickUsesTheWrapAndTheAlignment()
    {
        QTuiLabel label;
        label.setTextFormat(Qt::RichText);
        label.setText(kHtml);
        label.setWordWrap(true);
        label.setAlignment(Qt::AlignRight | Qt::AlignTop);
        // Rows: " Go here" and "or there", in 8 columns
        label.setGeometry(QRect(0, 0, 8, 3));
        ScreenBuffer buffer(8, 3);
        label.paintEvent(buffer);
        QCOMPARE(buffer.backCell(4, 0).ch, U'h');
        QCOMPARE(buffer.backCell(3, 1).ch, U't');

        QSignalSpy spy(&label, &QTuiLabel::linkActivated);
        click(label, 4, 0);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("https://Example.com/Path"));
        click(label, 3, 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("two"));
        click(label, 0, 1);
        QCOMPARE(spy.count(), 2);
    }

    void keysInAMainWindow()
    {
        QTuiMainWindow window;
        window.setGeometry(QRect(0, 0, 80, 24));
        window.setVisible(true);
        m_app->setMainWindow(&window);

        auto *button = new QTuiPushButton(QStringLiteral("Go"), &window);
        button->setGeometry(QRect(0, 0, 10, 3));
        auto *label = new QTuiLabel(QString(), &window);
        label->setTextFormat(Qt::MarkdownText);
        label->setText(QStringLiteral("[one](1) [two](2)"));
        label->setGeometry(QRect(0, 3, 20, 1));
        QSignalSpy spy(label, &QTuiLabel::linkActivated);

        auto key = [&](int k) {
            TuiKeyEvent ev(k, Qt::NoModifier);
            QCoreApplication::sendEvent(&window, &ev);
        };
        button->setFocus();
        key(Qt::Key_Tab);
        QVERIFY(label->hasFocus());
        key(Qt::Key_Right);
        key(Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("2"));

        // In a window, Tab goes to the next widget, not to the next link
        key(Qt::Key_Left);
        key(Qt::Key_Tab);
        QVERIFY(button->hasFocus());

        // A click in the window gives the focus and activates the link
        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(6, 3));
        QCoreApplication::sendEvent(&window, &press);
        QVERIFY(label->hasFocus());
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("2"));
    }

    void linkWithASchemeIsATerminalHyperlink()
    {
        QCOMPARE(QTuiLabel::linkScheme(QStringLiteral("HTTPS://x.example")), QStringLiteral("https"));
        QCOMPARE(QTuiLabel::linkScheme(QStringLiteral("mailto:a@x.example")), QStringLiteral("mailto"));
        QCOMPARE(QTuiLabel::linkScheme(QStringLiteral("file:///tmp/a.md")), QStringLiteral("file"));
        for (const char *internal : {"#section", "settings", "dir/page.md", "/abs/page.md",
                                     "C:\\dir\\page.md", "a b:c", ":x", "1x:y", ""})
            QVERIFY2(QTuiLabel::linkScheme(QString::fromLatin1(internal)).isEmpty(), internal);

        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        // "web mail top"
        label.setText(QStringLiteral("[web](https://x.example/a) [mail](mailto:a@x.example) [top](#top)"));
        label.setGeometry(QRect(0, 0, 20, 1));
        ScreenBuffer buffer(20, 1);
        label.paintEvent(buffer);
        auto url = [&](int x) { return buffer.linkUrl(buffer.backCell(x, 0).link); };
        for (int x : {0, 1, 2})
            QCOMPARE(url(x), QStringLiteral("https://x.example/a"));
        for (int x : {4, 7})
            QCOMPARE(url(x), QStringLiteral("mailto:a@x.example"));
        // The spaces between the links and the internal link have no id
        for (int x : {3, 8, 9, 10, 11, 12})
            QCOMPARE(buffer.backCell(x, 0).link, quint16(0));
        QVERIFY(buffer.backCell(9, 0).underline); // "#top" is still a link of the label
    }

    void hyperlinkStaysCorrectInAScrollArea()
    {
        // The scroll area paints its content into a different buffer, and
        // a link id is valid only in the buffer that gave it
        QTuiScrollArea area;
        area.setGeometry(QRect(0, 0, 20, 2));
        auto *label = new QTuiLabel;
        label->setTextFormat(Qt::MarkdownText);
        label->setText(QStringLiteral("[web](https://x.example/a)"));
        label->setGeometry(QRect(0, 0, 20, 1));
        area.setWidget(label);

        ScreenBuffer buffer(20, 2);
        // The screen buffer has other links already, so the ids differ
        buffer.linkId(QStringLiteral("https://other.example/1"));
        buffer.linkId(QStringLiteral("https://other.example/2"));
        area.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).ch, U'w');
        QCOMPARE(buffer.linkUrl(buffer.backCell(0, 0).link), QStringLiteral("https://x.example/a"));
        QCOMPARE(buffer.backCell(5, 0).link, quint16(0));
    }

    void aLabelCanHaveManyLinks()
    {
        // 300 links, one on each line: "L0" to "L299"
        QString text;
        for (int i = 0; i < 300; ++i)
            text += QStringLiteral("[L%1](t%1)\n").arg(i);
        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);
        label.setText(text);
        label.setGeometry(QRect(0, 0, 10, 300));

        QSignalSpy spy(&label, &QTuiLabel::linkActivated);
        click(label, 0, 280);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("t280"));
        QCOMPARE(label.currentLink(), 280);
        QCOMPARE(label.linkRect(299), QRect(0, 299, 4, 1));
    }

    void currentLinkAndLinkRect()
    {
        QTuiLabel label;
        QCOMPARE(label.currentLink(), -1);
        label.setTextFormat(Qt::RichText);
        label.setText(kHtml); // "Go here or there"
        label.setWordWrap(true);
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);
        // Rows: "Go here" and "or there"
        label.setGeometry(QRect(2, 1, 8, 2));
        QCOMPARE(label.currentLink(), 0);
        QCOMPARE(label.linkRect(0), QRect(2 + 3, 1, 4, 1));
        QCOMPARE(label.linkRect(1), QRect(2 + 3, 2, 5, 1));
        QVERIFY(label.linkRect(2).isEmpty());
        sendKey(label, Qt::Key_Right);
        QCOMPARE(label.currentLink(), 1);
    }

    void heightForWidthCountsTheWrappedLines()
    {
        QTuiLabel label(QStringLiteral("one two three\nfour"));
        QCOMPARE(label.heightForWidth(20), 2);
        label.setWordWrap(true);
        QCOMPARE(label.heightForWidth(20), 2);
        QCOMPARE(label.heightForWidth(7), 3); // "one two", "three", "four"
        QCOMPARE(label.heightForWidth(5), 4);
        label.setFrameShape(QTuiFrame::Box);
        QCOMPARE(label.heightForWidth(7 + 2), 3 + 2);
        QCOMPARE(QTuiLabel().heightForWidth(10), 1);
    }

    // The label keeps the parsed text and the lines. Each change that
    // changes them must show in the next paint.
    void paintFollowsEachChange()
    {
        QTuiLabel label(QStringLiteral("one two"));
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);
        label.setGeometry(QRect(0, 0, 8, 2));
        ScreenBuffer buffer(8, 2);
        auto row = [&](int y) {
            label.paintEvent(buffer);
            QString s;
            for (int x = 0; x < buffer.width(); ++x)
                s += buffer.backCell(x, y).text();
            return s.trimmed();
        };
        QCOMPARE(row(0), QStringLiteral("one two"));

        label.setText(QStringLiteral("a *b* three"));
        QCOMPARE(row(0), QStringLiteral("a *b* th"));
        label.setTextFormat(Qt::MarkdownText);
        QCOMPARE(row(0), QStringLiteral("a b thre"));
        QVERIFY(buffer.backCell(2, 0).italic);
        label.setWordWrap(true);
        QCOMPARE(row(0), QStringLiteral("a b"));
        QCOMPARE(row(1), QStringLiteral("three"));
        // A new width wraps again
        label.setGeometry(QRect(0, 0, 3, 2));
        QCOMPARE(row(0), QStringLiteral("a b"));
        label.setGeometry(QRect(0, 0, 1, 2));
        label.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).ch, U'a');
        QCOMPARE(buffer.backCell(0, 1).ch, U'b');
        QVERIFY(buffer.backCell(0, 1).italic);

        // The buddy changes how the label reads &
        label.setGeometry(QRect(0, 0, 8, 2));
        label.setTextFormat(Qt::PlainText);
        label.setText(QStringLiteral("&Name"));
        QCOMPARE(row(0), QStringLiteral("&Name"));
        auto *buddy = new QTuiPushButton(QStringLiteral("x"));
        label.setBuddy(buddy);
        QCOMPARE(row(0), QStringLiteral("Name"));
        QCOMPARE(label.mnemonic(), QChar(QLatin1Char('N')));
        delete buddy;
        QCOMPARE(row(0), QStringLiteral("&Name"));
        QCOMPARE(label.displayText(), QStringLiteral("&Name"));
    }

    void renderPaintsAPartOfTheLabel()
    {
        // 100 lines: "row 0" to "row 99"
        QString text;
        for (int i = 0; i < 100; ++i)
            text += QStringLiteral("row [%1](t%1)\n").arg(i);
        QTuiLabel label;
        label.setTextFormat(Qt::MarkdownText);
        label.setAlignment(Qt::AlignLeft | Qt::AlignTop);
        label.setText(text);
        label.setGeometry(QRect(0, 0, 10, 100));
        const ThemeProperties theme = label.themeProperties();

        // Rows 40 to 42 of the label show in the rows 1 to 3 at column 2
        ScreenBuffer buffer(14, 5);
        const Cell before = buffer.backCell(0, 0);
        label.render(buffer, QPoint(2, 1 - 40), QRect(2, 1, 10, 3), true);
        auto text3 = [&](int y) {
            QString s;
            for (int x = 6; x < 9; ++x)
                s += buffer.backCell(x, y).text();
            return s.trimmed();
        };
        QCOMPARE(text3(1), QStringLiteral("40"));
        QCOMPARE(text3(3), QStringLiteral("42"));
        QVERIFY(buffer.backCell(6, 1).underline);
        // No cell outside the clip changes
        for (int x = 0; x < 14; ++x) {
            QCOMPARE(buffer.backCell(x, 0), before);
            QCOMPARE(buffer.backCell(x, 4), before);
        }
        QCOMPARE(buffer.backCell(1, 2), before);
        QCOMPARE(buffer.backCell(12, 2), before);

        // showLinkFocus shows the current link in the focus colors
        click(label, 4, 41);
        label.render(buffer, QPoint(2, 1 - 40), QRect(2, 1, 10, 3), true);
        QCOMPARE(buffer.backCell(6, 2).background, theme.focusBackground);
        QCOMPARE(buffer.backCell(6, 1).background, theme.background);
        label.render(buffer, QPoint(2, 1 - 40), QRect(2, 1, 10, 3), false);
        QCOMPARE(buffer.backCell(6, 2).background, theme.background);
    }
};

QTEST_APPLESS_MAIN(TestLabelLinks)
#include "test_label_links.moc"
