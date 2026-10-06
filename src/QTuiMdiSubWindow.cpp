#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiMdiSubWindow::QTuiMdiSubWindow(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void QTuiMdiSubWindow::setWidget(QTuiWidget *widget)
{
    m_widget = widget;
    if (m_widget)
        m_widget->setParent(this);
}

QTuiWidget *QTuiMdiSubWindow::widget() const
{
    return m_widget;
}

QString QTuiMdiSubWindow::windowTitle() const
{
    return m_windowTitle;
}

void QTuiMdiSubWindow::setWindowTitle(const QString &title)
{
    m_windowTitle = title;
}

bool QTuiMdiSubWindow::isMaximized() const
{
    return m_maximized;
}

void QTuiMdiSubWindow::showMaximized()
{
    if (m_maximized)
        return;

    Qt::WindowStates oldState = Qt::WindowNoState;
    m_normalGeometry = geometry();
    m_maximized = true;

    // Expand to fill parent area
    if (parentWidget()) {
        QRect parentGeo = parentWidget()->geometry();
        setGeometry(QRect(parentGeo.x(), parentGeo.y(),
                          parentGeo.width(), parentGeo.height()));
    }

    emit windowStateChanged(oldState, Qt::WindowMaximized);
}

void QTuiMdiSubWindow::showNormal()
{
    if (!m_maximized)
        return;

    Qt::WindowStates oldState = Qt::WindowMaximized;
    m_maximized = false;

    if (m_normalGeometry.isValid())
        setGeometry(m_normalGeometry);

    emit windowStateChanged(oldState, Qt::WindowNoState);
}

void QTuiMdiSubWindow::mousePressEvent(TuiMouseEvent *event)
{
    const QRect geo = geometry();
    const QPoint pos = event->pos();

    if (event->mouseType() == TuiMouseEvent::Release) {
        m_drag = Drag::None;
        return;
    }

    if (event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton) {
        // Keep the window inside the MDI area
        const QRect area = parentWidget() ? parentWidget()->geometry() : QRect(geo.topLeft(), geo.size());
        if (m_drag == Drag::Move) {
            QRect moved(pos - m_dragOffset, geo.size());
            moved.moveLeft(qBound(area.left(), moved.left(), qMax(area.left(), area.right() + 1 - moved.width())));
            moved.moveTop(qBound(area.top(), moved.top(), qMax(area.top(), area.bottom() + 1 - moved.height())));
            setGeometry(moved);
        } else if (m_drag == Drag::Resize) {
            const QSize min = minimumSize();
            const int w = qBound(min.width(), pos.x() - geo.x() + 1, area.right() + 1 - geo.x());
            const int h = qBound(min.height(), pos.y() - geo.y() + 1, area.bottom() + 1 - geo.y());
            setGeometry(QRect(geo.topLeft(), QSize(w, h)));
        }
        return;
    }

    if (event->mouseType() != TuiMouseEvent::Press || event->button() != Qt::LeftButton)
        return;

    if (auto *area = qobject_cast<QTuiMdiArea *>(parentWidget()))
        area->setActiveSubWindow(this);
    markDirty();

    const int closeStart = geo.x() + geo.width() - 1 - 3; // "[X]"
    const int maxStart = closeStart - 3;                    // "[□]"
    if (pos.y() == geo.y()) {
        if (pos.x() >= closeStart && pos.x() < closeStart + 3) {
            close();
        } else if (pos.x() >= maxStart && pos.x() < maxStart + 3) {
            m_maximized ? showNormal() : showMaximized();
        } else if (!m_maximized) {
            m_drag = Drag::Move;
            m_dragOffset = pos - geo.topLeft();
        }
        return;
    }
    if (!m_maximized && pos == QPoint(geo.right(), geo.bottom()))
        m_drag = Drag::Resize;
}

void QTuiMdiSubWindow::close()
{
    setVisible(false);
}

QSize QTuiMdiSubWindow::minimumSize() const
{
    // Title bar row + at least 1 content row + bottom border = 3 rows
    // Width: border + title + indicators "[□]" "[X]" + border
    int titleWidth = m_windowTitle.length() + 10; // " title [□][X]" with borders
    int contentWidth = 4; // minimum 2 interior cols + 2 border cols
    if (m_widget) {
        QSize ws = m_widget->minimumSize();
        contentWidth = ws.width() + 2;
    }
    return QSize(qMax(titleWidth, contentWidth), 3);
}

void QTuiMdiSubWindow::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    // Maximize/restore indicator: [□] when normal, [▫] when maximized
    drawWindowFrame(buffer, geo, themeProperties(), m_windowTitle,
                    (m_maximized ? QStringLiteral("[▫]") : QStringLiteral("[□]")) + QStringLiteral("[X]"));

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // --- Paint content widget ---
    if (m_widget && m_widget->isVisible()) {
        QRect contentRect(x0 + 1, y0 + 1, w - 2, h - 2);
        m_widget->setGeometry(contentRect);
        m_widget->paintEvent(buffer);
    }
}

void QTuiMdiSubWindow::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();

    // Ctrl+Arrow: move subwindow within parent
    if (mods & Qt::ControlModifier) {
        QRect geo = geometry();
        switch (key) {
        case Qt::Key_Left:
            geo.moveLeft(qMax(0, geo.x() - 1));
            setGeometry(geo);
            return;
        case Qt::Key_Right:
            geo.moveLeft(geo.x() + 1);
            setGeometry(geo);
            return;
        case Qt::Key_Up:
            geo.moveTop(qMax(0, geo.y() - 1));
            setGeometry(geo);
            return;
        case Qt::Key_Down:
            geo.moveTop(geo.y() + 1);
            setGeometry(geo);
            return;
        default:
            break;
        }
    }

    // Shift+Arrow: resize subwindow
    if (mods & Qt::ShiftModifier) {
        QRect geo = geometry();
        QSize minSz = minimumSize();
        switch (key) {
        case Qt::Key_Left:
            if (geo.width() > minSz.width())
                geo.setWidth(geo.width() - 1);
            setGeometry(geo);
            return;
        case Qt::Key_Right:
            geo.setWidth(geo.width() + 1);
            setGeometry(geo);
            return;
        case Qt::Key_Up:
            if (geo.height() > minSz.height())
                geo.setHeight(geo.height() - 1);
            setGeometry(geo);
            return;
        case Qt::Key_Down:
            geo.setHeight(geo.height() + 1);
            setGeometry(geo);
            return;
        default:
            break;
        }
    }

    // M: maximize or restore, as the [□] button does
    if (key == Qt::Key_M && !(mods & (Qt::ControlModifier | Qt::AltModifier))) {
        m_maximized ? showNormal() : showMaximized();
        return;
    }

    // Escape or Q: close the subwindow
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        close();
        return;
    }

    QTuiWidget::keyPressEvent(event);
}

void QTuiMdiSubWindow::focusEntered(QTuiWidget *focus)
{
    Q_UNUSED(focus);
    if (auto *area = qobject_cast<QTuiMdiArea *>(parentWidget()))
        area->setActiveSubWindow(this);
}

} // namespace QtTUI
