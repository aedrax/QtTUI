#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/RenderUtils.h>
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

    fillRect(buffer, geo, cellStyle(theme, fg, bg));
    drawBorder(buffer, geo, theme.borderStyle, cellStyle(theme, fg, bg));

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
        int textLen = qMin(QtTuiUnicode::stringWidth(content), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center within inner area
        if (pressed && startCol + textLen < geoW - 1)
            ++startCol;

        Cell textStyle = cellStyle(theme, fg, bg);
        textStyle.bold = theme.bold || (enabled && hasFocus());
        renderText(buffer, geoX + startCol, contentRow, content, textLen, textStyle);
    }
}

} // namespace QtTUI
