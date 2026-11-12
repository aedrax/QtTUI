// QTuiTextBrowser: the text formats, the wrap at the viewport width, the
// scroll keys, the links, setSource(), the history and the paint.
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/QTuiTextBrowser.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

static void sendText(QTuiWidget &w, char ch)
{
    sendKey(w, Qt::Key_A + (QChar::toUpper(uint(ch)) - 'A'),
            QChar::isUpper(uint(ch)) ? Qt::ShiftModifier : Qt::NoModifier, QString(QLatin1Char(ch)));
}

static void click(QTuiWidget &w, int x, int y)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, QPoint(x, y));
    QCoreApplication::sendEvent(&w, &ev);
}

// The text of one row of the back buffer, without the spaces at the end
static QString rowText(const ScreenBuffer &buffer, int y, int from = 0, int to = -1)
{
    QString s;
    for (int x = from; x < (to < 0 ? buffer.width() : to); ++x)
        s += buffer.backCell(x, y).text();
    while (s.endsWith(QLatin1Char(' ')))
        s.chop(1);
    return s;
}

// "line 0\nline 1\n..." without a line break at the end
static QString numberedLines(int count)
{
    QStringList lines;
    for (int i = 0; i < count; ++i)
        lines << QStringLiteral("line %1").arg(i);
    return lines.join(QLatin1Char('\n'));
}

class TestQTuiTextBrowser : public QObject {
    Q_OBJECT

    int m_argc = 1;
    char m_arg0[5] = "test";
    char *m_argv[2] = {m_arg0, nullptr};
    QTuiApplication *m_app = nullptr;
    QTemporaryDir m_dir;

    QString write(const QString &name, const QByteArray &content)
    {
        const QString path = m_dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            qFatal("cannot write %s", qPrintable(path));
        file.write(content);
        return path;
    }

private slots:
    // Each test has its own application, for the events and the main window
    void init() { m_app = new QTuiApplication(m_argc, m_argv); }
    void cleanup() { delete m_app; }

    void textFormats()
    {
        QTuiTextBrowser browser;
        QCOMPARE(browser.toPlainText(), QString());
        QVERIFY(browser.openLinks());
        QCOMPARE(browser.source(), QString());

        browser.setMarkdown(QStringLiteral("a **b** [c](d)"));
        QCOMPARE(browser.toPlainText(), QStringLiteral("a b c"));
        browser.setHtml(QStringLiteral("a <b>b</b> <a href=\"d\">c</a>"));
        QCOMPARE(browser.toPlainText(), QStringLiteral("a b c"));
        browser.setPlainText(QStringLiteral("a **b** <b>"));
        QCOMPARE(browser.toPlainText(), QStringLiteral("a **b** <b>"));
    }

    void widthFollowsTheViewport()
    {
        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 12, 3));
        browser.setPlainText(QStringLiteral("one two three four five six"));
        // 12 columns give "one two", "three four", "five six": that fits,
        // so there is no scroll bar
        QCOMPARE(browser.widget()->geometry(), QRect(0, 0, 12, 3));
        QVERIFY(!browser.verticalScrollBar()->isVisible());

        // 10 columns give 3 lines, but only 2 rows show: one column is for
        // the scroll bar, and 9 columns give 4 lines
        browser.clearDirtyRegions();
        browser.setGeometry(QRect(0, 0, 10, 2));
        QCOMPARE(browser.widget()->geometry(), QRect(0, 0, 9, 4));
        QVERIFY(browser.verticalScrollBar()->isVisible());
        QVERIFY(!browser.horizontalScrollBar()->isVisible());
        QCOMPARE(browser.verticalScrollBar()->maximum(), 2);
        QVERIFY(browser.isDirty());

        ScreenBuffer buffer(10, 2);
        browser.paintEvent(buffer);
        QCOMPARE(rowText(buffer, 0, 0, 9), QStringLiteral("one two"));
        QCOMPARE(rowText(buffer, 1, 0, 9), QStringLiteral("three"));

        // New text starts at the top
        sendKey(browser, Qt::Key_End);
        QCOMPARE(browser.scrollOffset(), 2);
        browser.setPlainText(numberedLines(10));
        QCOMPARE(browser.scrollOffset(), 0);
        QCOMPARE(browser.verticalScrollBar()->maximum(), 8);

        // Also from the end of a long text to a shorter text
        browser.setPlainText(numberedLines(100));
        sendKey(browser, Qt::Key_End);
        QCOMPARE(browser.scrollOffset(), 98);
        browser.setPlainText(numberedLines(10));
        QCOMPARE(browser.scrollOffset(), 0);
        QCOMPARE(browser.verticalScrollBar()->value(), 0);
        browser.paintEvent(buffer);
        QCOMPARE(rowText(buffer, 0, 0, 9), QStringLiteral("line 0"));
        sendKey(browser, Qt::Key_Down);
        QCOMPARE(browser.scrollOffset(), 1);
        // And to a text that needs no scroll bar
        sendKey(browser, Qt::Key_End);
        browser.setPlainText(QStringLiteral("short"));
        QCOMPARE(browser.scrollOffset(), 0);
        browser.setPlainText(numberedLines(10));
        QCOMPARE(browser.scrollOffset(), 0);
        browser.paintEvent(buffer);
        QCOMPARE(rowText(buffer, 0, 0, 9), QStringLiteral("line 0"));
    }

    void keysAndWheelScroll()
    {
        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 20, 5));
        browser.setPlainText(numberedLines(100));
        ScreenBuffer buffer(20, 5);
        auto top = [&]() {
            browser.paintEvent(buffer);
            return rowText(buffer, 0, 0, 19);
        };
        QCOMPARE(top(), QStringLiteral("line 0"));

        browser.clearDirtyRegions();
        sendKey(browser, Qt::Key_Down);
        QVERIFY(browser.isDirty());
        QCOMPARE(top(), QStringLiteral("line 1"));
        sendText(browser, 'j');
        QCOMPARE(top(), QStringLiteral("line 2"));
        sendText(browser, 'k');
        sendKey(browser, Qt::Key_Up);
        QCOMPARE(top(), QStringLiteral("line 0"));

        sendKey(browser, Qt::Key_PageDown);
        QCOMPARE(top(), QStringLiteral("line 5"));
        sendKey(browser, Qt::Key_F, Qt::ControlModifier);
        QCOMPARE(top(), QStringLiteral("line 10"));
        sendKey(browser, Qt::Key_B, Qt::ControlModifier);
        sendKey(browser, Qt::Key_PageUp);
        QCOMPARE(top(), QStringLiteral("line 0"));

        sendKey(browser, Qt::Key_End);
        QCOMPARE(top(), QStringLiteral("line 95"));
        QCOMPARE(rowText(buffer, 4, 0, 19), QStringLiteral("line 99"));
        sendKey(browser, Qt::Key_Home);
        QCOMPARE(top(), QStringLiteral("line 0"));
        sendText(browser, 'G');
        QCOMPARE(top(), QStringLiteral("line 95"));
        browser.clearDirtyRegions();
        sendText(browser, 'g');
        QVERIFY(browser.isDirty());
        QCOMPARE(top(), QStringLiteral("line 0"));

        // The wheel scrolls 3 rows
        TuiMouseEvent wheel(TuiMouseEvent::Wheel, Qt::NoButton, QPoint(2, 2), Qt::NoModifier, -120);
        QCoreApplication::sendEvent(&browser, &wheel);
        QCOMPARE(top(), QStringLiteral("line 3"));
    }

    void keysMoveBetweenTheLinks()
    {
        // Links on the lines 0, 1 and 30
        QStringList lines;
        for (int i = 0; i < 40; ++i)
            lines << QStringLiteral("line %1").arg(i);
        lines[0] = QStringLiteral("[first](one)");
        lines[1] = QStringLiteral("[second](two)");
        lines[30] = QStringLiteral("to the [third](three)");

        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 20, 5));
        browser.setMarkdown(lines.join(QLatin1Char('\n')));
        const ThemeProperties theme = browser.widget()->themeProperties();
        QSignalSpy spy(&browser, &QTuiTextBrowser::anchorClicked);
        browser.setOpenLinks(false);
        ScreenBuffer buffer(20, 5);

        // Without the focus, no link shows as the current link
        browser.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).background, theme.background);
        QVERIFY(buffer.backCell(0, 0).underline);

        browser.setFocus();
        QVERIFY(browser.hasFocus());
        browser.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).background, theme.focusBackground);

        sendKey(browser, Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("one"));

        browser.clearDirtyRegions();
        sendKey(browser, Qt::Key_Right);
        QVERIFY(browser.isDirty());
        browser.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 0).background, theme.background);
        QCOMPARE(buffer.backCell(0, 1).background, theme.focusBackground);
        QCOMPARE(browser.scrollOffset(), 0);

        // The third link is not in view: the browser scrolls to it
        sendKey(browser, Qt::Key_Right);
        QCOMPARE(browser.scrollOffset(), 26);
        browser.paintEvent(buffer);
        QCOMPARE(rowText(buffer, 4, 0, 19), QStringLiteral("to the third"));
        QCOMPARE(buffer.backCell(7, 4).background, theme.focusBackground);
        sendKey(browser, Qt::Key_Enter);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("three"));
        // The last link: Right stays
        sendKey(browser, Qt::Key_Right);
        sendKey(browser, Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("three"));

        sendKey(browser, Qt::Key_Left);
        QCOMPARE(browser.scrollOffset(), 1);
        sendKey(browser, Qt::Key_Left);
        QCOMPARE(browser.scrollOffset(), 0);
        sendKey(browser, Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("one"));
        QCOMPARE(spy.count(), 4);
    }

    void clickFollowsALink()
    {
        QTuiTextBrowser browser;
        browser.setGeometry(QRect(3, 2, 20, 5));
        browser.setOpenLinks(false);
        QStringList lines;
        for (int i = 0; i < 40; ++i)
            lines << QStringLiteral("x [link %1](target%1)").arg(i);
        browser.setMarkdown(lines.join(QLatin1Char('\n')));
        QSignalSpy spy(&browser, &QTuiTextBrowser::anchorClicked);

        click(browser, 3 + 0, 2); // the "x"
        QCOMPARE(spy.count(), 0);
        click(browser, 3 + 4, 2 + 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("target1"));
        // The click uses the scroll position
        sendKey(browser, Qt::Key_PageDown);
        browser.clearDirtyRegions();
        click(browser, 3 + 4, 2 + 1);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("target6"));
        QVERIFY(browser.isDirty());
        // Enter then activates the link of the click
        sendKey(browser, Qt::Key_Return);
        QCOMPARE(spy.count(), 3);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("target6"));
    }

    void theBrowserKeepsTheFocus()
    {
        QTuiMainWindow window;
        window.setGeometry(QRect(0, 0, 40, 10));
        m_app->setMainWindow(&window);
        auto *before = new QTuiPushButton(QStringLiteral("Before"), &window);
        before->setGeometry(QRect(0, 0, 10, 1));
        auto *browser = new QTuiTextBrowser(&window);
        browser->setGeometry(QRect(0, 1, 40, 5));
        auto *after = new QTuiPushButton(QStringLiteral("After"), &window);
        after->setGeometry(QRect(0, 7, 10, 1));
        browser->setOpenLinks(false);
        browser->setMarkdown(QStringLiteral("[one](1) and [two](2)"));
        QSignalSpy spy(browser, &QTuiTextBrowser::anchorClicked);

        auto key = [&](int k, Qt::KeyboardModifiers mods = Qt::NoModifier) {
            TuiKeyEvent ev(k, mods);
            QCoreApplication::sendEvent(&window, &ev);
        };
        // The label with links is not a Tab stop
        QCOMPARE(browser->widget()->focusPolicy(), Qt::NoFocus);
        before->setFocus();
        key(Qt::Key_Tab);
        QVERIFY(browser->hasFocus());

        // Tab goes to the next link, and after the last link to the next
        // widget
        key(Qt::Key_Tab);
        QVERIFY(browser->hasFocus());
        key(Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("2"));
        key(Qt::Key_Tab);
        QVERIFY(after->hasFocus());

        // Shift+Tab comes back, goes to the previous link, and then to the
        // previous widget
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QVERIFY(browser->hasFocus());
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QVERIFY(browser->hasFocus());
        key(Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("1"));
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QVERIFY(before->hasFocus());

        // A click on a link gives the focus to the browser, not to the label
        TuiMouseEvent press(TuiMouseEvent::Press, Qt::LeftButton, QPoint(9, 1));
        QCoreApplication::sendEvent(&window, &press);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("2"));
        QVERIFY(browser->hasFocus());
        // New text with links does not move the focus or the policy
        browser->setHtml(QStringLiteral("<a href='3'>three</a>"));
        QCOMPARE(browser->widget()->focusPolicy(), Qt::NoFocus);
        QVERIFY(browser->hasFocus());
        key(Qt::Key_Return);
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("3"));
    }

    void renderPassShowsTheScroll()
    {
        QTuiMainWindow window;
        window.setGeometry(QRect(0, 0, 40, 10));
        m_app->setMainWindow(&window);
        auto *browser = new QTuiTextBrowser(&window);
        browser->setGeometry(QRect(2, 1, 20, 5));
        browser->setMarkdown(numberedLines(50) + QStringLiteral("\n[end](https://x.example/end)"));
        browser->setFocus();
        QCoreApplication::processEvents();

        ScreenBuffer *screen = m_app->screenBuffer();
        auto row = [&](int y) {
            QString s;
            for (int x = 2; x < 21; ++x)
                s += screen->frontCell(x, y).text();
            return s.trimmed();
        };
        QCOMPARE(row(1), QStringLiteral("line 0"));
        QCOMPARE(row(5), QStringLiteral("line 4"));

        sendKey(*browser, Qt::Key_Down);
        QCoreApplication::processEvents();
        QCOMPARE(row(1), QStringLiteral("line 1"));

        // The link is a hyperlink of the terminal in the screen buffer
        sendKey(*browser, Qt::Key_End);
        QCoreApplication::processEvents();
        QCOMPARE(row(5), QStringLiteral("end"));
        QCOMPARE(screen->linkUrl(screen->frontCell(2, 5).link), QStringLiteral("https://x.example/end"));
        QCOMPARE(screen->frontCell(2, 4).link, quint16(0));
    }

    void setSourceReadsALocalFile()
    {
        const QString md = write(QStringLiteral("a.md"), "**bold** [x](y)");
        const QString html = write(QStringLiteral("b.HTML"), "<b>bold</b> &amp; <a href='y'>x</a>");
        const QString txt = write(QStringLiteral("c.txt"), "**plain** <b>");
        const QString utf8 = write(QStringLiteral("d"), "gr\xc3\xbc\xc3\x9f");

        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 30, 5));
        QSignalSpy changed(&browser, &QTuiTextBrowser::sourceChanged);
        browser.setSource(md);
        QCOMPARE(browser.toPlainText(), QStringLiteral("bold x"));
        QCOMPARE(browser.source(), md);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.last().at(0).toString(), md);
        browser.setSource(html);
        QCOMPARE(browser.toPlainText(), QStringLiteral("bold & x"));
        browser.setSource(txt);
        QCOMPARE(browser.toPlainText(), QStringLiteral("**plain** <b>"));
        browser.setSource(utf8);
        QCOMPARE(browser.toPlainText(), QString::fromUtf8("gr\xc3\xbc\xc3\x9f"));
        QCOMPARE(changed.count(), 4);

        // A missing file and a directory: a warning and no text
        const QString missing = m_dir.filePath(QStringLiteral("missing.md"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("cannot read .*missing\\.md")));
        browser.setSource(missing);
        QCOMPARE(browser.toPlainText(), QString());
        QCOMPARE(browser.source(), missing);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("cannot read")));
        browser.setSource(m_dir.path());
        QCOMPARE(browser.toPlainText(), QString());
    }

    void linksOpenLocalFiles()
    {
        QVERIFY(QDir(m_dir.path()).mkpath(QStringLiteral("sub")));
        const QString index = write(QStringLiteral("index.md"),
            "[rel](sub/page.md) [frag](sub/page.md#part) [web](https://x.example/) "
            "[top](#top) [app](help:topic)");
        const QString page = write(QStringLiteral("sub/page.md"), "page [up](../index.md) [abs](ABS) [url](URL)");
        const QString other = write(QStringLiteral("other.txt"), "other");
        // An absolute path and a file: URL as link targets
        {
            QFile file(page);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QByteArray content = file.readAll();
            file.close();
            content.replace("ABS", other.toUtf8());
            content.replace("URL", QUrl::fromLocalFile(other).toString().toUtf8());
            write(QStringLiteral("sub/page.md"), content);
        }

        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 60, 5));
        browser.setSource(index);
        QSignalSpy clicked(&browser, &QTuiTextBrowser::anchorClicked);
        QSignalSpy changed(&browser, &QTuiTextBrowser::sourceChanged);
        auto follow = [&](int link) {
            sendKey(browser, Qt::Key_Home);
            for (int i = 0; i < 10; ++i)
                sendKey(browser, Qt::Key_Left);
            for (int i = 0; i < link; ++i)
                sendKey(browser, Qt::Key_Right);
            sendKey(browser, Qt::Key_Return);
        };

        // Links with a different scheme and "#" links are only reported
        follow(2);
        QCOMPARE(clicked.last().at(0).toString(), QStringLiteral("https://x.example/"));
        follow(3);
        QCOMPARE(clicked.last().at(0).toString(), QStringLiteral("#top"));
        follow(4);
        QCOMPARE(clicked.last().at(0).toString(), QStringLiteral("help:topic"));
        QCOMPARE(changed.count(), 0);
        QCOMPARE(browser.source(), index);

        // A relative link starts at the directory of the source
        follow(0);
        QCOMPARE(clicked.last().at(0).toString(), QStringLiteral("sub/page.md"));
        QCOMPARE(browser.source(), page);
        QVERIFY(browser.toPlainText().startsWith(QStringLiteral("page")));
        follow(0); // "../index.md"
        QCOMPARE(browser.source(), index);
        // The browser removes the fragment
        follow(1);
        QCOMPARE(browser.source(), page);
        follow(1); // the absolute path
        QCOMPARE(browser.source(), other);
        QCOMPARE(browser.toPlainText(), QStringLiteral("other"));
        browser.backward();
        follow(2); // the file: URL
        QCOMPARE(browser.source(), other);

        // With openLinks off, the browser only reports
        browser.setSource(index);
        browser.setOpenLinks(false);
        const int count = clicked.count();
        follow(0);
        QCOMPARE(clicked.count(), count + 1);
        QCOMPARE(browser.source(), index);
    }

    void history()
    {
        const QString a = write(QStringLiteral("a.txt"), "A");
        const QString b = write(QStringLiteral("b.txt"), "B");
        const QString c = write(QStringLiteral("c.txt"), "C");

        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 30, 5));
        QSignalSpy back(&browser, &QTuiTextBrowser::backwardAvailable);
        QSignalSpy next(&browser, &QTuiTextBrowser::forwardAvailable);
        QSignalSpy changed(&browser, &QTuiTextBrowser::sourceChanged);

        // No history: the slots do nothing
        browser.backward();
        browser.forward();
        browser.home();
        QCOMPARE(changed.count(), 0);

        browser.setSource(a);
        QVERIFY(!browser.isBackwardAvailable());
        QCOMPARE(back.count(), 0);
        browser.setSource(b);
        QVERIFY(browser.isBackwardAvailable());
        QCOMPARE(back.count(), 1);
        QCOMPARE(back.last().at(0).toBool(), true);
        browser.setSource(c);
        QCOMPARE(back.count(), 1);
        QCOMPARE(next.count(), 0);

        browser.backward();
        QCOMPARE(browser.source(), b);
        QCOMPARE(browser.toPlainText(), QStringLiteral("B"));
        QCOMPARE(next.last().at(0).toBool(), true);
        QVERIFY(browser.isForwardAvailable());
        browser.backward();
        browser.backward(); // at the start: stays
        QCOMPARE(browser.source(), a);
        QCOMPARE(back.last().at(0).toBool(), false);
        browser.forward();
        browser.forward();
        browser.forward(); // at the end: stays
        QCOMPARE(browser.source(), c);
        QCOMPARE(next.last().at(0).toBool(), false);

        // The keys: Alt+Left and Backspace go back, Alt+Right goes forward
        sendKey(browser, Qt::Key_Left, Qt::AltModifier);
        QCOMPARE(browser.source(), b);
        sendKey(browser, Qt::Key_Backspace);
        QCOMPARE(browser.source(), a);
        sendKey(browser, Qt::Key_Right, Qt::AltModifier);
        QCOMPARE(browser.source(), b);

        // A new file removes the history after the current file
        browser.setSource(a);
        QVERIFY(!browser.isForwardAvailable());
        browser.backward();
        QCOMPARE(browser.source(), b);
        browser.backward();
        QCOMPARE(browser.source(), a);

        // The same file again adds no entry. home() goes to the first file.
        browser.forward();
        browser.setSource(b);
        browser.backward();
        QCOMPARE(browser.source(), a);
        browser.forward();
        browser.forward();
        QCOMPARE(browser.source(), a); // a, b, a
        browser.setSource(c);
        browser.home();
        QCOMPARE(browser.source(), a);
        browser.backward();
        QCOMPARE(browser.source(), c);

        // setMarkdown() does not change the source or the history
        browser.setMarkdown(QStringLiteral("text"));
        QCOMPARE(browser.source(), c);
    }

    void largeDocument()
    {
        // 5000 lines of Markdown with markup and one link on each line
        QString text;
        for (int i = 0; i < 5000; ++i)
            text += QStringLiteral("Line %1 has **bold** text, *italic* text, `code` and a "
                                   "[link %1](page%1.md) in it.\n").arg(i);
        const QString path = write(QStringLiteral("large.md"), text.toUtf8());

        QTuiTextBrowser browser;
        browser.setGeometry(QRect(0, 0, 80, 24));
        QElapsedTimer timer;
        timer.start();
        browser.setSource(path);
        const qint64 loadMs = timer.elapsed();
        // One row for each line, and one for the line break at the end
        QCOMPARE(browser.widget()->geometry(), QRect(0, 0, 79, 5001));

        ScreenBuffer buffer(80, 24);
        browser.paintEvent(buffer);
        QCOMPARE(rowText(buffer, 0, 0, 12), QStringLiteral("Line 0 has b"));
        timer.start();
        const int paints = 50;
        for (int i = 0; i < paints; ++i)
            browser.paintEvent(buffer);
        const double paintMs = double(timer.nsecsElapsed()) / 1e6 / paints;
        timer.start();
        const int moves = 20;
        for (int i = 0; i < moves; ++i)
            sendKey(browser, Qt::Key_Right);
        // The time is information only; it depends on the build and the machine
        qInfo("5000 lines: load %lld ms, paint %.3f ms, move to the next link %.3f ms", loadMs,
              paintMs, double(timer.nsecsElapsed()) / 1e6 / moves);

        sendKey(browser, Qt::Key_End);
        browser.paintEvent(buffer);
        QCOMPARE(rowText(buffer, 22, 0, 16), QStringLiteral("Line 4999 has bo"));
        QVERIFY(buffer.backCell(8, 22).bold == false);
        QVERIFY(buffer.backCell(14, 22).bold);

        // A link after number 255 is its own link
        QSignalSpy spy(&browser, &QTuiTextBrowser::anchorClicked);
        browser.setOpenLinks(false);
        click(browser, 52, 22); // in "link 4999"
        QCOMPARE(spy.last().at(0).toString(), QStringLiteral("page4999.md"));
    }
};

QTEST_APPLESS_MAIN(TestQTuiTextBrowser)
#include "test_qtui_textbrowser.moc"
