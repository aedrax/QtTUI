#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiScrollBar::QTuiScrollBar(Qt::Orientation orientation, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_orientation(orientation)
{
    setFocusPolicy(Qt::StrongFocus);
}

int QTuiScrollBar::value() const
{
    return m_value;
}

void QTuiScrollBar::setValue(int value)
{
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

int QTuiScrollBar::minimum() const
{
    return m_minimum;
}

void QTuiScrollBar::setMinimum(int min)
{
    if (m_minimum == min)
        return;
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
    markDirty();
}

int QTuiScrollBar::maximum() const
{
    return m_maximum;
}

void QTuiScrollBar::setMaximum(int max)
{
    if (m_maximum == max)
        return;
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
    markDirty();
}

void QTuiScrollBar::setRange(int min, int max)
{
    max = qMax(min, max);
    if (m_minimum == min && m_maximum == max)
        return;
    m_minimum = min;
    m_maximum = max;
    setValue(m_value);
    markDirty();
}

int QTuiScrollBar::singleStep() const
{
    return m_singleStep;
}

void QTuiScrollBar::setSingleStep(int step)
{
    if (step > 0)
        m_singleStep = step;
}

int QTuiScrollBar::pageStep() const
{
    return m_pageStep;
}

void QTuiScrollBar::setPageStep(int step)
{
    if (step > 0 && m_pageStep != step) {
        m_pageStep = step;
        markDirty();
    }
}

Qt::Orientation QTuiScrollBar::orientation() const
{
    return m_orientation;
}

QSize QTuiScrollBar::minimumSize() const
{
    if (m_orientation == Qt::Horizontal)
        return QSize(3, 1);
    else
        return QSize(1, 3);
}

void QTuiScrollBar::paintEvent(ScreenBuffer &buffer)
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

    int range = m_maximum - m_minimum;

    if (m_orientation == Qt::Vertical) {
        int trackLen = geoH;

        // Compute thumb size proportional to pageStep vs total range
        int thumbSize = 1;
        if (range > 0 && trackLen > 1) {
            thumbSize = qMax(1, static_cast<int>(
                static_cast<double>(m_pageStep) / (range + m_pageStep) * trackLen + 0.5));
            thumbSize = qMin(thumbSize, trackLen);
        }

        // Compute thumb position
        int thumbPos = 0;
        int scrollableTrack = trackLen - thumbSize;
        if (range > 0 && scrollableTrack > 0) {
            thumbPos = static_cast<int>(
                static_cast<double>(m_value - m_minimum) / range * scrollableTrack + 0.5);
            thumbPos = qBound(0, thumbPos, scrollableTrack);
        }

        for (int row = 0; row < trackLen; ++row) {
            int px = geoX;
            int py = geoY + row;
            if (px >= buffer.width() || py >= buffer.height())
                continue;

            Cell &cell = buffer.backCell(px, py);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;

            if (row >= thumbPos && row < thumbPos + thumbSize) {
                cell.setChar(0x2588); // █ thumb
            } else {
                cell.setChar(0x2502); // │ track
            }
        }
    } else {
        // Horizontal
        int trackLen = geoW;

        int thumbSize = 1;
        if (range > 0 && trackLen > 1) {
            thumbSize = qMax(1, static_cast<int>(
                static_cast<double>(m_pageStep) / (range + m_pageStep) * trackLen + 0.5));
            thumbSize = qMin(thumbSize, trackLen);
        }

        int thumbPos = 0;
        int scrollableTrack = trackLen - thumbSize;
        if (range > 0 && scrollableTrack > 0) {
            thumbPos = static_cast<int>(
                static_cast<double>(m_value - m_minimum) / range * scrollableTrack + 0.5);
            thumbPos = qBound(0, thumbPos, scrollableTrack);
        }

        int py = geoY;
        if (py >= buffer.height())
            return;

        for (int col = 0; col < trackLen; ++col) {
            int px = geoX + col;
            if (px >= buffer.width())
                continue;

            Cell &cell = buffer.backCell(px, py);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;

            if (col >= thumbPos && col < thumbPos + thumbSize) {
                cell.setChar(0x2588); // █ thumb
            } else {
                cell.setChar(0x2500); // ─ track
            }
        }
    }
}

void QTuiScrollBar::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();

    if (m_orientation == Qt::Vertical) {
        if (key == Qt::Key_Up) {
            setValue(m_value - m_singleStep);
        } else if (key == Qt::Key_Down) {
            setValue(m_value + m_singleStep);
        } else if (key == Qt::Key_PageUp) {
            setValue(m_value - m_pageStep);
        } else if (key == Qt::Key_PageDown) {
            setValue(m_value + m_pageStep);
        }
    } else {
        if (key == Qt::Key_Left) {
            setValue(m_value - m_singleStep);
        } else if (key == Qt::Key_Right) {
            setValue(m_value + m_singleStep);
        } else if (key == Qt::Key_PageUp) {
            setValue(m_value - m_pageStep);
        } else if (key == Qt::Key_PageDown) {
            setValue(m_value + m_pageStep);
        }
    }
}

void QTuiScrollBar::mousePressEvent(TuiMouseEvent *event)
{
    // A press jumps to the position; a drag with the left button follows
    // the mouse
    const bool drag = event->mouseType() == TuiMouseEvent::Move && event->button() == Qt::LeftButton;
    if (event->mouseType() != TuiMouseEvent::Press && !drag)
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
