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

    const bool enabled = isEffectivelyEnabled();
    // Pressed: move the label 1 cell right, so the button looks pushed in
    const bool pressed = isDown() && enabled;
    const Cell style = buttonStyle(theme);

    fillRect(buffer, geo, style);
    drawBorder(buffer, geo, theme.borderStyle, style);

    // Build content string: icon + text
    QString content;
    if (!m_icon.isEmpty()) {
        content = m_icon + QLatin1Char(' ') + text();
    } else {
        content = text();
    }

    // Render content centered in the middle row(s)
    if (geo.height() >= 3 && geo.width() >= 3) {
        int contentRow = geo.y() + geo.height() / 2; // Middle row
        int innerWidth = geo.width() - 2;        // Exclude left/right borders
        int textLen = qMin(QtTuiUnicode::stringWidth(content), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center within inner area
        if (pressed && startCol + textLen < geo.width() - 1)
            ++startCol;

        Cell textStyle = style;
        textStyle.bold = theme.bold || (enabled && hasFocus());
        renderText(buffer, geo.x() + startCol, contentRow, content, textLen, textStyle);
    }
}

} // namespace QtTUI
