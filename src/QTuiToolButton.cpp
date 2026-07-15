#include <QtTUI/QTuiToolButton.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiToolButton::QTuiToolButton(const QString &text, QTuiWidget *parent)
    : QTuiPushButton(text, parent)
{
}

void QTuiToolButton::setMenu(QTuiMenu *menu)
{
    m_menu = menu;
}

QTuiMenu *QTuiToolButton::menu() const
{
    return m_menu;
}

QSize QTuiToolButton::minimumSize() const
{
    // Compact: no border, just padding (1 char each side) + content
    int contentWidth = text().length();
    if (!icon().isEmpty())
        contentWidth += icon().length() + 1; // icon + space
    if (m_menu)
        contentWidth += 2; // space + dropdown indicator "▼"

    // 1 char padding each side
    return QSize(contentWidth + 2, 1);
}

void QTuiToolButton::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();

    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    // Determine colors based on state (a disabled ancestor also disables us)
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

    // If checked, use inverse colors
    if (isCheckable() && isChecked() && enabled) {
        std::swap(fg, bg);
    }

    // Pressed: invert again and move the label 1 cell right
    const bool pressed = isDown() && enabled;

    // Under the mouse (and not focused or pressed): the accent color shows
    // that a click goes here
    if (underMouse() && enabled && !hasFocus() && !pressed && !(isCheckable() && isChecked()))
        fg = theme.focusBackground;
    if (pressed)
        std::swap(fg, bg);

    // Clear the entire area with background (no border)
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

    // Build content string: icon + text + optional dropdown indicator
    QString content;
    if (!icon().isEmpty())
        content = icon() + QLatin1Char(' ') + text();
    else
        content = text();

    if (m_menu)
        content += QStringLiteral(" \u25BC"); // " ▼"

    // Render content centered vertically, with 1 char left padding
    int contentRow = geoY + geoH / 2;
    int availWidth = geoW - 2; // 1 char padding each side
    int textLen = qMin(static_cast<int>(content.length()), availWidth);
    int startCol = geoX + 1; // 1 char left padding
    if (pressed && textLen < availWidth)
        ++startCol;

    for (int i = 0; i < textLen; ++i) {
        int px = startCol + i;
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

void QTuiToolButton::keyPressEvent(TuiKeyEvent *event)
{
    // If menu is set and Down arrow pressed, open the menu
    if (m_menu && event->key() == Qt::Key_Down) {
        // QTuiMenu doesn't exist yet - menu opening will be wired
        // when QTuiMenu is implemented
        return;
    }

    // Delegate Enter/Space handling to parent
    QTuiPushButton::keyPressEvent(event);
}

} // namespace QtTUI
