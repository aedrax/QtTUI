#include <QtTUI/QTuiScrollBar.h>
#include <QtTUI/RenderUtils.h>
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

    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    const bool vertical = m_orientation == Qt::Vertical;
    const int range = m_maximum - m_minimum;
    const int trackLen = vertical ? geo.height() : geo.width();

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

    const Cell style = cellStyle(fg, bg);
    for (int i = 0; i < trackLen; ++i) {
        const bool thumb = i >= thumbPos && i < thumbPos + thumbSize;
        const char32_t ch = thumb ? 0x2588 : (vertical ? 0x2502 : 0x2500); // █ thumb, │ or ─ track
        buffer.putChar(vertical ? geo.x() : geo.x() + i, vertical ? geo.y() + i : geo.y(), ch, style);
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
