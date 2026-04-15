#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/RenderScheduler.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>
#include <QDebug>

namespace QtTUI {

QTuiWidget *QTuiWidget::s_focusWidget = nullptr;

QTuiWidget::QTuiWidget(QTuiWidget *parent)
    : QObject(parent)
{
}

QTuiWidget::~QTuiWidget()
{
    if (s_focusWidget == this)
        s_focusWidget = nullptr;
}

QRect QTuiWidget::geometry() const
{
    return m_geometry;
}

void QTuiWidget::setGeometry(const QRect &rect)
{
    QRect clamped = rect;
    if (clamped.width() < 0) {
        qWarning() << "QTuiWidget::setGeometry: clamping negative width"
                    << clamped.width() << "to 0";
        clamped.setWidth(0);
    }
    if (clamped.height() < 0) {
        qWarning() << "QTuiWidget::setGeometry: clamping negative height"
                    << clamped.height() << "to 0";
        clamped.setHeight(0);
    }

    if (m_geometry == clamped)
        return;

    // Track dirty regions: old and new geometry
    m_dirtyOld = m_geometry;
    m_dirtyNew = clamped;
    m_hasDirtyRegions = true;

    m_geometry = clamped;

    // Notify the scheduler about both old and new geometry regions
    markDirty(m_dirtyOld);
    markDirty(m_dirtyNew);

    emit geometryChanged(m_geometry);
}

bool QTuiWidget::isVisible() const
{
    return m_visible;
}

void QTuiWidget::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    // When transitioning to hidden, mark current geometry dirty so the area is cleared
    if (m_visible && !visible) {
        markDirty();
    }

    m_visible = visible;
    emit visibilityChanged(m_visible);
}

bool QTuiWidget::hasFocus() const
{
    return m_hasFocus;
}

void QTuiWidget::setFocus()
{
    setFocusWidget(this);
}

void QTuiWidget::setFocusWidget(QTuiWidget *widget)
{
    if (s_focusWidget == widget)
        return;

    QTuiWidget *prev = s_focusWidget;
    s_focusWidget = widget;

    if (prev) {
        prev->m_hasFocus = false;
        prev->markDirty();
        emit prev->focusLost();
    }
    if (widget) {
        widget->m_hasFocus = true;
        widget->markDirty();
        emit widget->focusGained();
    }
}

QTuiWidget *QTuiWidget::parentWidget() const
{
    return qobject_cast<QTuiWidget *>(parent());
}

QList<QTuiWidget *> QTuiWidget::childWidgets() const
{
    QList<QTuiWidget *> result;
    for (QObject *child : children()) {
        if (auto *w = qobject_cast<QTuiWidget *>(child))
            result.append(w);
    }
    return result;
}

QSize QTuiWidget::minimumSize() const
{
    return QSize(0, 0);
}

ThemeProperties QTuiWidget::themeProperties() const
{
    return Theme_Engine::instance()->effectiveProperties(this);
}

void QTuiWidget::markDirty()
{
    markDirty(m_geometry);
}

void QTuiWidget::markDirty(const QRect &region)
{
    if (m_dirtyRegion.isNull()) {
        m_dirtyRegion = region;
    } else {
        m_dirtyRegion = m_dirtyRegion.united(region);
    }
    m_isDirty = true;

    auto *app = QTuiApplication::instance();
    if (app && app->scheduler()) {
        app->scheduler()->scheduleDirtyRegion(region);
    }
}

void QTuiWidget::clearBackground(ScreenBuffer &buffer, const QRect &clipRect)
{
    const QRect geo = geometry();
    const QRect clearArea = geo.intersected(clipRect);
    if (clearArea.isEmpty())
        return;

    const auto theme = themeProperties();

    for (int row = clearArea.y(); row < clearArea.y() + clearArea.height(); ++row) {
        for (int col = clearArea.x(); col < clearArea.x() + clearArea.width(); ++col) {
            Cell &cell = buffer.backCell(col, row);
            cell.character = QLatin1Char(' ');
            cell.foreground = theme.foreground;
            cell.background = theme.background;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;
            cell.strikethrough = false;
            cell.inverse = false;
        }
    }

    // Handle geometry-shrink clearing: clear old geometry areas not covered by new geometry
    if (m_hasDirtyRegions && !m_dirtyOld.isEmpty() && m_dirtyOld != geo) {
        // Determine the region of the old geometry that is not covered by the new geometry
        // We clear those cells with the parent's background color
        QTuiWidget *pw = parentWidget();
        ThemeProperties parentTheme;
        if (pw) {
            parentTheme = pw->themeProperties();
        }

        // Clear rows of old geometry that are outside new geometry
        const QRect oldClipped = m_dirtyOld.intersected(clipRect);
        for (int row = oldClipped.y(); row < oldClipped.y() + oldClipped.height(); ++row) {
            for (int col = oldClipped.x(); col < oldClipped.x() + oldClipped.width(); ++col) {
                // Only clear cells that are NOT in the new geometry
                if (!geo.contains(col, row)) {
                    Cell &cell = buffer.backCell(col, row);
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

bool QTuiWidget::event(QEvent *event)
{
    if (event->type() == TuiEventTypes::keyEventType()) {
        keyPressEvent(static_cast<TuiKeyEvent *>(event));
        return true;
    }
    if (event->type() == TuiEventTypes::mouseEventType()) {
        mousePressEvent(static_cast<TuiMouseEvent *>(event));
        return true;
    }
    if (event->type() == TuiEventTypes::resizeEventType()) {
        resizeEvent(static_cast<TuiResizeEvent *>(event));
        return true;
    }
    return QObject::event(event);
}

void QTuiWidget::keyPressEvent(TuiKeyEvent * /*event*/) {}
void QTuiWidget::mousePressEvent(TuiMouseEvent * /*event*/) {}
void QTuiWidget::resizeEvent(TuiResizeEvent * /*event*/) {}

bool QTuiWidget::hasDirtyRegions() const
{
    return m_hasDirtyRegions;
}

QRect QTuiWidget::dirtyOldRegion() const
{
    return m_dirtyOld;
}

QRect QTuiWidget::dirtyNewRegion() const
{
    return m_dirtyNew;
}

void QTuiWidget::clearDirtyRegions()
{
    m_hasDirtyRegions = false;
    m_dirtyOld = QRect();
    m_dirtyNew = QRect();
    m_isDirty = false;
    m_dirtyRegion = QRect();
}

bool QTuiWidget::isDirty() const
{
    return m_isDirty;
}

QRect QTuiWidget::dirtyRegion() const
{
    return m_dirtyRegion;
}

} // namespace QtTUI
