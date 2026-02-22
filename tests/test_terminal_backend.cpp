#include <QtTest/QtTest>
#include <rapidcheck.h>

#include "MockTerminalBackend.h"

using namespace QtTUI;

class TestTerminalBackend : public QObject {
    Q_OBJECT

private slots:
    void property35_resizeSignalEmitsDimensions();
    void testEnterExitRawMode();
    void testEnterExitAlternateScreen();
    void testReadWriteBuffers();
    void testTerminalSizeConfigurable();
};

void TestTerminalBackend::property35_resizeSignalEmitsDimensions()
{
    // Property 35: For any dimensions (w, h), the backend emits
    // resized signal with QSize(w, h).
    rc::check("resize signal carries exact dimensions", []() {
        const int w = *rc::gen::inRange(1, 500);
        const int h = *rc::gen::inRange(1, 500);

        MockTerminalBackend backend;
        QSize received;
        int signalCount = 0;

        QObject::connect(&backend, &TerminalBackend::resized,
                         [&](QSize size) {
                             received = size;
                             ++signalCount;
                         });

        backend.emitResized(QSize(w, h));

        RC_ASSERT(signalCount == 1);
        RC_ASSERT(received.width() == w);
        RC_ASSERT(received.height() == h);
    });
}

void TestTerminalBackend::testEnterExitRawMode()
{
    MockTerminalBackend backend;

    QVERIFY(!backend.isRawMode());
    QVERIFY(backend.enterRawMode());
    QVERIFY(backend.isRawMode());
    QVERIFY(backend.exitRawMode());
    QVERIFY(!backend.isRawMode());
}

void TestTerminalBackend::testEnterExitAlternateScreen()
{
    MockTerminalBackend backend;

    QVERIFY(!backend.isAlternateScreen());
    QVERIFY(backend.enterAlternateScreen());
    QVERIFY(backend.isAlternateScreen());
    QVERIFY(backend.exitAlternateScreen());
    QVERIFY(!backend.isAlternateScreen());
}

void TestTerminalBackend::testReadWriteBuffers()
{
    MockTerminalBackend backend;

    // Write data and verify it's captured
    QByteArray testData("hello terminal");
    qint64 written = backend.write(testData);
    QCOMPARE(written, testData.size());
    QCOMPARE(backend.writtenData(), testData);

    // Feed input and read it back
    QByteArray inputData("\x1b[A");
    backend.feedInput(inputData);
    QCOMPARE(backend.read(), inputData);

    // Second read should be empty
    QVERIFY(backend.read().isEmpty());
}

void TestTerminalBackend::testTerminalSizeConfigurable()
{
    MockTerminalBackend backend;

    QCOMPARE(backend.terminalSize(), QSize(80, 24));

    backend.setTerminalSize(QSize(120, 40));
    QCOMPARE(backend.terminalSize(), QSize(120, 40));
}

QTEST_GUILESS_MAIN(TestTerminalBackend)
#include "test_terminal_backend.moc"
