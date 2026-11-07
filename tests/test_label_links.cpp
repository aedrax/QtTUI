// QTuiLabel links: [text](url) in Markdown and <a href> in rich text, the
// linkActivated() signal, the keys, the mouse and the paint.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiPushButton.h>
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

        // Tab and Shift+Tab, if they reach the label
        sendKey(label, Qt::Key_Tab);
        QCOMPARE(current(), QStringLiteral("2"));
        sendKey(label, Qt::Key_Tab);
        sendKey(label, Qt::Key_Backtab, Qt::ShiftModifier);
        sendKey(label, Qt::Key_Tab, Qt::ShiftModifier);
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
};

QTEST_APPLESS_MAIN(TestLabelLinks)
#include "test_label_links.moc"
