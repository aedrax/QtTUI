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

class QTuiClipboard;
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

    // The application clipboard (created on first use)
    static QTuiClipboard *clipboard();

    // Default layout direction of widgets (LeftToRight unless set), as in
    // QGuiApplication. Set it before you build the widgets.
    static Qt::LayoutDirection layoutDirection();
    static void setLayoutDirection(Qt::LayoutDirection direction);

    // Reduced motion: no moving busy bar and no pressed-button flash. The
    // start value comes from QTTUI_REDUCE_MOTION=1.
    static bool reducedMotion();
    static void setReducedMotion(bool reduced);

    // Focus cursor: the terminal cursor stays on the focus widget, also
    // when it is not editable, so screen readers and magnifiers that
    // follow the cursor follow the focus. The start value comes from
    // QTTUI_FOCUS_CURSOR=1.
    static bool focusCursor();
    static void setFocusCursor(bool on);

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
