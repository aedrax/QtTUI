#include <QtTUI/QTuiDial.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiDial::QTuiDial(QTuiWidget *parent)
    : QTuiAbstractSlider(Qt::Horizontal, parent)
{
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

    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    // Clear the entire widget area
    fillRect(buffer, geo, cellStyle(fg, bg));

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
    int dialW = qMin(geo.width(), qMax(9, geo.width()));
    int dialH = qMin(geo.height(), qMax(5, geo.height()));

    // Offsets to center the dial in the widget
    int offX = (geo.width() - dialW) / 2;
    int offY = (geo.height() - dialH) / 2;

    // Inner width between the side borders (row 2)
    int innerW = dialW - 2; // excluding │ on each side

    // Helper to write a character at a position
    auto writeChar = [&](int col, int row, char32_t ch, bool bold = false) {
        int px = geo.x() + offX + col;
        int py = geo.y() + offY + row;
        if (px >= 0 && px < buffer.width() && py >= 0 && py < buffer.height()) {
            Cell &cell = buffer.backCell(px, py);
            cell.setChar(ch);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = bold;
        }
    };

    // Row 0: top arc "  ╭───╮  "
    if (dialH >= 1) {
        int arcStart = 2;
        int arcEnd = dialW - 3;
        if (arcStart < dialW) writeChar(arcStart, 0, char32_t(0x256D)); // ╭
        for (int c = arcStart + 1; c <= arcEnd - 1 && c < dialW; ++c)
            writeChar(c, 0, char32_t(0x2500)); // ─
        if (arcEnd < dialW) writeChar(arcEnd, 0, char32_t(0x256E)); // ╮
    }

    // Row 1: upper sides " ╱     ╲ "
    if (dialH >= 2) {
        writeChar(1, 1, char32_t(0x2571)); // ╱
        writeChar(dialW - 2, 1, char32_t(0x2572)); // ╲
    }

    // Row 2 (middle): value display "│  val  │"
    if (dialH >= 3) {
        int midRow = dialH / 2;
        writeChar(0, midRow, char32_t(0x2502)); // │
        writeChar(dialW - 1, midRow, char32_t(0x2502)); // │

        // Render value centered in the inner area
        QString valueStr = QString::number(m_value);
        int textLen = qMin(static_cast<int>(valueStr.length()), innerW);
        int textStart = 1 + (innerW - textLen) / 2;
        for (int i = 0; i < textLen; ++i) {
            writeChar(textStart + i, midRow, valueStr.at(i).unicode(), true);
        }
    }

    // Row 3: lower sides " ╲     ╱ "
    if (dialH >= 4) {
        int lowerRow = dialH - 2;
        writeChar(1, lowerRow, char32_t(0x2572)); // ╲
        writeChar(dialW - 2, lowerRow, char32_t(0x2571)); // ╱
    }

    // Row 4: bottom arc "  ╰───╯  "
    if (dialH >= 5) {
        int botRow = dialH - 1;
        int arcStart = 2;
        int arcEnd = dialW - 3;
        if (arcStart < dialW) writeChar(arcStart, botRow, char32_t(0x2570)); // ╰
        for (int c = arcStart + 1; c <= arcEnd - 1 && c < dialW; ++c)
            writeChar(c, botRow, char32_t(0x2500)); // ─
        if (arcEnd < dialW) writeChar(arcEnd, botRow, char32_t(0x256F)); // ╯
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
            writeChar(indicatorCol, 0, char32_t(0x25CF), true); // ● indicator on top arc
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
