#include <QtTUI/QTuiSlider.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiSlider::QTuiSlider(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiAbstractSlider(orientation, parent)
{
}

int QTuiSlider::tickInterval() const
{
    return m_tickInterval;
}

void QTuiSlider::setTickInterval(int interval)
{
    int clamped = (interval >= 0) ? interval : 0;
    if (m_tickInterval == clamped)
        return;
    m_tickInterval = clamped;
    markDirty();
}

QSize QTuiSlider::minimumSize() const
{
    if (m_orientation == Qt::Horizontal) {
        int height = (m_tickInterval > 0) ? 2 : 1;
        return QSize(10, height);
    } else {
        int width = (m_tickInterval > 0) ? 3 : 1;
        return QSize(width, 5);
    }
}

void QTuiSlider::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();

    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    // Clear the entire widget area
    fillRect(buffer, geo, cellStyle(fg, bg));

    int range = m_maximum - m_minimum;

    if (m_orientation == Qt::Horizontal) {
        bool hasTicks = (m_tickInterval > 0) && (geo.height() >= 2);
        int trackRow = hasTicks ? 1 : 0;

        // Compute indicator position on the track
        int trackWidth = geo.width();
        int indicatorPos = 0;
        if (range > 0 && trackWidth > 1)
            indicatorPos = static_cast<int>(static_cast<double>(m_value - m_minimum) / range * (trackWidth - 1) + 0.5);
        indicatorPos = qBound(0, indicatorPos, trackWidth - 1);

        // Render tick marks row
        if (hasTicks) {
            int py = geo.y();
            for (int col = 0; col < trackWidth; ++col) {
                int px = geo.x() + col;
                if (px >= buffer.width() || py >= buffer.height())
                    continue;

                // Determine if this column corresponds to a tick value
                bool isTick = false;
                if (range > 0 && trackWidth > 1) {
                    // Check each tick value to see if it maps to this column
                    for (int tickVal = m_minimum; tickVal <= m_maximum; tickVal += m_tickInterval) {
                        int tickPos = static_cast<int>(static_cast<double>(tickVal - m_minimum) / range * (trackWidth - 1) + 0.5);
                        if (tickPos == col) {
                            isTick = true;
                            break;
                        }
                    }
                }

                if (isTick) {
                    Cell &cell = buffer.backCell(px, py);
                    cell.setChar(U'|');
                    cell.foreground = fg;
                    cell.background = bg;
                }
            }
        }

        // Render track row
        int py = geo.y() + trackRow;
        if (py < buffer.height()) {
            for (int col = 0; col < trackWidth; ++col) {
                int px = geo.x() + col;
                if (px >= buffer.width())
                    continue;

                Cell &cell = buffer.backCell(px, py);
                if (col == indicatorPos) {
                    cell.setChar(0x25CF); // ●
                    cell.bold = true;
                } else {
                    cell.setChar(0x2500); // ─
                }
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    } else {
        // Vertical orientation
        bool hasTicks = (m_tickInterval > 0) && (geo.width() >= 3);
        int trackCol = 0;

        int trackHeight = geo.height();
        // Position inverted: max at top, min at bottom
        int indicatorPos = 0;
        if (range > 0 && trackHeight > 1)
            indicatorPos = static_cast<int>(static_cast<double>(m_maximum - m_value) / range * (trackHeight - 1) + 0.5);
        indicatorPos = qBound(0, indicatorPos, trackHeight - 1);

        // Render track column
        for (int row = 0; row < trackHeight; ++row) {
            int px = geo.x() + trackCol;
            int py = geo.y() + row;
            if (px >= buffer.width() || py >= buffer.height())
                continue;

            Cell &cell = buffer.backCell(px, py);
            if (row == indicatorPos) {
                cell.setChar(0x25CF); // ●
                cell.bold = true;
            } else {
                cell.setChar(0x2502); // │
            }
            cell.foreground = fg;
            cell.background = bg;
        }

        // Render tick marks column
        if (hasTicks) {
            int tickCol = trackCol + 1;
            for (int row = 0; row < trackHeight; ++row) {
                int px = geo.x() + tickCol;
                int py = geo.y() + row;
                if (px >= buffer.width() || py >= buffer.height())
                    continue;

                bool isTick = false;
                if (range > 0 && trackHeight > 1) {
                    for (int tickVal = m_minimum; tickVal <= m_maximum; tickVal += m_tickInterval) {
                        int tickPos = static_cast<int>(static_cast<double>(m_maximum - tickVal) / range * (trackHeight - 1) + 0.5);
                        if (tickPos == row) {
                            isTick = true;
                            break;
                        }
                    }
                }

                if (isTick) {
                    Cell &cell = buffer.backCell(px, py);
                    cell.setChar(0x2500); // ─
                    cell.foreground = fg;
                    cell.background = bg;
                }
            }
        }
    }
}

void QTuiSlider::keyPressEvent(TuiKeyEvent *event)
{
    // Up steps up; Right also steps up on a horizontal slider
    const int key = event->key();
    const bool horizontal = m_orientation == Qt::Horizontal;
    if (key == Qt::Key_Up || (horizontal && key == Qt::Key_Right))
        setValue(m_value + m_singleStep);
    else if (key == Qt::Key_Down || (horizontal && key == Qt::Key_Left))
        setValue(m_value - m_singleStep);
}

void QTuiSlider::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    // A vertical slider has the maximum at the top
    const int value = valueAt(event->x(), event->y());
    setValue(m_orientation == Qt::Horizontal ? value : m_maximum + m_minimum - value);
}

} // namespace QtTUI
