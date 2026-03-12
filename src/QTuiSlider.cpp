#include <QtTUI/QTuiSlider.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiSlider::QTuiSlider(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
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
    }
}

int QTuiSlider::minimum() const
{
    return m_minimum;
}

void QTuiSlider::setMinimum(int min)
{
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
}

int QTuiSlider::maximum() const
{
    return m_maximum;
}

void QTuiSlider::setMaximum(int max)
{
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
}

void QTuiSlider::setRange(int min, int max)
{
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value);
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
    m_orientation = orientation;
}

int QTuiSlider::tickInterval() const
{
    return m_tickInterval;
}

void QTuiSlider::setTickInterval(int interval)
{
    m_tickInterval = (interval >= 0) ? interval : 0;
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

    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    Color fg = theme.foreground;
    Color bg = theme.background;

    if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // Clear the entire widget area
    for (int row = 0; row < geoH; ++row) {
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            int py = geoY + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.character = QLatin1Char(' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
                cell.italic = false;
                cell.underline = false;
            }
        }
    }

    int range = m_maximum - m_minimum;

    if (m_orientation == Qt::Horizontal) {
        bool hasTicks = (m_tickInterval > 0) && (geoH >= 2);
        int trackRow = hasTicks ? 1 : 0;

        // Compute indicator position on the track
        int trackWidth = geoW;
        int indicatorPos = 0;
        if (range > 0 && trackWidth > 1)
            indicatorPos = static_cast<int>(static_cast<double>(m_value - m_minimum) / range * (trackWidth - 1) + 0.5);
        indicatorPos = qBound(0, indicatorPos, trackWidth - 1);

        // Render tick marks row
        if (hasTicks) {
            int py = geoY;
            for (int col = 0; col < trackWidth; ++col) {
                int px = geoX + col;
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
                    cell.character = QLatin1Char('|');
                    cell.foreground = fg;
                    cell.background = bg;
                }
            }
        }

        // Render track row
        int py = geoY + trackRow;
        if (py < buffer.height()) {
            for (int col = 0; col < trackWidth; ++col) {
                int px = geoX + col;
                if (px >= buffer.width())
                    continue;

                Cell &cell = buffer.backCell(px, py);
                if (col == indicatorPos) {
                    cell.character = QChar(0x25CF); // ●
                    cell.bold = true;
                } else {
                    cell.character = QChar(0x2500); // ─
                }
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    } else {
        // Vertical orientation
        bool hasTicks = (m_tickInterval > 0) && (geoW >= 3);
        int trackCol = 0;

        int trackHeight = geoH;
        // Position inverted: max at top, min at bottom
        int indicatorPos = 0;
        if (range > 0 && trackHeight > 1)
            indicatorPos = static_cast<int>(static_cast<double>(m_maximum - m_value) / range * (trackHeight - 1) + 0.5);
        indicatorPos = qBound(0, indicatorPos, trackHeight - 1);

        // Render track column
        for (int row = 0; row < trackHeight; ++row) {
            int px = geoX + trackCol;
            int py = geoY + row;
            if (px >= buffer.width() || py >= buffer.height())
                continue;

            Cell &cell = buffer.backCell(px, py);
            if (row == indicatorPos) {
                cell.character = QChar(0x25CF); // ●
                cell.bold = true;
            } else {
                cell.character = QChar(0x2502); // │
            }
            cell.foreground = fg;
            cell.background = bg;
        }

        // Render tick marks column
        if (hasTicks) {
            int tickCol = trackCol + 1;
            for (int row = 0; row < trackHeight; ++row) {
                int px = geoX + tickCol;
                int py = geoY + row;
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
                    cell.character = QChar(0x2500); // ─
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

} // namespace QtTUI
