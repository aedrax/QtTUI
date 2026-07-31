#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
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

    const auto theme = themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);
    const Color fg = theme.foreground;
    const Color bg = theme.background;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // --- Title bar (row 0) ---
    for (int col = 0; col < w; ++col) {
        int px = x0 + col;
        if (px >= buffer.width() || y0 >= buffer.height())
            continue;
        Cell &cell = buffer.backCell(px, y0);
        if (col == 0)
            cell.setChar(border.topLeft);
        else if (col == w - 1)
            cell.setChar(border.topRight);
        else
            cell.setChar(border.horizontal);
        cell.foreground = fg;
        cell.background = bg;
        cell.bold = theme.bold;
    }

    // Write title text starting at col 2
    if (!m_windowTitle.isEmpty() && w > 4) {
        int indicatorWidth = 7; // " [□][X]" or " [▫][X]"
        int maxTitleLen = w - 4 - indicatorWidth;
        if (maxTitleLen < 0)
            maxTitleLen = 0;
        QString titleStr = m_windowTitle.left(maxTitleLen);

        int titleCol = 2;
        for (int i = 0; i < titleStr.length(); ++i) {
            int px = x0 + titleCol + i;
            if (px < buffer.width() && y0 < buffer.height()) {
                Cell &cell = buffer.backCell(px, y0);
                cell.setChar(titleStr.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = true;
            }
        }
    }

    // Write maximize/restore indicator and close indicator near right edge
    if (w >= 10) {
        // Maximize/restore indicator: [□] when normal, [▫] when maximized
        QString maxStr = m_maximized ? QStringLiteral("[▫]") : QStringLiteral("[□]");
        QString closeStr = QStringLiteral("[X]");

        int closeStart = x0 + w - 1 - closeStr.length();
        int maxStart = closeStart - maxStr.length();

        // Draw maximize/restore indicator
        for (int i = 0; i < maxStr.length(); ++i) {
            int px = maxStart + i;
            if (px >= x0 + 1 && px < buffer.width() && y0 < buffer.height()) {
                Cell &cell = buffer.backCell(px, y0);
                cell.setChar(maxStr.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
            }
        }

        // Draw close indicator
        for (int i = 0; i < closeStr.length(); ++i) {
            int px = closeStart + i;
            if (px >= x0 + 1 && px < buffer.width() && y0 < buffer.height()) {
                Cell &cell = buffer.backCell(px, y0);
                cell.setChar(closeStr.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
            }
        }
    }

    // --- Bottom border ---
    int by = y0 + h - 1;
    if (by < buffer.height()) {
        for (int col = 0; col < w; ++col) {
            int px = x0 + col;
            if (px >= buffer.width())
                continue;
            Cell &cell = buffer.backCell(px, by);
            if (col == 0)
                cell.setChar(border.bottomLeft);
            else if (col == w - 1)
                cell.setChar(border.bottomRight);
            else
                cell.setChar(border.horizontal);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }

    // --- Left and right borders (interior rows) ---
    for (int row = 1; row < h - 1; ++row) {
        int py = y0 + row;
        if (py >= buffer.height())
            continue;
        if (x0 < buffer.width()) {
            Cell &cell = buffer.backCell(x0, py);
            cell.setChar(border.vertical);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
        int rx = x0 + w - 1;
        if (rx < buffer.width()) {
            Cell &cell = buffer.backCell(rx, py);
            cell.setChar(border.vertical);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }

    // --- Clear interior ---
    for (int row = 1; row < h - 1; ++row) {
        for (int col = 1; col < w - 1; ++col) {
            int px = x0 + col;
            int py = y0 + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
            }
        }
    }

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

    // Escape or Q: close the subwindow
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        close();
        return;
    }

    QTuiWidget::keyPressEvent(event);
}

} // namespace QtTUI
