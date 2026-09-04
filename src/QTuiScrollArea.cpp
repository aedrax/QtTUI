#include <QtTUI/QTuiScrollArea.h>
#include <QtTUI/RenderUtils.h>
#include <QCoreApplication>
#include <functional>
#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

#include <algorithm>

namespace QtTUI {

QTuiScrollArea::QTuiScrollArea(QTuiWidget *parent)
    : QTuiAbstractScrollArea(parent)
    , m_verticalScrollBar(new QTuiScrollBar(Qt::Vertical, this))
    , m_horizontalScrollBar(new QTuiScrollBar(Qt::Horizontal, this))
{
    setFocusPolicy(Qt::StrongFocus);
    // Connect scroll bar value changes to update scroll offsets
    connect(m_verticalScrollBar, &QTuiScrollBar::valueChanged, this, [this](int value) {
        m_scrollOffset = value;
        markDirty();
    });
    connect(m_horizontalScrollBar, &QTuiScrollBar::valueChanged, this, [this](int value) {
        m_scrollX = value;
        markDirty();
    });

    // Hide scroll bars initially
    m_verticalScrollBar->setVisible(false);
    m_horizontalScrollBar->setVisible(false);
}

void QTuiScrollArea::setWidget(QTuiWidget *widget)
{
    if (m_widget == widget)
        return;

    if (m_widget)
        m_widget->setPaintedByParent(false);
    m_widget = widget;
    if (m_widget) {
        // The content geometry is relative to the scroll area, and
        // paintEvent() paints the content through a separate buffer
        m_widget->setPaintedByParent(true);
        m_widget->setVisible(true);
    }
    m_scrollX = 0;
    m_scrollOffset = 0;
    updateScrollBars();
    markDirty();
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
    if (y < m_scrollOffset) {
        m_scrollOffset = y;
    } else if (y >= m_scrollOffset + viewportH) {
        m_scrollOffset = y - viewportH + 1;
    }

    m_scrollX = qMax(0, m_scrollX);
    m_scrollOffset = qMax(0, m_scrollOffset);

    m_horizontalScrollBar->setValue(m_scrollX);
    m_verticalScrollBar->setValue(m_scrollOffset);
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
        m_scrollOffset = qBound(0, m_scrollOffset, qMax(0, childH - viewportH));
        m_verticalScrollBar->setValue(m_scrollOffset);
    } else {
        m_scrollOffset = 0;
    }

    if (needsHScroll) {
        m_scrollX = qBound(0, m_scrollX, qMax(0, childW - viewportW));
        m_horizontalScrollBar->setValue(m_scrollX);
    } else {
        m_scrollX = 0;
    }
}

QRect QTuiScrollArea::contentArea() const
{
    const QRect geo = geometry();
    if (geo.isEmpty())
        return QRect();

    // Content area (viewport) is the area minus scroll bars
    int childW = 0;
    int childH = 0;
    if (m_widget) {
        const QRect childGeo = m_widget->geometry();
        childW = childGeo.width();
        childH = childGeo.height();
    }

    bool needsVScroll = childH > geo.height();
    bool needsHScroll = childW > geo.width();

    if (needsVScroll && !needsHScroll) {
        needsHScroll = childW > (geo.width() - 1);
    }
    if (needsHScroll && !needsVScroll) {
        needsVScroll = childH > (geo.height() - 1);
    }

    int viewportW = geo.width() - (needsVScroll ? 1 : 0);
    int viewportH = geo.height() - (needsHScroll ? 1 : 0);

    return QRect(geo.x(), geo.y(), viewportW, viewportH);
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
    fillRect(buffer, QRect(geo.x(), geo.y(), viewportW, viewportH), cellStyle(fg, bg));

    // Render the clipped portion of the child widget into a temporary buffer,
    // then copy the visible region into the main buffer
    if (m_widget && m_widget->isVisible()) {
        const QRect childGeo = m_widget->geometry();
        if (childGeo.width() > 0 && childGeo.height() > 0) {
            // Paint the content and all widgets in it into a temporary buffer
            // at their own (content) coordinates. Geometry does not change
            // during the paint, so no repaint is scheduled from here.
            const int left = qMax(0, childGeo.x());
            const int top = qMax(0, childGeo.y());
            ScreenBuffer childBuffer(left + childGeo.width(), top + childGeo.height());
            std::function<void(QTuiWidget *)> paintSubtree = [&](QTuiWidget *w) {
                w->paintEvent(childBuffer);
                for (QTuiWidget *child : w->childWidgets()) {
                    if (child->isVisible() && !child->isPopup())
                        paintSubtree(child);
                }
            };
            paintSubtree(m_widget);

            // Copy the visible portion (clipped by viewport) from child buffer
            for (int row = 0; row < viewportH; ++row) {
                int childRow = row + m_scrollOffset;
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

                    const Cell &src = childBuffer.backCell(left + childCol, top + childRow);
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

    // Render scroll indicators (arrows) when content exceeds viewport
    if (needsVScroll) {
        // Up arrow indicator at top-right
        int indicatorX = geo.x() + geo.width() - 1;
        int topY = geo.y();
        if (indicatorX < buffer.width() && topY < buffer.height() && m_scrollOffset > 0) {
            Cell &cell = buffer.backCell(indicatorX, topY);
            cell.setChar(0x25B2); // ▲
            cell.foreground = fg;
            cell.background = bg;
        }
        // Down arrow indicator at bottom-right (above hscroll if present)
        int bottomY = geo.y() + viewportH - 1;
        int childH = m_widget ? m_widget->geometry().height() : 0;
        if (indicatorX < buffer.width() && bottomY < buffer.height()
            && m_scrollOffset < qMax(0, childH - viewportH)) {
            Cell &cell = buffer.backCell(indicatorX, bottomY);
            cell.setChar(0x25BC); // ▼
            cell.foreground = fg;
            cell.background = bg;
        }
    }
    if (needsHScroll) {
        // Left arrow indicator at bottom-left
        int indicatorY = geo.y() + geo.height() - 1;
        int leftX = geo.x();
        if (leftX < buffer.width() && indicatorY < buffer.height() && m_scrollX > 0) {
            Cell &cell = buffer.backCell(leftX, indicatorY);
            cell.setChar(0x25C4); // ◄
            cell.foreground = fg;
            cell.background = bg;
        }
        // Right arrow indicator at bottom-right (before vscroll if present)
        int rightX = geo.x() + viewportW - 1;
        int childW = m_widget ? m_widget->geometry().width() : 0;
        if (rightX < buffer.width() && indicatorY < buffer.height()
            && m_scrollX < qMax(0, childW - viewportW)) {
            Cell &cell = buffer.backCell(rightX, indicatorY);
            cell.setChar(0x25BA); // ►
            cell.foreground = fg;
            cell.background = bg;
        }
    }

    // Fill corner cell if both scroll bars are visible
    if (needsVScroll && needsHScroll)
        fillRect(buffer, QRect(geo.right(), geo.bottom(), 1, 1), cellStyle(fg, bg));
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
            m_verticalScrollBar->setValue(m_scrollOffset - 1);
        break;
    case Qt::Key_Down:
        if (needsVScroll)
            m_verticalScrollBar->setValue(m_scrollOffset + 1);
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
            m_verticalScrollBar->setValue(m_scrollOffset - viewportH);
        break;
    case Qt::Key_PageDown:
        if (needsVScroll)
            m_verticalScrollBar->setValue(m_scrollOffset + viewportH);
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

void QTuiScrollArea::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() == TuiMouseEvent::Wheel) {
        // 3 lines per notch, as is usual
        const int lines = event->wheelDelta() > 0 ? -3 : 3;
        if (m_verticalScrollBar->isVisible())
            m_verticalScrollBar->setValue(m_verticalScrollBar->value() + lines);
        return;
    }
    if (!m_widget || !m_widget->isVisible())
        return;

    // Viewport position -> content position
    const QPoint content = event->pos() - geometry().topLeft() + QPoint(m_scrollX, m_scrollOffset);
    if (!m_widget->geometry().contains(content))
        return;

    // The deepest visible content widget at that position
    QTuiWidget *target = m_widget;
    for (bool found = true; found;) {
        found = false;
        const auto children = target->childWidgets();
        for (auto it = children.crbegin(); it != children.crend(); ++it) {
            QTuiWidget *child = *it;
            if (child->isVisible() && !child->isPopup() && child->geometry().contains(content)) {
                target = child;
                found = true;
                break;
            }
        }
    }

    if (event->mouseType() == TuiMouseEvent::Press && (target->focusPolicy() & Qt::ClickFocus)
        && target->isEffectivelyEnabled())
        target->setFocus();
    TuiMouseEvent local(event->mouseType(), event->button(), content, event->modifiers(),
                        event->wheelDelta());
    QCoreApplication::sendEvent(target, &local);
}

} // namespace QtTUI
