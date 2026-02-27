#ifndef QTTUI_QTUIAPPLICATION_H
#define QTTUI_QTUIAPPLICATION_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/TerminalBackend.h>
#include <QtTUI/InputParser.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/AnsiRenderer.h>

#include <QCoreApplication>
#include <QSize>

#include <memory>

class QSocketNotifier;

namespace QtTUI {

class QTuiMainWindow;

class QTuiApplication : public QCoreApplication {
    Q_OBJECT
public:
    QTuiApplication(int &argc, char **argv);
    ~QTuiApplication() override;

    static QTuiApplication *instance();

    TerminalBackend *backend() const;
    ScreenBuffer *screenBuffer() const;

    void setMainWindow(QTuiMainWindow *window);
    QTuiMainWindow *mainWindow() const;

    void triggerRender();
    void scheduleRender();

protected:
    bool event(QEvent *event) override;

private slots:
    void onInputReady();
    void onTerminalResized(QSize newSize);

private:
    void restoreTerminal();

    std::unique_ptr<TerminalBackend> m_backend;
    std::unique_ptr<InputParser> m_inputParser;
    std::unique_ptr<ScreenBuffer> m_screenBuffer;
    std::unique_ptr<AnsiRenderer> m_renderer;
    QSocketNotifier *m_inputNotifier = nullptr;
    QTuiMainWindow *m_mainWindow = nullptr;
    bool m_renderScheduled = false;

    static QTuiApplication *s_instance;
};

} // namespace QtTUI

#endif // QTTUI_QTUIAPPLICATION_H
