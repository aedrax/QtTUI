#include <QtTUI/QTuiDial.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiDial::QTuiDial(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

int QTuiDial::value() const
{
    return m_value;
}

void QTuiDial::setValue(int value)
{
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
    }
}

int QTuiDial::minimum() const
{
    return m_minimum;
}

void QTuiDial::setMinimum(int min)
{
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
}

int QTuiDial::maximum() const
{
    return m_maximum;
}

void QTuiDial::setMaximum(int max)
{
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
}

void QTuiDial::setRange(int min, int max)
{
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value);
}

int QTuiDial::singleStep() const
{
    return m_singleStep;
}

void QTuiDial::setSingleStep(int step)
{
    if (step > 0)
        m_singleStep = step;
}

bool QTuiDial::wrapping() const
{
    return m_wrapping;
}

void QTuiDial::setWrapping(bool wrapping)
{
    m_wrapping = wrapping;
}

void QTuiDial::stepUp()
{
    int newValue = m_value + m_singleStep;
    if (newValue > m_maximum) {
        if (m_wrapping)
            newValue = m_minimum;
        else
            newValue = m_maximum;
    }
    setValue(newValue);
}

void QTuiDial::stepDown()
{
    int newValue = m_value - m_singleStep;
    if (newValue < m_minimum) {
        if (m_wrapping)
            newValue = m_maximum;
        else
            newValue = m_minimum;
    }
    setValue(newValue);
}

QSize QTuiDial::minimumSize() const
{
    return QSize(9, 5);
}

void QTuiDial::paintEvent(ScreenBuffer &buffer)
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

    // Render a circular dial using box-drawing characters
    // Layout for 5 rows (minimum):
    //   row 0:  "  ╭───╮  "
    //   row 1:  " ╱     ╲ "
    //   row 2:  "│  val  │"
    //   row 3:  " ╲     ╱ "
    //   row 4:  "  ╰───╯  "
    //
    // We center this pattern within the widget geometry.

    // Use at least 5 rows and 9 cols for the dial shape
    int dialW = qMin(geoW, qMax(9, geoW));
    int dialH = qMin(geoH, qMax(5, geoH));

    // Offsets to center the dial in the widget
    int offX = (geoW - dialW) / 2;
    int offY = (geoH - dialH) / 2;

    // Inner width between the side borders (row 2)
    int innerW = dialW - 2; // excluding │ on each side

    // Helper to write a character at a position
    auto writeChar = [&](int col, int row, QChar ch, bool bold = false) {
        int px = geoX + offX + col;
        int py = geoY + offY + row;
        if (px >= 0 && px < buffer.width() && py >= 0 && py < buffer.height()) {
            Cell &cell = buffer.backCell(px, py);
            cell.character = ch;
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = bold;
        }
    };

    // Row 0: top arc "  ╭───╮  "
    if (dialH >= 1) {
        int arcStart = 2;
        int arcEnd = dialW - 3;
        if (arcStart < dialW) writeChar(arcStart, 0, QChar(0x256D)); // ╭
        for (int c = arcStart + 1; c <= arcEnd - 1 && c < dialW; ++c)
            writeChar(c, 0, QChar(0x2500)); // ─
        if (arcEnd < dialW) writeChar(arcEnd, 0, QChar(0x256E)); // ╮
    }

    // Row 1: upper sides " ╱     ╲ "
    if (dialH >= 2) {
        writeChar(1, 1, QChar(0x2571)); // ╱
        writeChar(dialW - 2, 1, QChar(0x2572)); // ╲
    }

    // Row 2 (middle): value display "│  val  │"
    if (dialH >= 3) {
        int midRow = dialH / 2;
        writeChar(0, midRow, QChar(0x2502)); // │
        writeChar(dialW - 1, midRow, QChar(0x2502)); // │

        // Render value centered in the inner area
        QString valueStr = QString::number(m_value);
        int textLen = qMin(static_cast<int>(valueStr.length()), innerW);
        int textStart = 1 + (innerW - textLen) / 2;
        for (int i = 0; i < textLen; ++i) {
            writeChar(textStart + i, midRow, valueStr.at(i), true);
        }
    }

    // Row 3: lower sides " ╲     ╱ "
    if (dialH >= 4) {
        int lowerRow = dialH - 2;
        writeChar(1, lowerRow, QChar(0x2572)); // ╲
        writeChar(dialW - 2, lowerRow, QChar(0x2571)); // ╱
    }

    // Row 4: bottom arc "  ╰───╯  "
    if (dialH >= 5) {
        int botRow = dialH - 1;
        int arcStart = 2;
        int arcEnd = dialW - 3;
        if (arcStart < dialW) writeChar(arcStart, botRow, QChar(0x2570)); // ╰
        for (int c = arcStart + 1; c <= arcEnd - 1 && c < dialW; ++c)
            writeChar(c, botRow, QChar(0x2500)); // ─
        if (arcEnd < dialW) writeChar(arcEnd, botRow, QChar(0x256F)); // ╯
    }

    // Position indicator on the arc based on value
    // Map value to a position around the arc (12 positions on the border)
    int range = m_maximum - m_minimum;
    if (range > 0) {
        // Normalize value to 0.0 - 1.0
        double norm = static_cast<double>(m_value - m_minimum) / range;

        // Map to arc positions (0 = bottom-left, going clockwise)
        // We have positions around the border:
        // bottom: ╰───╯  (positions along bottom arc)
        // right:  ╲, │, ╱ (right side)
        // top:    ╭───╮  (positions along top arc)
        // left:   ╱, │, ╲ (left side)
        // Use a simple indicator: place a marker on the top arc proportional to value
        int arcStart = 2;
        int arcEnd = dialW - 3;
        int arcLen = arcEnd - arcStart - 1; // number of ─ positions
        if (arcLen > 0) {
            int indicatorCol = arcStart + 1 + static_cast<int>(norm * (arcLen - 1) + 0.5);
            indicatorCol = qBound(arcStart + 1, indicatorCol, arcEnd - 1);
            writeChar(indicatorCol, 0, QChar(0x25CF), true); // ● indicator on top arc
        }
    }
}

void QTuiDial::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Up || key == Qt::Key_Right) {
        stepUp();
    } else if (key == Qt::Key_Down || key == Qt::Key_Left) {
        stepDown();
    }
}

} // namespace QtTUI
