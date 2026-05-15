#include <QtTUI/QTuiMdiArea.h>
#include <QtTUI/QTuiMdiSubWindow.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QCoreApplication>
#include <QtMath>

namespace QtTUI {

QTuiMdiArea::QTuiMdiArea(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

QTuiMdiSubWindow *QTuiMdiArea::addSubWindow(QTuiWidget *widget)
{
    auto *subWindow = new QTuiMdiSubWindow(this);
    subWindow->setWidget(widget);
    if (widget)
        subWindow->setWindowTitle(widget->objectName());

    // Give it a default size and position
    const QRect geo = geometry();
    int defaultW = qMax(10, geo.width() / 2);
    int defaultH = qMax(5, geo.height() / 2);
    int offsetX = geo.x() + m_subWindows.size() * 2;
    int offsetY = geo.y() + m_subWindows.size();
    subWindow->setGeometry(QRect(offsetX, offsetY, defaultW, defaultH));

    m_subWindows.append(subWindow);

    // Activate the newly added window
    setActiveSubWindow(subWindow);

    return subWindow;
}

void QTuiMdiArea::removeSubWindow(QTuiWidget *widget)
{
    for (int i = 0; i < m_subWindows.size(); ++i) {
        QTuiMdiSubWindow *sub = m_subWindows.at(i);
        if (sub->widget() == widget || sub == widget) {
            m_subWindows.removeAt(i);
            if (m_activeWindow == sub) {
                m_activeWindow = m_subWindows.isEmpty() ? nullptr : m_subWindows.last();
                emit subWindowActivated(m_activeWindow);
            }
            sub->setVisible(false);
            return;
        }
    }
}

QList<QTuiMdiSubWindow *> QTuiMdiArea::subWindowList() const
{
    return m_subWindows;
}

QTuiMdiSubWindow *QTuiMdiArea::activeSubWindow() const
{
    return m_activeWindow;
}

void QTuiMdiArea::setActiveSubWindow(QTuiMdiSubWindow *window)
{
    if (m_activeWindow == window)
        return;

    if (window)
        emit window->aboutToActivate();

    m_activeWindow = window;
    emit subWindowActivated(m_activeWindow);
}

void QTuiMdiArea::cascadeSubWindows()
{
    const QRect area = geometry();
    const int cascadeOffsetX = 2;
    const int cascadeOffsetY = 1;

    // All subwindows get the same size
    int count = m_subWindows.size();
    if (count == 0)
        return;

    int subW = qMax(10, area.width() - (count - 1) * cascadeOffsetX);
    int subH = qMax(5, area.height() - (count - 1) * cascadeOffsetY);

    for (int i = 0; i < count; ++i) {
        int x = area.x() + i * cascadeOffsetX;
        int y = area.y() + i * cascadeOffsetY;
        m_subWindows[i]->setGeometry(QRect(x, y, subW, subH));
    }
}

void QTuiMdiArea::tileSubWindows()
{
    const QRect area = geometry();
    int count = m_subWindows.size();
    if (count == 0)
        return;

    // Compute grid dimensions
    int cols = qCeil(qSqrt(static_cast<double>(count)));
    int rows = (count + cols - 1) / cols;

    int cellW = area.width() / cols;
    int cellH = area.height() / rows;
    cellW = qMax(cellW, 4);
    cellH = qMax(cellH, 3);

    for (int i = 0; i < count; ++i) {
        int row = i / cols;
        int col = i % cols;
        int x = area.x() + col * cellW;
        int y = area.y() + row * cellH;
        m_subWindows[i]->setGeometry(QRect(x, y, cellW, cellH));
    }
}

void QTuiMdiArea::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color bg = theme.background;

    // Clear the MDI area background
    for (int row = 0; row < geo.height(); ++row) {
        for (int col = 0; col < geo.width(); ++col) {
            int px = geo.x() + col;
            int py = geo.y() + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = theme.foreground;
                cell.background = bg;
                cell.bold = false;
            }
        }
    }

    // Paint each subwindow; active window is painted last (on top)
    for (auto *sub : m_subWindows) {
        if (sub != m_activeWindow && sub->isVisible())
            sub->paintEvent(buffer);
    }
    if (m_activeWindow && m_activeWindow->isVisible())
        m_activeWindow->paintEvent(buffer);
}

void QTuiMdiArea::keyPressEvent(TuiKeyEvent *event)
{
    // Tab: switch to next subwindow
    if (event->key() == Qt::Key_Tab && !m_subWindows.isEmpty()) {
        int idx = m_subWindows.indexOf(m_activeWindow);
        int next = (idx + 1) % m_subWindows.size();
        setActiveSubWindow(m_subWindows.at(next));
        return;
    }

    // Forward key events to active subwindow
    if (m_activeWindow) {
        QCoreApplication::sendEvent(m_activeWindow, event);
        return;
    }

    QTuiWidget::keyPressEvent(event);
}

} // namespace QtTUI
