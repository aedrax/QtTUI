#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiPushButton::QTuiPushButton(const QString &text, QTuiWidget *parent)
    : QTuiAbstractButton(parent)
{
    setText(text);
}

QString QTuiPushButton::icon() const
{
    return m_icon;
}

void QTuiPushButton::setIcon(const QString &icon)
{
    if (m_icon == icon)
        return;
    m_icon = icon;
    markDirty();
}

QSize QTuiPushButton::minimumSize() const
{
    // Content: icon + space + text, surrounded by border (1 char each side)
    int contentWidth = text().length();
    if (!m_icon.isEmpty())
        contentWidth += m_icon.length() + 1; // icon + space

    // Add 2 for left/right border, 1 padding each side
    return QSize(contentWidth + 4, 3); // 3 rows: top border, content, bottom border
}

void QTuiPushButton::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);

    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    // Determine colors based on state; a disabled ancestor also renders
    // this button with the disabled palette.
    const bool enabled = isEffectivelyEnabled();
    Color fg = theme.foreground;
    Color bg = theme.background;

    if (!enabled) {
        fg = theme.disabledForeground;
        bg = theme.disabledBackground;
    } else if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // If checked, use inverse colors as visual indicator
    if (isCheckable() && isChecked() && enabled) {
        std::swap(fg, bg);
    }

    // Pressed: invert again and move the label 1 cell right, so the button
    // looks pushed in
    const bool pressed = isDown() && enabled;

    // Under the mouse (and not focused or pressed): the accent color shows
    // that a click goes here
    if (underMouse() && enabled && !hasFocus() && !pressed && !(isCheckable() && isChecked()))
        fg = theme.focusBackground;
    if (pressed)
        std::swap(fg, bg);

    // Clear the entire button area with background
    for (int row = 0; row < geoH; ++row) {
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            int py = geoY + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }

    // Render top border
    if (geoH >= 1) {
        int py = geoY;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.setChar(border.topLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.topRight);
                else
                    cell.setChar(border.horizontal);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render bottom border
    if (geoH >= 3) {
        int py = geoY + geoH - 1;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.setChar(border.bottomLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.bottomRight);
                else
                    cell.setChar(border.horizontal);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render left and right borders for middle rows
    for (int row = 1; row < geoH - 1; ++row) {
        int py = geoY + row;
        if (py < buffer.height()) {
            // Left border
            int pxLeft = geoX;
            if (pxLeft < buffer.width()) {
                Cell &cell = buffer.backCell(pxLeft, py);
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
            // Right border
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                Cell &cell = buffer.backCell(pxRight, py);
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Build content string: icon + text
    QString content;
    if (!m_icon.isEmpty()) {
        content = m_icon + QLatin1Char(' ') + text();
    } else {
        content = text();
    }

    // Render content centered in the middle row(s)
    if (geoH >= 3 && geoW >= 3) {
        int contentRow = geoY + geoH / 2; // Middle row
        int innerWidth = geoW - 2;        // Exclude left/right borders
        int textLen = qMin(static_cast<int>(content.length()), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center within inner area
        if (pressed && startCol + textLen < geoW - 1)
            ++startCol;

        for (int i = 0; i < textLen; ++i) {
            int px = geoX + startCol + i;
            int py = contentRow;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(content.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold || (enabled && hasFocus());
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }
}

} // namespace QtTUI
