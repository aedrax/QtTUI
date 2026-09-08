#include <QtTUI/QTuiSlider.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiSlider::QTuiSlider(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
    setFocusPolicy(Qt::StrongFocus);
}

int QTuiSlider::value() const
{
    return m_value;
}

void QTuiSlider::setValue(int value)
{
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

int QTuiSlider::minimum() const
{
    return m_minimum;
}

void QTuiSlider::setMinimum(int min)
{
    if (m_minimum == min)
        return;
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
    markDirty();
}

int QTuiSlider::maximum() const
{
    return m_maximum;
}

void QTuiSlider::setMaximum(int max)
{
    if (m_maximum == max)
        return;
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
    markDirty();
}

void QTuiSlider::setRange(int min, int max)
{
    if (m_minimum == min && m_maximum == qMax(min, max))
        return;
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value);
    markDirty();
}

int QTuiSlider::singleStep() const
{
    return m_singleStep;
}

void QTuiSlider::setSingleStep(int step)
{
    if (step > 0)
        m_singleStep = step;
}

Qt::Orientation QTuiSlider::orientation() const
{
    return m_orientation;
}

void QTuiSlider::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation)
        return;
    m_orientation = orientation;
    markDirty();
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
    const int key = event->key();

    if (m_orientation == Qt::Horizontal) {
        // Horizontal: Right/Up = increment, Left/Down = decrement
        if (key == Qt::Key_Right || key == Qt::Key_Up) {
            int newValue = qBound(m_minimum, m_value + m_singleStep, m_maximum);
            setValue(newValue);
        } else if (key == Qt::Key_Left || key == Qt::Key_Down) {
            int newValue = qBound(m_minimum, m_value - m_singleStep, m_maximum);
            setValue(newValue);
        }
    } else {
        // Vertical: Up = increment, Down = decrement, Left/Right ignored
        if (key == Qt::Key_Up) {
            int newValue = qBound(m_minimum, m_value + m_singleStep, m_maximum);
            setValue(newValue);
        } else if (key == Qt::Key_Down) {
            int newValue = qBound(m_minimum, m_value - m_singleStep, m_maximum);
            setValue(newValue);
        }
    }
}

void QTuiSlider::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    int newValue;

    if (m_orientation == Qt::Horizontal) {
        newValue = m_minimum + (event->x() - geo.x()) * (m_maximum - m_minimum) / qMax(1, geo.width() - 1);
    } else {
        newValue = m_minimum + (event->y() - geo.y()) * (m_maximum - m_minimum) / qMax(1, geo.height() - 1);
    }
    setValue(qBound(m_minimum, newValue, m_maximum));
}

} // namespace QtTUI
