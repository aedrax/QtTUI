// Terminal window title: OSC 2, control character removal, and the title
// stack (XTWINOPS 22;0 / 23;0) on start, suspend and exit.
#include <QtTUI/QTuiApplication.h>

#include "RecordingBackend.h"

#include <QtTest/QtTest>

using namespace QtTUI;

static const QByteArray kPush = "\x1b[22;0t";
static const QByteArray kPop = "\x1b[23;0t";

class TestTerminalTitle : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { useTemporaryStateDir(); }

    void pushOnStartPopOnExit()
    {
        QByteArray log;
        {
            int argc = 1;
            char name[] = "test";
            char *argv[] = {name, nullptr};
            QTuiApplication app(argc, argv, std::make_unique<RecordingBackend>(&log));
            QCOMPARE(log, kPush);
            QVERIFY(app.windowTitle().isNull());
            log.clear();
        }
        QCOMPARE(log, kPop);
    }

    void setTitleWritesOsc2()
    {
        QByteArray log;
        int argc = 1;
        char name[] = "test";
        char *argv[] = {name, nullptr};
        QTuiApplication app(argc, argv, std::make_unique<RecordingBackend>(&log));
        log.clear();

        app.setWindowTitle(QStringLiteral("Report é"));
        QCOMPARE(app.windowTitle(), QStringLiteral("Report é"));
        QCOMPARE(log, QByteArray("\x1b]2;Report \xc3\xa9\x1b\\"));
    }

    void titleHasNoControlCharacters()
    {
        QByteArray log;
        int argc = 1;
        char name[] = "test";
        char *argv[] = {name, nullptr};
        QTuiApplication app(argc, argv, std::make_unique<RecordingBackend>(&log));
        log.clear();

        // ESC, BEL, a line feed, DEL and the C1 controls OSC (U+009D) and
        // ST (U+009C) must not get to the terminal
        const QString hostile = QString::fromUtf8("a\x1b]2;evil\x07" "b\nc\x7f")
            + QChar(0x9d) + QChar(0x9c) + QLatin1Char('d');
        app.setWindowTitle(hostile);
        QCOMPARE(app.windowTitle(), QStringLiteral("a]2;evilbcd"));
        QCOMPARE(log, QByteArray("\x1b]2;a]2;evilbcd\x1b\\"));
    }

    void suspendPopsThenPushesAndSetsTheTitleAgain()
    {
        QByteArray log;
        int argc = 1;
        char name[] = "test";
        char *argv[] = {name, nullptr};
        QTuiApplication app(argc, argv, std::make_unique<RecordingBackend>(&log));
        app.setWindowTitle(QStringLiteral("T"));
        log.clear();

        app.suspend();
        QCOMPARE(log, kPop + kPush + QByteArray("\x1b]2;T\x1b\\"));
    }

    void suspendWithoutTitleWritesNoTitle()
    {
        QByteArray log;
        int argc = 1;
        char name[] = "test";
        char *argv[] = {name, nullptr};
        QTuiApplication app(argc, argv, std::make_unique<RecordingBackend>(&log));
        log.clear();

        app.suspend();
        QCOMPARE(log, kPop + kPush);
    }
};

QTEST_APPLESS_MAIN(TestTerminalTitle)
#include "test_terminal_title.moc"
