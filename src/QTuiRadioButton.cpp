#include <QtTUI/QTuiRadioButton.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiRadioButton::QTuiRadioButton(const QString &text, QTuiWidget *parent)
    : QTuiAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
}

void QTuiRadioButton::nextCheckState()
{
    setChecked(true);
}

QSize QTuiRadioButton::minimumSize() const
{
    // "(*) " = 4 chars + text length
    return QSize(4 + text().length(), 1);
}

void QTuiRadioButton::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const int geoX = geo.x();
    const int geoY = geo.y();
    const int geoW = geo.width();
    const int geoH = geo.height();

    // Determine colors based on focus state
    Color fg = theme.foreground;
    Color bg = theme.background;
    if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // Clear the widget area with background
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

    // Build the display string: indicator + space + text
    QString indicator = isChecked() ? QStringLiteral("(*)") : QStringLiteral("( )");
    QString content = indicator + QLatin1Char(' ') + text();
    int textLen = qMin(static_cast<int>(content.length()), geoW);

    // Render on the first row
    int py = geoY;
    if (py < buffer.height()) {
        for (int i = 0; i < textLen; ++i) {
            int px = geoX + i;
            if (px < buffer.width()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(content.at(i).unicode());
                // Pressed look: the 3-cell indicator inverts
                const bool pressedCell = i < 3 && isDown() && isEffectivelyEnabled();
                cell.foreground = pressedCell ? bg : fg;
                cell.background = pressedCell ? fg : bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }
}

} // namespace QtTUI
