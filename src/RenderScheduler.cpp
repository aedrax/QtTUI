#include <QtTUI/RenderScheduler.h>
#include <QtTUI/RenderUtils.h>
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
    updateRendererThemeColors();
}

// The renderer needs the theme colors for its no-color mode
void RenderScheduler::updateRendererThemeColors()
{
    const ThemeProperties theme = Theme_Engine::instance()->properties(QStringLiteral("default"));
    AnsiRenderer::setThemeColors(theme.foreground, theme.background, theme.focusBackground);
}

void RenderScheduler::scheduleDirtyRegion(const QRect &region)
{
    if (m_dirtyUnion.isNull()) {
        m_dirtyUnion = region;
    } else {
        m_dirtyUnion = m_dirtyUnion.united(region);
    }

    if (!region.isEmpty()) {
        m_dirtyRects.append(region);
        // Keep the intersection tests cheap: collapse to the union past a bound
        if (m_dirtyRects.size() > 32) {
            m_dirtyRects.clear();
            m_dirtyRects.append(m_dirtyUnion);
        }
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
    m_passRects = std::move(m_dirtyRects);
    m_dirtyRects.clear();

    ScreenBuffer *buffer = m_app->screenBuffer();
    if (!buffer)
        return;

    // If no dirty regions and not a full repaint, skip paint traversal entirely
    if (dirtyRegion.isNull() && !m_fullRepaint) {
        m_passRects.clear();
        return;
    }

    buffer->setResizedDuringRender(false);

    // Paint the widget tree into the back buffer using clip-rect propagation
    QTuiMainWindow *mainWindow = m_app->mainWindow();
    if (mainWindow && mainWindow->isVisible()) {
        QRect rootClipRect(0, 0, buffer->width(), buffer->height());

        // Restore content under popups that closed since the last pass. Popups
        // escape their parent's clip rect, so their vacated regions must be
        // cleared against the full screen — before anything paints, so fresh
        // sibling output lands on top of the cleared cells.
        clearHiddenPopupRegions(mainWindow, rootClipRect);

        paintTree(mainWindow, rootClipRect);

        // Topmost pass: visible popups paint above everything painted so far,
        // clipped only to the screen rather than to their parent widget.
        paintPopupLayer(mainWindow, rootClipRect);

        // If a resize happened during painting, abort and reschedule a full render
        if (buffer->resizedDuringRender()) {
            buffer->setResizedDuringRender(false);
            m_fullRepaint = true;
            m_passRects.clear();
            scheduleDirtyRegion(QRect(0, 0, buffer->width(), buffer->height()));
            return;
        }
    }

    m_fullRepaint = false;
    m_passRects.clear();

    // Diff and write only changed cells to the terminal
    AnsiRenderer renderer;
    QByteArray output = renderer.render(*buffer);

    // Park the hardware cursor at the focused editable widget's position (so
    // the terminal shows the edit point), or keep it hidden. The hide prefix
    // prevents visible cursor skitter while the frame's cells are written.
    QPoint cursorPos(-1, -1);
    QTuiWidget *focused = QTuiWidget::focusWidget();
    if (focused && focused->isVisible())
        cursorPos = focused->terminalCursorPos();
    // Focus cursor mode: a widget without its own cursor gets the cursor
    // on its first cell
    if (focused && focused->isVisible() && cursorPos.x() < 0 && QTuiApplication::focusCursor())
        cursorPos = focused->geometry().topLeft();

    const bool cursorVisible = cursorPos.x() >= 0 && cursorPos.y() >= 0
        && cursorPos.x() < buffer->width() && cursorPos.y() < buffer->height();

    if (cursorVisible) {
        output.prepend("\x1b[?25l");
        output.append(AnsiRenderer::cursorPosition(cursorPos.x(), cursorPos.y()));
        output.append("\x1b[?25h");
    } else if (!output.isEmpty() || m_cursorShown) {
        output.prepend("\x1b[?25l");
    }
    m_cursorShown = cursorVisible;

    if (!output.isEmpty()) {
        m_app->backend()->write(output);
    }
    buffer->swapBuffers();
}

bool RenderScheduler::intersectsPassRectsFully(const QRect &rect) const
{
    for (const QRect &damage : m_passRects) {
        if (damage.contains(rect))
            return true;
    }
    return false;
}

bool RenderScheduler::intersectsPassRects(const QRect &rect) const
{
    for (const QRect &damage : m_passRects) {
        if (rect.intersects(damage))
            return true;
    }
    return false;
}

void RenderScheduler::clearVacatedRegion(QTuiWidget *parent, const QRect &region)
{
    ScreenBuffer *buffer = m_app->screenBuffer();
    if (!buffer || region.isEmpty())
        return;

    ThemeProperties parentTheme = parent->themeProperties();
    fillRect(*buffer, region, cellStyle(parentTheme.foreground, parentTheme.background));
}

void RenderScheduler::paintTree(QTuiWidget *widget, const QRect &parentClipRect)
{
    // Compute the effective clip rect as the intersection of widget geometry and parent clip
    QRect clipRect = widget->geometry().intersected(parentClipRect);

    // If the clip rect is empty, the widget is entirely outside the visible area — skip it
    if (clipRect.isEmpty())
        return;

    // First: clear regions vacated by newly hidden children, BEFORE this widget
    // and its visible children paint, so their fresh content lands on top.
    // Hidden popups are excluded: their vacated regions may extend outside this
    // widget's clip rect and are cleared screen-wide in clearHiddenPopupRegions().
    for (auto *child : widget->childWidgets()) {
        if (!child->isVisible() && child->isDirty() && !child->isPopup()) {
            clearVacatedRegion(widget, child->geometry().intersected(clipRect));
            child->clearDirtyRegions();
        }
    }

    // Paint this widget if it is dirty, we're doing a full repaint, or it
    // intersects damage from another widget (hidden/moved overlay above it)
    if (widget->isDirty() || m_fullRepaint || intersectsPassRects(widget->geometry())) {
        ScreenBuffer *buffer = m_app->screenBuffer();
        if (buffer) {
            widget->paintEvent(*buffer);
        }
        widget->clearDirtyRegions();

        // A widget paints its whole geometry, which can be more than the
        // damage that caused the repaint. Record the painted area, so that
        // its children and any popup above it repaint on top of it. The
        // main window paints nothing itself.
        if (widget != m_app->mainWindow() && !intersectsPassRectsFully(widget->geometry()))
            m_passRects.append(widget->geometry());
    }

    // Recurse into visible children, passing the computed clip rect. Popup
    // children are skipped here: they belong to the topmost popup layer and
    // must not be clipped to this widget's rect (see paintPopupLayer()).
    // Children painted by their parent are skipped too: their geometry is not
    // a screen position.
    for (auto *child : widget->childWidgets()) {
        if (child->isVisible() && !child->isPopup() && !child->isPaintedByParent()) {
            paintTree(child, clipRect);
        }
    }
}

void RenderScheduler::paintPopupLayer(QTuiWidget *root, const QRect &screenRect)
{
    // Pre-order collection: a parent popup is collected (and painted) before
    // any popup in its subtree, so submenus land on top of their menu.
    QVector<QTuiWidget *> popups;
    collectVisiblePopups(root, popups);

    // paintTree's repaint condition (dirty / full repaint / intersects damage)
    // also covers popups: anything repainted underneath a popup schedules
    // damage that intersects the popup, so the popup repaints on top of it.
    for (QTuiWidget *popup : popups)
        paintTree(popup, screenRect);
}

void RenderScheduler::collectVisiblePopups(QTuiWidget *widget, QVector<QTuiWidget *> &popups)
{
    for (auto *child : widget->childWidgets()) {
        if (!child->isVisible())
            continue;
        if (child->isPopup())
            popups.append(child);
        collectVisiblePopups(child, popups);
    }
}

void RenderScheduler::clearHiddenPopupRegions(QTuiWidget *widget, const QRect &screenRect)
{
    for (auto *child : widget->childWidgets()) {
        if (child->isPopup() && !child->isVisible() && child->isDirty()) {
            // A popup floats over unrelated widgets, so its QObject parent's
            // theme is meaningless for the vacated cells; clear with the main
            // window's theme and let damage-intersecting widgets repaint over.
            clearVacatedRegion(m_app->mainWindow(), child->geometry().intersected(screenRect));
            child->clearDirtyRegions();
        }
        // Descend even into hidden widgets: a just-closed menu may still own a
        // just-closed submenu whose region also needs clearing.
        clearHiddenPopupRegions(child, screenRect);
    }
}

void RenderScheduler::onThemeChanged()
{
    updateRendererThemeColors();
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
