#include <QtTUI/QTuiCheckBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiCheckBox::QTuiCheckBox(const QString &text, QTuiWidget *parent)
    : QTuiAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
}

int QTuiCheckBox::checkState() const
{
    return m_checkState;
}

void QTuiCheckBox::setCheckState(int state)
{
    // Clamp to valid range
    state = qBound(0, state, 2);
    if (m_checkState == state)
        return;
    m_checkState = state;
    emit stateChanged(m_checkState);
    markDirty();
    // Keep the base checked state (Checked only) in sync
    QTuiAbstractButton::setChecked(m_checkState == Checked);
}

void QTuiCheckBox::nextCheckState()
{
    if (m_tristate)
        setCheckState((m_checkState + 1) % 3); // Unchecked, Partially, Checked
    else
        setCheckState(m_checkState == Checked ? Unchecked : Checked);
}

void QTuiCheckBox::checkStateSet()
{
    // setChecked() through the base class: follow it, but keep a partial
    // state that already matches "not checked"
    if ((m_checkState == Checked) != isChecked())
        setCheckState(isChecked() ? Checked : Unchecked);
}

bool QTuiCheckBox::isTristate() const
{
    return m_tristate;
}

void QTuiCheckBox::setTristate(bool tristate)
{
    m_tristate = tristate;
}

QSize QTuiCheckBox::minimumSize() const
{
    // "[x] " = 4 chars + text length
    return QSize(4 + text().length(), 1);
}

void QTuiCheckBox::paintEvent(ScreenBuffer &buffer)
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
    QString indicator;
    switch (m_checkState) {
    case Checked:          indicator = QStringLiteral("[x]"); break;
    case PartiallyChecked: indicator = QStringLiteral("[-]"); break;
    default:               indicator = QStringLiteral("[ ]"); break;
    }

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
