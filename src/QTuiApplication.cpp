#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>

#include <QtTUI/TuiEvents.h>

#include <QSocketNotifier>
#include <QTimer>

#include <cstdlib>

#ifndef _WIN32
#include <unistd.h>
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

QTuiApplication::QTuiApplication(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    s_instance = this;

    // Create subsystems
    m_backend = createTerminalBackend(this);
    m_inputParser = std::make_unique<InputParser>(this, this);
    const QSize termSize = m_backend->terminalSize();
    m_screenBuffer = std::make_unique<ScreenBuffer>(termSize.width(), termSize.height());
    m_renderer = std::make_unique<AnsiRenderer>();

    // Enter alternate screen and raw mode
    m_backend->enterAlternateScreen();
    m_backend->enterRawMode();

    // Register atexit safety net for terminal restoration
    std::atexit(atexitHandler);

    // Set up stdin notifier for input readiness (POSIX)
#ifndef _WIN32
    m_inputNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(m_inputNotifier, &QSocketNotifier::activated, this, &QTuiApplication::onInputReady);
#endif

    // Connect backend resize signal
    connect(m_backend.get(), &TerminalBackend::resized, this, &QTuiApplication::onTerminalResized);

    // Sync InputParser terminal size
    m_inputParser->setTerminalSize(termSize);
}

QTuiApplication::~QTuiApplication()
{
    restoreTerminal();
    if (s_instance == this)
        s_instance = nullptr;
}

QTuiApplication *QTuiApplication::instance()
{
    return s_instance;
}

TerminalBackend *QTuiApplication::backend() const
{
    return m_backend.get();
}

ScreenBuffer *QTuiApplication::screenBuffer() const
{
    return m_screenBuffer.get();
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
    m_renderScheduled = false;

    // Paint the widget tree into the back buffer
    if (m_mainWindow && m_mainWindow->isVisible()) {
        m_mainWindow->paintEvent(*m_screenBuffer);
    }

    // Diff and write only changed cells to the terminal
    QByteArray output = m_renderer->render(*m_screenBuffer);
    if (!output.isEmpty()) {
        m_backend->write(output);
    }
    m_screenBuffer->swapBuffers();
}

bool QTuiApplication::event(QEvent *event)
{
    // Forward TUI events to the main window
    if (m_mainWindow) {
        if (event->type() == TuiEventTypes::keyEventType()
            || event->type() == TuiEventTypes::mouseEventType()
            || event->type() == TuiEventTypes::resizeEventType()) {
            return QCoreApplication::sendEvent(m_mainWindow, event);
        }
    }
    return QCoreApplication::event(event);
}

void QTuiApplication::scheduleRender()
{
    if (m_renderScheduled)
        return;
    m_renderScheduled = true;
    // Fire after all pending events are processed
    QTimer::singleShot(0, this, &QTuiApplication::triggerRender);
}

void QTuiApplication::onInputReady()
{
    QByteArray data = m_backend->read();
    if (!data.isEmpty()) {
        m_inputParser->processInput(data);
        // Schedule a render after the event batch is dispatched
        scheduleRender();
    }
}

void QTuiApplication::onTerminalResized(QSize newSize)
{
    m_screenBuffer->resize(newSize.width(), newSize.height());
    m_inputParser->setTerminalSize(newSize);
    m_inputParser->processResize(newSize);
    // Resize triggers relayout in widgets, schedule a full repaint
    scheduleRender();
}

void QTuiApplication::restoreTerminal()
{
    if (m_backend) {
        m_backend->exitAlternateScreen();
        m_backend->exitRawMode();
    }
}

} // namespace QtTUI
