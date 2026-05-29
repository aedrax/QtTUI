#ifndef QTTUI_QTUIAPPLICATION_H
#define QTTUI_QTUIAPPLICATION_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/TerminalBackend.h>
#include <QtTUI/InputParser.h>
#include <QtTUI/ScreenBuffer.h>

#include <QCoreApplication>
#include <QSize>

#include <memory>

class QSocketNotifier;

namespace QtTUI {

class QTuiMainWindow;
class RenderScheduler;

class QTuiApplication : public QCoreApplication {
    Q_OBJECT
public:
    QTuiApplication(int &argc, char **argv);
    ~QTuiApplication() override;

    static QTuiApplication *instance();

    TerminalBackend *backend() const;
    ScreenBuffer *screenBuffer() const;
    RenderScheduler *scheduler() const;

    // True if the backend successfully entered both the alternate screen and
    // raw mode at construction. False in headless environments (e.g. tests
    // without a tty); the application still runs, it just does not control a
    // terminal.
    bool terminalInitialized() const;

    void setMainWindow(QTuiMainWindow *window);
    QTuiMainWindow *mainWindow() const;

    void triggerRender();

    // Gives the terminal back to the shell and stops the process, as Ctrl+Z
    // does in a normal program. The main window calls this for Ctrl+Z
    // unless an action uses Ctrl+Z as its shortcut. On resume ("fg"), the
    // terminal modes are set again and the screen is drawn again.
    void suspend();

protected:
    bool event(QEvent *event) override;

private slots:
    void onInputReady();
    void onTerminalResized(QSize newSize);

private:
    void restoreTerminal();

    // QObject-based subsystems are owned via QObject parenting (parent = this);
    // plain pointers avoid double deletion. Non-QObject subsystems stay in
    // unique_ptrs.
    TerminalBackend *m_backend = nullptr;
    InputParser *m_inputParser = nullptr;
    std::unique_ptr<ScreenBuffer> m_screenBuffer;
    RenderScheduler *m_scheduler = nullptr;
    QSocketNotifier *m_inputNotifier = nullptr;
    QTuiMainWindow *m_mainWindow = nullptr;
    bool m_terminalInitialized = false;
    bool m_messageHandlerInstalled = false;

    static QTuiApplication *s_instance;
};

} // namespace QtTUI

#endif // QTTUI_QTUIAPPLICATION_H
