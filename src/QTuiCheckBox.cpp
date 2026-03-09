#include <QtTUI/QTuiCheckBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiCheckBox::QTuiCheckBox(const QString &text, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_text(text)
{
}

QString QTuiCheckBox::text() const
{
    return m_text;
}

void QTuiCheckBox::setText(const QString &text)
{
    m_text = text;
}

int QTuiCheckBox::checkState() const
{
    return m_checkState;
}

void QTuiCheckBox::setCheckState(int state)
{
    // Clamp to valid range
    state = qBound(0, state, 2);
    if (m_checkState != state) {
        m_checkState = state;
        emit stateChanged(m_checkState);
    }
}

bool QTuiCheckBox::isChecked() const
{
    return m_checkState == Checked;
}

void QTuiCheckBox::setChecked(bool checked)
{
    setCheckState(checked ? Checked : Unchecked);
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
    return QSize(4 + m_text.length(), 1);
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
    QString indicator;
    switch (m_checkState) {
    case Checked:          indicator = QStringLiteral("[x]"); break;
    case PartiallyChecked: indicator = QStringLiteral("[-]"); break;
    default:               indicator = QStringLiteral("[ ]"); break;
    }

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

void QTuiCheckBox::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        if (m_tristate) {
            // Cycle: Unchecked → PartiallyChecked → Checked → Unchecked
            setCheckState((m_checkState + 1) % 3);
        } else {
            // Toggle: Unchecked ↔ Checked
            setCheckState(m_checkState == Checked ? Unchecked : Checked);
        }
    }
}

} // namespace QtTUI
