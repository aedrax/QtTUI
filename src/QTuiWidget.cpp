#include <QtTUI/QTuiWidget.h>
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
        emit prev->focusLost();
    }
    if (widget) {
        widget->m_hasFocus = true;
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
}

} // namespace QtTUI
