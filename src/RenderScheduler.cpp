#include <QtTUI/RenderScheduler.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/AnsiRenderer.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>

#include <QTimer>

namespace QtTUI {

RenderScheduler::RenderScheduler(QTuiApplication *app)
    : QObject(app)
    , m_app(app)
{
    // Connect theme change signal to mark all visible widgets dirty
    connect(Theme_Engine::instance(), &Theme_Engine::themeChanged,
            this, &RenderScheduler::onThemeChanged);
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
    m_fullRepaint = true;
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

    // If no dirty regions and not a full repaint, skip paint traversal entirely
    if (dirtyRegion.isNull() && !m_fullRepaint)
        return;

    buffer->setResizedDuringRender(false);

    // Paint the widget tree into the back buffer using clip-rect propagation
    QTuiMainWindow *mainWindow = m_app->mainWindow();
    if (mainWindow && mainWindow->isVisible()) {
        QRect rootClipRect(0, 0, buffer->width(), buffer->height());
        paintTree(mainWindow, rootClipRect);

        // If a resize happened during painting, abort and reschedule a full render
        if (buffer->resizedDuringRender()) {
            buffer->setResizedDuringRender(false);
            m_fullRepaint = true;
            scheduleDirtyRegion(QRect(0, 0, buffer->width(), buffer->height()));
            return;
        }
    }

    m_fullRepaint = false;

    // Diff and write only changed cells to the terminal
    AnsiRenderer renderer;
    QByteArray output = renderer.render(*buffer);
    if (!output.isEmpty()) {
        m_app->backend()->write(output);
    }
    buffer->swapBuffers();
}

void RenderScheduler::paintTree(QTuiWidget *widget, const QRect &parentClipRect)
{
    // Compute the effective clip rect as the intersection of widget geometry and parent clip
    QRect clipRect = widget->geometry().intersected(parentClipRect);

    // If the clip rect is empty, the widget is entirely outside the visible area — skip it
    if (clipRect.isEmpty())
        return;

    // Paint this widget only if it is dirty or we're doing a full repaint
    if (widget->isDirty() || m_fullRepaint) {
        ScreenBuffer *buffer = m_app->screenBuffer();
        if (buffer) {
            widget->paintEvent(*buffer);
        }
        widget->clearDirtyRegions();
    }

    // Recurse into visible children, passing the computed clip rect
    for (auto *child : widget->childWidgets()) {
        if (child->isVisible()) {
            paintTree(child, clipRect);
        } else if (child->isDirty()) {
            if (!m_fullRepaint) {
                // Hidden widget that was just marked dirty (transitioned to hidden):
                // Clear its geometry with the parent's background color.
                // Skip during full repaint — visible siblings will paint over the area.
                ScreenBuffer *buffer = m_app->screenBuffer();
                if (buffer) {
                    QRect childClip = child->geometry().intersected(clipRect);
                    if (!childClip.isEmpty()) {
                        ThemeProperties parentTheme = widget->themeProperties();
                        for (int row = childClip.y(); row < childClip.y() + childClip.height(); ++row) {
                            for (int col = childClip.x(); col < childClip.x() + childClip.width(); ++col) {
                                Cell &cell = buffer->backCell(col, row);
                                cell.character = QLatin1Char(' ');
                                cell.foreground = parentTheme.foreground;
                                cell.background = parentTheme.background;
                                cell.bold = false;
                                cell.italic = false;
                                cell.underline = false;
                                cell.strikethrough = false;
                                cell.inverse = false;
                            }
                        }
                    }
                }
            }
            // Always clear dirty state so the widget isn't re-processed
            child->clearDirtyRegions();
        }
    }
}

void RenderScheduler::onThemeChanged()
{
    QTuiMainWindow *mainWindow = m_app->mainWindow();
    if (mainWindow && mainWindow->isVisible()) {
        markAllVisibleWidgetsDirty(mainWindow);
    }
}

void RenderScheduler::markAllVisibleWidgetsDirty(QTuiWidget *widget)
{
    if (!widget || !widget->isVisible())
        return;

    widget->markDirty();

    for (auto *child : widget->childWidgets()) {
        if (child->isVisible()) {
            markAllVisibleWidgetsDirty(child);
        }
    }
}

} // namespace QtTUI
