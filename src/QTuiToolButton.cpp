#include <QtTUI/QTuiToolButton.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>
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

    const bool enabled = isEffectivelyEnabled();
    // Pressed: move the label 1 cell right
    const bool pressed = isDown() && enabled;
    const Cell style = buttonStyle(theme);

    // Clear the entire area with background (no border)
    fillRect(buffer, geo, style);

    // Build content string: icon + text + optional dropdown indicator
    QString content;
    if (!icon().isEmpty())
        content = icon() + QLatin1Char(' ') + text();
    else
        content = text();

    if (m_menu)
        content += QStringLiteral(" \u25BC"); // " ▼"

    // Render content centered vertically, with 1 char left padding
    int contentRow = geo.y() + geo.height() / 2;
    int availWidth = geo.width() - 2; // 1 char padding each side
    int textLen = qMin(QtTuiUnicode::stringWidth(content), availWidth);
    int startCol = geo.x() + 1; // 1 char left padding
    if (pressed && textLen < availWidth)
        ++startCol;

    Cell textStyle = style;
    textStyle.bold = theme.bold || (enabled && hasFocus());
    renderText(buffer, startCol, contentRow, content, textLen, textStyle);
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
