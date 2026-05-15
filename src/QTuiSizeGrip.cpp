#include <QtTUI/QTuiSizeGrip.h>
#include <QtTUI/QTuiMainWindow.h>
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
        if (qobject_cast<QTuiMainWindow *>(w))
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
    QTuiWidget *topLevel = findTopLevelWindow();
    if (!topLevel) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    const QRect geo = topLevel->geometry();
    const QSize minSize = topLevel->minimumSize();
    QRect newGeo = geo;

    switch (event->key()) {
    case Qt::Key_Right:
        newGeo.setWidth(geo.width() + 1);
        break;
    case Qt::Key_Left:
        if (geo.width() > minSize.width())
            newGeo.setWidth(geo.width() - 1);
        break;
    case Qt::Key_Down:
        newGeo.setHeight(geo.height() + 1);
        break;
    case Qt::Key_Up:
        if (geo.height() > minSize.height())
            newGeo.setHeight(geo.height() - 1);
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        return;
    }

    if (newGeo != geo)
        topLevel->setGeometry(newGeo);
}

} // namespace QtTUI
