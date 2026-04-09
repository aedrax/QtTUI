#include <QtTUI/RenderScheduler.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/ScreenBuffer.h>

#include <QTimer>

namespace QtTUI {

RenderScheduler::RenderScheduler(QTuiApplication *app)
    : QObject(app)
    , m_app(app)
{
}

void RenderScheduler::scheduleDirtyRegion(const QRect &region)
{
    if (m_dirtyUnion.isNull()) {
        m_dirtyUnion = region;
    } else {
        m_dirtyUnion = m_dirtyUnion.united(region);
    }

    if (!m_pending) {
        m_pending = true;
        QTimer::singleShot(0, this, &RenderScheduler::executeRenderPass);
    }
}

void RenderScheduler::renderNow()
{
    m_pending = false;
    executeRenderPass();
}

bool RenderScheduler::hasPendingRender() const
{
    return m_pending;
}

void RenderScheduler::executeRenderPass()
{
    m_pending = false;
    QRect dirtyRegion = m_dirtyUnion;
    m_dirtyUnion = QRect();

    ScreenBuffer *buffer = m_app->screenBuffer();
    if (!buffer)
        return;

    buffer->setResizedDuringRender(false);

    // Paint the widget tree into the back buffer
    QTuiMainWindow *mainWindow = m_app->mainWindow();
    if (mainWindow && mainWindow->isVisible()) {
        mainWindow->paintEvent(*buffer);

        // If a resize happened during painting, abort and reschedule a full render
        if (buffer->resizedDuringRender()) {
            buffer->setResizedDuringRender(false);
            scheduleDirtyRegion(QRect(0, 0, buffer->width(), buffer->height()));
            return;
        }
    }

    // Diff and write only changed cells to the terminal
    AnsiRenderer renderer;
    QByteArray output = renderer.render(*buffer);
    if (!output.isEmpty()) {
        m_app->backend()->write(output);
    }
    buffer->swapBuffers();
}

} // namespace QtTUI
