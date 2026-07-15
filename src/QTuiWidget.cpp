#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiAction.h>
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

    const bool wasVisible = m_visible;
    m_visible = visible;

    if (visible) {
        // Becoming visible: mark dirty so the widget gets painted on next render pass
        markDirty();
        if (m_isPopup) {
            // Remember who had focus so it can be restored when the popup
            // closes. Openers call setFocus() after setVisible(true), so the
            // previously focused widget is still current at this point.
            QTuiWidget *focused = focusWidget();
            if (focused && !isAncestorOf(focused))
                m_focusBeforePopup = focused;
        }
    } else if (wasVisible) {
        // Becoming hidden: mark dirty so the area is cleared on next render pass
        markDirty();
        if (m_isPopup) {
            // Popup closing: hand focus back to the widget that was focused
            // before it opened, but only if focus is still inside the popup.
            QTuiWidget *focused = focusWidget();
            if (focused && isAncestorOf(focused)) {
                if (m_focusBeforePopup && m_focusBeforePopup->isVisible())
                    m_focusBeforePopup->setFocus();
                else
                    setFocusWidget(nullptr);
            }
            m_focusBeforePopup.clear();
        }
    }

    emit visibilityChanged(m_visible);
}

bool QTuiWidget::isEnabled() const
{
    return m_enabled;
}

void QTuiWidget::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;

    // A widget that contains the focus and becomes (effectively) disabled
    // must release it: keys would otherwise keep flowing to a dead widget.
    if (!enabled) {
        QTuiWidget *focused = focusWidget();
        if (focused && (focused == this || isAncestorOf(focused)))
            setFocusWidget(nullptr);
    }

    // Repaint the whole subtree: children pick up the effective state through
    // isEffectivelyEnabled() when they repaint, and dirtying our geometry
    // covers theirs (children render inside the parent rect).
    markDirty();
    emit enabledChanged(m_enabled);
}

bool QTuiWidget::isEffectivelyEnabled() const
{
    for (const QTuiWidget *w = this; w; w = w->parentWidget()) {
        if (!w->m_enabled)
            return false;
    }
    return true;
}

Qt::FocusPolicy QTuiWidget::focusPolicy() const
{
    return m_focusPolicy;
}

void QTuiWidget::setFocusPolicy(Qt::FocusPolicy policy)
{
    m_focusPolicy = policy;
}

bool QTuiWidget::acceptsTextInput() const
{
    return false;
}

bool QTuiWidget::hasFocus() const
{
    return m_hasFocus;
}

void QTuiWidget::setFocus()
{
    if (m_focusPolicy == Qt::NoFocus)
        return;
    setFocusWidget(this);
}

QTuiWidget *QTuiWidget::focusWidget()
{
    return s_focusWidget;
}

QPoint QTuiWidget::terminalCursorPos() const
{
    return QPoint(-1, -1);
}

bool QTuiWidget::isPopup() const
{
    return m_isPopup;
}

void QTuiWidget::setPopup(bool popup)
{
    m_isPopup = popup;
}

bool QTuiWidget::isPaintedByParent() const
{
    return m_paintedByParent;
}

void QTuiWidget::setPaintedByParent(bool paintedByParent)
{
    m_paintedByParent = paintedByParent;
}

bool QTuiWidget::underMouse() const
{
    return m_underMouse;
}

void QTuiWidget::setUnderMouse(bool under)
{
    if (m_underMouse == under)
        return;
    m_underMouse = under;
    markDirty();
}

Qt::ContextMenuPolicy QTuiWidget::contextMenuPolicy() const
{
    return m_contextMenuPolicy;
}

void QTuiWidget::setContextMenuPolicy(Qt::ContextMenuPolicy policy)
{
    m_contextMenuPolicy = policy;
}

void QTuiWidget::addAction(QTuiAction *action)
{
    if (action && !m_actions.contains(action))
        m_actions.append(action);
}

void QTuiWidget::removeAction(QTuiAction *action)
{
    m_actions.removeAll(action);
}

QList<QTuiAction *> QTuiWidget::actions() const
{
    QList<QTuiAction *> result;
    for (const auto &action : m_actions) {
        if (action)
            result.append(action);
    }
    return result;
}

QString QTuiWidget::toolTip() const
{
    return m_toolTip;
}

void QTuiWidget::setToolTip(const QString &toolTip)
{
    m_toolTip = toolTip;
}

void QTuiWidget::popupOutsideClick()
{
    setVisible(false);
}

bool QTuiWidget::isAncestorOf(const QTuiWidget *child) const
{
    for (const QTuiWidget *w = child; w; w = w->parentWidget()) {
        if (w == this)
            return true;
    }
    return false;
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
    return m_layout ? m_layout->minimumSize() : QSize(0, 0);
}

QSize QTuiWidget::sizeHint() const
{
    return minimumSize();
}

QTuiSizePolicy QTuiWidget::sizePolicy() const
{
    return m_sizePolicy;
}

void QTuiWidget::setSizePolicy(const QTuiSizePolicy &policy)
{
    m_sizePolicy = policy;
    if (auto *parent = parentWidget(); parent && parent->layout())
        parent->layout()->invalidate();
}

void QTuiWidget::setSizePolicy(QTuiSizePolicy::Policy horizontal, QTuiSizePolicy::Policy vertical)
{
    setSizePolicy(QTuiSizePolicy(horizontal, vertical));
}

void QTuiWidget::paintEvent(ScreenBuffer &buffer)
{
    if (!isVisible() || geometry().isEmpty())
        return;
    clearBackground(buffer, geometry());
}

void QTuiWidget::setLayout(QTuiLayout *layout)
{
    if (m_layout == layout)
        return;
    if (m_layout)
        disconnect(this, &QTuiWidget::geometryChanged, m_layout, nullptr);

    m_layout = layout;
    if (!m_layout)
        return;

    m_layout->setParent(this);
    connect(this, &QTuiWidget::geometryChanged, m_layout, [this](const QRect &geo) {
        m_layout->apply(geo);
    });
    if (!geometry().isEmpty())
        m_layout->apply(geometry());
    markDirty();
}

QTuiLayout *QTuiWidget::layout() const
{
    return m_layout;
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
            cell.setChar(U' ');
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
                    cell.setChar(U' ');
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
    // Effectively-disabled widgets are inert: key and mouse events are
    // consumed without reaching the handlers (resize events still apply).
    const bool interactive = event->type() == TuiEventTypes::keyEventType()
        || event->type() == TuiEventTypes::mouseEventType()
        || event->type() == TuiEventTypes::pasteEventType();
    if (interactive && !isEffectivelyEnabled())
        return true;

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
    if (event->type() == TuiEventTypes::pasteEventType()) {
        pasteEvent(static_cast<TuiPasteEvent *>(event));
        return true;
    }
    return QObject::event(event);
}

void QTuiWidget::keyPressEvent(TuiKeyEvent * /*event*/) {}
void QTuiWidget::mousePressEvent(TuiMouseEvent * /*event*/) {}
void QTuiWidget::resizeEvent(TuiResizeEvent * /*event*/) {}

void QTuiWidget::pasteEvent(TuiPasteEvent * /*event*/) {}

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
