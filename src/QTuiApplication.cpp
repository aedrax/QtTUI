#include <QtTUI/QTuiApplication.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/QTuiClipboard.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/RenderScheduler.h>

#include <QtTUI/TuiEvents.h>

#include <QDateTime>
#include <QPointer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSocketNotifier>

#include <cstdlib>

#ifndef _WIN32
#include <unistd.h>
#else
#include <QWinEventNotifier>
#include <windows.h>
#endif

namespace QtTUI {

QTuiApplication *QTuiApplication::s_instance = nullptr;

static void atexitHandler()
{
    if (QTuiApplication::instance()) {
        QTuiApplication::instance()->backend()->exitAlternateScreen();
        QTuiApplication::instance()->backend()->exitRawMode();
    }
}

// ---- Log-file message handler ----
//
// While a real TUI is on screen the terminal is in raw mode and any stderr
// write corrupts the display, so QTuiApplication routes all Qt messages
// (qDebug/qInfo/qWarning/qCritical) to a log file instead:
//     $XDG_STATE_HOME/qttui/<appname>.log
//     (fallback: ~/.local/state/qttui/<appname>.log)
// The handler is only installed when terminal initialization succeeded; in
// headless environments (tests) the default handler stays active. The file is
// opened lazily on the first message, in append mode, and flushed per message
// (correctness over speed). If the directory or file cannot be created the
// output is suppressed entirely — stderr would be worse. qFatal is forwarded
// to the previous handler after logging so default abort handling applies.

static QtMessageHandler s_previousMessageHandler = nullptr;
static QString s_logFilePath;
static QFile s_logFile;
static bool s_logOpenAttempted = false;
static QMutex s_logMutex;

static void logFileMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                  const QString &msg)
{
    {
        QMutexLocker locker(&s_logMutex);

        if (!s_logOpenAttempted) {
            s_logOpenAttempted = true;
            const QString dirPath = QFileInfo(s_logFilePath).absolutePath();
            if (QDir().mkpath(dirPath)) {
                s_logFile.setFileName(s_logFilePath);
                s_logFile.open(QIODevice::Append | QIODevice::Text);
            }
            // On failure s_logFile stays closed and messages are suppressed.
        }

        if (s_logFile.isOpen()) {
            const char *level = "unknown";
            switch (type) {
            case QtDebugMsg:    level = "debug";    break;
            case QtInfoMsg:     level = "info";     break;
            case QtWarningMsg:  level = "warning";  break;
            case QtCriticalMsg: level = "critical"; break;
            case QtFatalMsg:    level = "fatal";    break;
            }
            const QString line = QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                + QLatin1String(" [") + QLatin1String(level) + QLatin1String("] ")
                + msg + QLatin1Char('\n');
            s_logFile.write(line.toUtf8());
            s_logFile.flush();
        }
    }

    if (type == QtFatalMsg) {
        // Preserve default fatal handling (abort) after logging.
        if (s_previousMessageHandler)
            s_previousMessageHandler(type, context, msg);
        else
            abort();
    }
}

QTuiApplication::QTuiApplication(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    s_instance = this;

    // Pick the color depth from the environment. The application can call
    // AnsiRenderer::setColorMode() after this to override it.
    AnsiRenderer::setColorMode(AnsiRenderer::detectColorMode());

    // Create subsystems. The QObject-based ones are parented to this
    // application object and deleted through QObject ownership.
    m_backend = createTerminalBackend(this).release();
    m_inputParser = new InputParser(this, this);
    const QSize termSize = m_backend->terminalSize();
    m_screenBuffer = std::make_unique<ScreenBuffer>(termSize.width(), termSize.height());
    m_scheduler = new RenderScheduler(this);

    // Enter alternate screen and raw mode. Failure is not fatal: headless
    // environments (tests, CI, no tty) legitimately run without terminal
    // control, so log once and continue.
    const bool altScreenOk = m_backend->enterAlternateScreen();
    const bool rawModeOk = m_backend->enterRawMode();
    m_terminalInitialized = altScreenOk && rawModeOk;
    if (!m_terminalInitialized) {
        qWarning("QTuiApplication: terminal initialization failed "
                 "(alternate screen: %s, raw mode: %s); continuing without "
                 "terminal control (headless environment?)",
                 altScreenOk ? "ok" : "failed", rawModeOk ? "ok" : "failed");
    } else {
        // A real TUI is now on screen: route Qt messages to a log file so
        // stderr writes cannot corrupt the raw-mode display (see the
        // logFileMessageHandler comment above).
        QString stateDir = qEnvironmentVariable("XDG_STATE_HOME");
        if (stateDir.isEmpty())
            stateDir = QDir::homePath() + QLatin1String("/.local/state");
        QString appName = applicationName();
        if (appName.isEmpty())
            appName = QLatin1String("qttui");
        {
            QMutexLocker locker(&s_logMutex);
            s_logFilePath = stateDir + QLatin1String("/qttui/") + appName
                + QLatin1String(".log");
            s_logOpenAttempted = false;
        }
        s_previousMessageHandler = qInstallMessageHandler(logFileMessageHandler);
        m_messageHandlerInstalled = true;
    }

    // Force a full redraw on the first render cycle
    m_screenBuffer->invalidateAll();

    // Register atexit safety net for terminal restoration
    std::atexit(atexitHandler);

    // Wake the event loop when input is ready: a socket notifier on stdin
    // (POSIX), or an event notifier on the console input handle (Windows)
#ifndef _WIN32
    m_inputNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(m_inputNotifier, &QSocketNotifier::activated, this, &QTuiApplication::onInputReady);
#else
    auto *consoleNotifier = new QWinEventNotifier(GetStdHandle(STD_INPUT_HANDLE), this);
    m_consoleNotifier = consoleNotifier;
    connect(consoleNotifier, &QWinEventNotifier::activated, this, &QTuiApplication::onInputReady);
#endif

    // Connect backend resize signal
    connect(m_backend, &TerminalBackend::resized, this, &QTuiApplication::onTerminalResized);

    // Sync InputParser terminal size
    m_inputParser->setTerminalSize(termSize);
}

QTuiApplication::~QTuiApplication()
{
    restoreTerminal();

    // Restore the previous message handler after the terminal is restored
    // (teardown messages emitted while still in raw mode go to the file).
    if (m_messageHandlerInstalled) {
        qInstallMessageHandler(s_previousMessageHandler);
        s_previousMessageHandler = nullptr;
        QMutexLocker locker(&s_logMutex);
        if (s_logFile.isOpen())
            s_logFile.close();
        s_logOpenAttempted = false;
    }

    // Delete the QtTUI subsystems (with their notifiers and timers) now,
    // while QCoreApplication and its event dispatcher still exist. As plain
    // QObject children they would go after ~QCoreApplication(), too late to
    // unregister their notifiers and timers; a later QTuiApplication in the
    // same process (tests) then got events for deleted objects. Only the
    // objects that QtTUI creates: Qt attaches its own children too.
    delete m_inputNotifier;
    m_inputNotifier = nullptr;
    delete m_consoleNotifier;
    m_consoleNotifier = nullptr;
    delete m_scheduler;
    m_scheduler = nullptr;
    delete m_inputParser;
    m_inputParser = nullptr;
    delete m_backend;
    m_backend = nullptr;
    deleteClipboard();

    if (s_instance == this)
        s_instance = nullptr;
}

QTuiApplication *QTuiApplication::instance()
{
    return s_instance;
}

TerminalBackend *QTuiApplication::backend() const
{
    return m_backend;
}

ScreenBuffer *QTuiApplication::screenBuffer() const
{
    return m_screenBuffer.get();
}

RenderScheduler *QTuiApplication::scheduler() const
{
    return m_scheduler;
}

bool QTuiApplication::terminalInitialized() const
{
    return m_terminalInitialized;
}

void QTuiApplication::setMainWindow(QTuiMainWindow *window)
{
    m_mainWindow = window;
}

QTuiMainWindow *QTuiApplication::mainWindow() const
{
    return m_mainWindow;
}

void QTuiApplication::triggerRender()
{
    m_scheduler->renderNow();
}

static Qt::LayoutDirection s_layoutDirection = Qt::LeftToRight;
static bool s_reducedMotion = qEnvironmentVariableIntValue("QTTUI_REDUCE_MOTION") != 0;
static bool s_focusCursor = qEnvironmentVariableIntValue("QTTUI_FOCUS_CURSOR") != 0;

bool QTuiApplication::reducedMotion()
{
    return s_reducedMotion;
}

void QTuiApplication::setReducedMotion(bool reduced)
{
    s_reducedMotion = reduced;
}

bool QTuiApplication::focusCursor()
{
    return s_focusCursor;
}

void QTuiApplication::setFocusCursor(bool on)
{
    s_focusCursor = on;
}

Qt::LayoutDirection QTuiApplication::layoutDirection()
{
    return s_layoutDirection;
}

void QTuiApplication::setLayoutDirection(Qt::LayoutDirection direction)
{
    s_layoutDirection = direction == Qt::LayoutDirectionAuto ? Qt::LeftToRight : direction;
}

// Parented to the application when one exists, so it is deleted with it
static QPointer<QTuiClipboard> s_clipboard;

QTuiClipboard *QTuiApplication::clipboard()
{
    if (!s_clipboard)
        s_clipboard = new QTuiClipboard(instance());
    return s_clipboard;
}

void QTuiApplication::deleteClipboard()
{
    if (s_clipboard && s_clipboard->parent() == this)
        delete s_clipboard;
}

void QTuiApplication::suspend()
{
    if (m_backend)
        m_backend->suspend();
}

bool QTuiApplication::event(QEvent *event)
{
    // Forward TUI events to the main window
    if (m_mainWindow) {
        if (event->type() == TuiEventTypes::keyEventType()
            || event->type() == TuiEventTypes::mouseEventType()
            || event->type() == TuiEventTypes::resizeEventType()
            || event->type() == TuiEventTypes::pasteEventType()) {
            return QCoreApplication::sendEvent(m_mainWindow, event);
        }
    }
    return QCoreApplication::event(event);
}

void QTuiApplication::onInputReady()
{
    QByteArray data = m_backend->read();
    if (!data.isEmpty()) {
        m_inputParser->processInput(data);
        // No blanket render here: widgets that change state in response to
        // the dispatched events mark themselves dirty, which schedules the
        // exact regions that need repainting.
    }
}

void QTuiApplication::onTerminalResized(QSize newSize)
{
    const QSize oldSize = m_inputParser->terminalSize();

    m_screenBuffer->resize(newSize.width(), newSize.height());
    m_inputParser->setTerminalSize(newSize);

    // Deliver the resize event synchronously (event() forwards it to the
    // main window) so the window has relayouted before the repaint below.
    // This replaces the previous posted-event + processEvents() dance.
    TuiResizeEvent resizeEvent(newSize, oldSize);
    QCoreApplication::sendEvent(this, &resizeEvent);

    // Now do a full repaint with the updated layout
    m_scheduler->renderNow();
}

void QTuiApplication::restoreTerminal()
{
    if (m_backend) {
        m_backend->exitAlternateScreen();
        m_backend->exitRawMode();
    }
}

} // namespace QtTUI
