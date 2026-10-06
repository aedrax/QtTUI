#include <QtTUI/QTuiToolButton.h>
#include <QtTUI/QTuiMenu.h>
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

    const QString content = label();

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

QString QTuiToolButton::label() const
{
    QString content = icon().isEmpty() ? text() : icon() + QLatin1Char(' ') + text();
    if (m_menu)
        content += QStringLiteral(" \u25BC"); // " ▼"
    return content;
}

void QTuiToolButton::mousePressEvent(TuiMouseEvent *event)
{
    // The label starts after 1 column of padding, so "▼" is at the column
    // after the label width
    if (m_menu && event->mouseType() == TuiMouseEvent::Press && event->button() == Qt::LeftButton
        && event->x() == geometry().x() + QtTuiUnicode::stringWidth(label())) {
        m_menu->popup(geometry().bottomLeft() + QPoint(0, 1));
        return;
    }
    QTuiPushButton::mousePressEvent(event);
}

void QTuiToolButton::keyPressEvent(TuiKeyEvent *event)
{
    // If menu is set and Down arrow pressed, open the menu below the button
    if (m_menu && event->key() == Qt::Key_Down) {
        m_menu->popup(geometry().bottomLeft() + QPoint(0, 1));
        return;
    }

    // Delegate Enter/Space handling to parent
    QTuiPushButton::keyPressEvent(event);
}

} // namespace QtTUI
