#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

#include <algorithm>

namespace QtTUI {

QTuiScrollArea::QTuiScrollArea(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_verticalScrollBar(new QTuiScrollBar(Qt::Vertical, this))
    , m_horizontalScrollBar(new QTuiScrollBar(Qt::Horizontal, this))
{
    // Connect scroll bar value changes to update scroll offsets
    connect(m_verticalScrollBar, &QTuiScrollBar::valueChanged, this, [this](int value) {
        m_scrollY = value;
    });
    connect(m_horizontalScrollBar, &QTuiScrollBar::valueChanged, this, [this](int value) {
        m_scrollX = value;
    });

    // Hide scroll bars initially
    m_verticalScrollBar->setVisible(false);
    m_horizontalScrollBar->setVisible(false);
}

void QTuiScrollArea::setWidget(QTuiWidget *widget)
{
    if (m_widget == widget)
        return;

    m_widget = widget;
    if (m_widget) {
        m_widget->setVisible(true);
    }
    m_scrollX = 0;
    m_scrollY = 0;
    updateScrollBars();
}

QTuiWidget *QTuiScrollArea::widget() const
{
    return m_widget;
}

QTuiScrollBar *QTuiScrollArea::verticalScrollBar() const
{
    return m_verticalScrollBar;
}

QTuiScrollBar *QTuiScrollArea::horizontalScrollBar() const
{
    return m_horizontalScrollBar;
}

void QTuiScrollArea::ensureVisible(int x, int y)
{
    const QRect geo = geometry();
    const bool needsVScroll = m_verticalScrollBar->isVisible();
    const bool needsHScroll = m_horizontalScrollBar->isVisible();

    const int viewportW = geo.width() - (needsVScroll ? 1 : 0);
    const int viewportH = geo.height() - (needsHScroll ? 1 : 0);

    // Adjust horizontal scroll so x is visible
    if (x < m_scrollX) {
        m_scrollX = x;
    } else if (x >= m_scrollX + viewportW) {
        m_scrollX = x - viewportW + 1;
    }

    // Adjust vertical scroll so y is visible
    if (y < m_scrollY) {
        m_scrollY = y;
    } else if (y >= m_scrollY + viewportH) {
        m_scrollY = y - viewportH + 1;
    }

    m_scrollX = qMax(0, m_scrollX);
    m_scrollY = qMax(0, m_scrollY);

    m_horizontalScrollBar->setValue(m_scrollX);
    m_verticalScrollBar->setValue(m_scrollY);
}

QSize QTuiScrollArea::minimumSize() const
{
    return QSize(3, 3);
}

void QTuiScrollArea::updateScrollBars()
{
    const QRect geo = geometry();
    if (geo.isEmpty())
        return;

    int areaW = geo.width();
    int areaH = geo.height();

    int childW = 0;
    int childH = 0;
    if (m_widget) {
        const QRect childGeo = m_widget->geometry();
        childW = childGeo.width();
        childH = childGeo.height();
    }

    // Determine which scroll bars are needed
    bool needsVScroll = childH > areaH;
    bool needsHScroll = childW > areaW;

    // Re-check: if vertical scroll bar is shown, horizontal viewport shrinks
    if (needsVScroll && !needsHScroll) {
        needsHScroll = childW > (areaW - 1);
    }
    // If horizontal scroll bar is shown, vertical viewport shrinks
    if (needsHScroll && !needsVScroll) {
        needsVScroll = childH > (areaH - 1);
    }

    int viewportW = areaW - (needsVScroll ? 1 : 0);
    int viewportH = areaH - (needsHScroll ? 1 : 0);

    m_verticalScrollBar->setVisible(needsVScroll);
    m_horizontalScrollBar->setVisible(needsHScroll);

    if (needsVScroll) {
        m_verticalScrollBar->setGeometry(QRect(
            geo.x() + areaW - 1, geo.y(), 1, viewportH));
        m_verticalScrollBar->setRange(0, qMax(0, childH - viewportH));
        m_verticalScrollBar->setPageStep(viewportH);
        m_verticalScrollBar->setSingleStep(1);
    }

    if (needsHScroll) {
        m_horizontalScrollBar->setGeometry(QRect(
            geo.x(), geo.y() + areaH - 1, viewportW, 1));
        m_horizontalScrollBar->setRange(0, qMax(0, childW - viewportW));
        m_horizontalScrollBar->setPageStep(viewportW);
        m_horizontalScrollBar->setSingleStep(1);
    }

    // Clamp scroll offsets
    if (needsVScroll) {
        m_scrollY = qBound(0, m_scrollY, qMax(0, childH - viewportH));
        m_verticalScrollBar->setValue(m_scrollY);
    } else {
        m_scrollY = 0;
    }

    if (needsHScroll) {
        m_scrollX = qBound(0, m_scrollX, qMax(0, childW - viewportW));
        m_horizontalScrollBar->setValue(m_scrollX);
    } else {
        m_scrollX = 0;
    }
}

void QTuiScrollArea::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    updateScrollBars();

    const auto theme = themeProperties();
    const bool needsVScroll = m_verticalScrollBar->isVisible();
    const bool needsHScroll = m_horizontalScrollBar->isVisible();

    const int viewportW = geo.width() - (needsVScroll ? 1 : 0);
    const int viewportH = geo.height() - (needsHScroll ? 1 : 0);

    const Color fg = theme.foreground;
    const Color bg = theme.background;

    // Clear the viewport area with background
    for (int row = 0; row < viewportH; ++row) {
        for (int col = 0; col < viewportW; ++col) {
            int px = geo.x() + col;
            int py = geo.y() + row;
            if (px >= buffer.width() || py >= buffer.height())
                continue;
            Cell &cell = buffer.backCell(px, py);
            cell.character = QLatin1Char(' ');
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;
        }
    }

    // Render the clipped portion of the child widget into a temporary buffer,
    // then copy the visible region into the main buffer
    if (m_widget && m_widget->isVisible()) {
        const QRect childGeo = m_widget->geometry();
        if (childGeo.width() > 0 && childGeo.height() > 0) {
            // Create a temporary buffer for the child to paint into
            ScreenBuffer childBuffer(childGeo.width(), childGeo.height());

            // Temporarily adjust child geometry to (0,0) origin for painting
            QRect savedGeo = m_widget->geometry();
            m_widget->setGeometry(QRect(0, 0, childGeo.width(), childGeo.height()));
            m_widget->paintEvent(childBuffer);
            m_widget->setGeometry(savedGeo);

            // Copy the visible portion (clipped by viewport) from child buffer
            for (int row = 0; row < viewportH; ++row) {
                int childRow = row + m_scrollY;
                if (childRow < 0 || childRow >= childGeo.height())
                    continue;

                for (int col = 0; col < viewportW; ++col) {
                    int childCol = col + m_scrollX;
                    if (childCol < 0 || childCol >= childGeo.width())
                        continue;

                    int px = geo.x() + col;
                    int py = geo.y() + row;
                    if (px >= buffer.width() || py >= buffer.height())
                        continue;

                    const Cell &src = childBuffer.backCell(childCol, childRow);
                    Cell &dst = buffer.backCell(px, py);
                    dst = src;
                }
            }
        }
    }

    // Paint scroll bars (they paint themselves into the main buffer)
    if (needsVScroll)
        m_verticalScrollBar->paintEvent(buffer);
    if (needsHScroll)
        m_horizontalScrollBar->paintEvent(buffer);

    // Fill corner cell if both scroll bars are visible
    if (needsVScroll && needsHScroll) {
        int cx = geo.x() + geo.width() - 1;
        int cy = geo.y() + geo.height() - 1;
        if (cx < buffer.width() && cy < buffer.height()) {
            Cell &corner = buffer.backCell(cx, cy);
            corner.character = QLatin1Char(' ');
            corner.foreground = fg;
            corner.background = bg;
        }
    }
}

void QTuiScrollArea::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();

    const QRect geo = geometry();
    const bool needsVScroll = m_verticalScrollBar->isVisible();
    const bool needsHScroll = m_horizontalScrollBar->isVisible();
    const int viewportH = geo.height() - (needsHScroll ? 1 : 0);

    int childH = 0;
    int childW = 0;
    if (m_widget) {
        const QRect childGeo = m_widget->geometry();
        childH = childGeo.height();
        childW = childGeo.width();
    }

    const int maxScrollY = needsVScroll ? m_verticalScrollBar->maximum() : 0;
    const int maxScrollX = needsHScroll ? m_horizontalScrollBar->maximum() : 0;

    switch (key) {
    case Qt::Key_Up:
        if (needsVScroll)
            m_verticalScrollBar->setValue(m_scrollY - 1);
        break;
    case Qt::Key_Down:
        if (needsVScroll)
            m_verticalScrollBar->setValue(m_scrollY + 1);
        break;
    case Qt::Key_Left:
        if (needsHScroll)
            m_horizontalScrollBar->setValue(m_scrollX - 1);
        break;
    case Qt::Key_Right:
        if (needsHScroll)
            m_horizontalScrollBar->setValue(m_scrollX + 1);
        break;
    case Qt::Key_PageUp:
        if (needsVScroll)
            m_verticalScrollBar->setValue(m_scrollY - viewportH);
        break;
    case Qt::Key_PageDown:
        if (needsVScroll)
            m_verticalScrollBar->setValue(m_scrollY + viewportH);
        break;
    case Qt::Key_Home:
        if (needsVScroll)
            m_verticalScrollBar->setValue(0);
        if (needsHScroll)
            m_horizontalScrollBar->setValue(0);
        break;
    case Qt::Key_End:
        if (needsVScroll)
            m_verticalScrollBar->setValue(maxScrollY);
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        break;
    }
}

void QTuiScrollArea::resizeEvent(TuiResizeEvent *event)
{
    Q_UNUSED(event);
    updateScrollBars();
}

} // namespace QtTUI
