// Tests for QTuiApplication error-handling behavior:
// - terminalInitialized() reports whether raw mode + alternate screen setup
//   succeeded (false in headless environments such as ctest, where stdin is
//   not a tty).
// - When terminal initialization fails, QTuiApplication must NOT install its
//   log-file message handler; Qt messages keep flowing through the default
//   (here: QtTest's) handler.
//
// Uses QTEST_APPLESS_MAIN because QTuiApplication is itself a
// QCoreApplication and is constructed inside the test slots.

#include <QtTest/QtTest>
#include <QtTUI/QTuiApplication.h>

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace QtTUI;

namespace {

bool stdinIsTty()
{
#ifndef _WIN32
    return ::isatty(STDIN_FILENO) == 1;
#else
    return false;
#endif
}

} // namespace

class TestQTuiApplication : public QObject {
    Q_OBJECT
private slots:
    void terminalInitializedReflectsEnvironment()
    {
        static int argc = 1;
        static char arg0[] = "test_qtui_application";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        // Raw mode needs stdin to be a real terminal. Under ctest/CI there is
        // no tty, so initialization fails and terminalInitialized() must be
        // false while the application object still works. When someone runs
        // this binary from an interactive shell, initialization can succeed,
        // so compare against the actual environment instead of hard-coding.
        if (!stdinIsTty()) {
            QVERIFY(!app.terminalInitialized());
        } else {
            QVERIFY(app.terminalInitialized());
        }

        // The pipeline must exist either way (headless construction is a
        // supported mode that the whole test suite relies on).
        QVERIFY(app.backend() != nullptr);
        QVERIFY(app.screenBuffer() != nullptr);
        QVERIFY(app.scheduler() != nullptr);
    }

    void headlessAppKeepsDefaultMessageHandler()
    {
        static int argc = 1;
        static char arg0[] = "test_qtui_application";
        static char *argv[] = { arg0, nullptr };
        QTuiApplication app(argc, argv);

        if (app.terminalInitialized())
            QSKIP("Running on a real terminal; the log-file handler is installed by design");

        // With no terminal, QTuiApplication must not have hijacked the
        // message handler: this only passes if QtTest's own handler still
        // receives the warning.
        QTest::ignoreMessage(QtWarningMsg, "qttui message handler probe");
        qWarning("qttui message handler probe");
    }

    void headlessDestructionRestoresNothingAndWarningsStillWork()
    {
        {
            static int argc = 1;
            static char arg0[] = "test_qtui_application";
            static char *argv[] = { arg0, nullptr };
            QTuiApplication app(argc, argv);
            if (app.terminalInitialized())
                QSKIP("Running on a real terminal; skipping headless-only check");
        }
        // After the app is destroyed, the default handler must still be in
        // place (destructor must not have "restored" a handler it never
        // installed).
        QTest::ignoreMessage(QtWarningMsg, "qttui post-destruction probe");
        qWarning("qttui post-destruction probe");
    }
};

QTEST_APPLESS_MAIN(TestQTuiApplication)
#include "test_qtui_application.moc"
