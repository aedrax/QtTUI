#include <QtTUI/QTuiSizeGrip.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiSizeGrip::QTuiSizeGrip(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // One cell with a "╱"
    setGeometry(QRect(0, 0, 1, 1));
}

QTuiWidget *QTuiSizeGrip::findTopLevelWindow() const
{
    // Without a window, the topmost parent widget
    QTuiWidget *top = nullptr;
    for (QTuiWidget *w = parentWidget(); w; w = w->parentWidget()) {
        if (qobject_cast<QTuiMdiSubWindow *>(w) || qobject_cast<QTuiDialog *>(w)
            || qobject_cast<QTuiMainWindow *>(w))
            return w;
        top = w;
    }
    return top;
}

void QTuiSizeGrip::paintEvent(ScreenBuffer &buffer)
{
    if (!isVisible())
        return;

    const QRect geo = geometry();
    if (geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    const Color bg = hasFocus() ? theme.focusBackground : theme.background;

    // Clear the widget, then draw one "╱" in the bottom-right cell
    const Cell style = cellStyle(theme, fg, bg);
    fillRect(buffer, geo, style);
    buffer.putChar(geo.right(), geo.bottom(), U'╱', style);
}

QSize QTuiSizeGrip::minimumSize() const
{
    return QSize(1, 1);
}

void QTuiSizeGrip::keyPressEvent(TuiKeyEvent *event)
{
    QTuiWidget *window = findTopLevelWindow();
    if (!window) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    QSize size = window->geometry().size();
    switch (event->key()) {
    case Qt::Key_Right: size.rwidth() += 1; break;
    case Qt::Key_Left:  size.rwidth() -= 1; break;
    case Qt::Key_Down:  size.rheight() += 1; break;
    case Qt::Key_Up:    size.rheight() -= 1; break;
    default:
        QTuiWidget::keyPressEvent(event);
        return;
    }
    resizeWindow(window, size);
}

void QTuiSizeGrip::mousePressEvent(TuiMouseEvent *event)
{
    QTuiWidget *window = findTopLevelWindow();
    if (!window || event->button() != Qt::LeftButton)
        return;
    if (event->mouseType() == TuiMouseEvent::Press) {
        m_pressPos = event->pos();
        m_pressSize = window->geometry().size();
    } else if (event->mouseType() == TuiMouseEvent::Move) {
        const QPoint delta = event->pos() - m_pressPos;
        resizeWindow(window, m_pressSize + QSize(delta.x(), delta.y()));
    }
}

// The size stays at or above the minimum size of the window, and inside
// the parent of the window (the MDI area for a sub-window)
void QTuiSizeGrip::resizeWindow(QTuiWidget *window, const QSize &size)
{
    const QRect geo = window->geometry();
    QSize bounded = size.expandedTo(window->minimumSize());
    if (QTuiWidget *parent = window->parentWidget()) {
        const QRect area = parent->geometry();
        bounded = bounded.boundedTo(QSize(qMax(1, area.right() + 1 - geo.x()),
                                          qMax(1, area.bottom() + 1 - geo.y())));
    }
    if (bounded != geo.size())
        window->setGeometry(QRect(geo.topLeft(), bounded));
}

} // namespace QtTUI
