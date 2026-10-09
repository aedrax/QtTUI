#include <QtTUI/QTuiSizeGrip.h>
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
    // Default size: 3x3 for the diagonal grip pattern
    setGeometry(QRect(0, 0, 3, 3));
}

QTuiWidget *QTuiSizeGrip::findTopLevelWindow() const
{
    QTuiWidget *w = parentWidget();
    while (w) {
        if (qobject_cast<QTuiMdiSubWindow *>(w) || qobject_cast<QTuiDialog *>(w)
            || qobject_cast<QTuiMainWindow *>(w))
            return w;
        w = w->parentWidget();
    }
    // If no QTuiMainWindow found, return the topmost parent widget
    w = parentWidget();
    if (!w)
        return nullptr;
    while (w->parentWidget())
        w = w->parentWidget();
    return w;
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

    auto setCell = [&](int x, int y, char32_t ch) {
        if (x >= 0 && x < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(x, y);
            cell.setChar(ch);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    };

    // Render a diagonal resize handle pattern in the widget area
    // Using slash characters to suggest a draggable corner grip:
    //   . . /
    //   . / .
    //   / . .
    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // Fill background
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            setCell(x0 + col, y0 + row, U' ');
        }
    }

    // Draw diagonal lines from bottom-left to top-right
    const int diag = qMin(w, h);
    for (int i = 0; i < diag; ++i) {
        setCell(x0 + w - 1 - i, y0 + h - 1 - i, char32_t(0x2571)); // ╱
    }
}

QSize QTuiSizeGrip::minimumSize() const
{
    return QSize(3, 3);
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
