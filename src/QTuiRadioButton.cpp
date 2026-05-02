#include <QtTUI/QTuiRadioButton.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiRadioButton::QTuiRadioButton(const QString &text, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_text(text)
{
}

QString QTuiRadioButton::text() const
{
    return m_text;
}

void QTuiRadioButton::setText(const QString &text)
{
    m_text = text;
}

bool QTuiRadioButton::isChecked() const
{
    return m_checked;
}

void QTuiRadioButton::setChecked(bool checked)
{
    if (m_checked != checked) {
        m_checked = checked;
        emit toggled(m_checked);
        markDirty();
    }
}

QSize QTuiRadioButton::minimumSize() const
{
    // "(*) " = 4 chars + text length
    return QSize(4 + m_text.length(), 1);
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
                cell.character = QLatin1Char(' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }

    // Build the display string: indicator + space + text
    QString indicator = m_checked ? QStringLiteral("(*)") : QStringLiteral("( )");
    QString content = indicator + QLatin1Char(' ') + m_text;
    int textLen = qMin(static_cast<int>(content.length()), geoW);

    // Render on the first row
    int py = geoY;
    if (py < buffer.height()) {
        for (int i = 0; i < textLen; ++i) {
            int px = geoX + i;
            if (px < buffer.width()) {
                Cell &cell = buffer.backCell(px, py);
                cell.character = content.at(i);
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }
}

void QTuiRadioButton::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        // Radio buttons only set themselves to checked
        // Unchecking others is handled by QTuiButtonGroup
        setChecked(true);
    }
}

void QTuiRadioButton::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    // Radio buttons only set themselves to checked
    // Unchecking others is handled by QTuiButtonGroup
    setChecked(true);
}

} // namespace QtTUI
