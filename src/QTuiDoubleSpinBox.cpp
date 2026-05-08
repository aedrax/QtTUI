#include <QtTUI/QTuiDoubleSpinBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiDoubleSpinBox::QTuiDoubleSpinBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

double QTuiDoubleSpinBox::value() const
{
    return m_value;
}

void QTuiDoubleSpinBox::setValue(double value)
{
    double clamped = qBound(m_minimum, value, m_maximum);
    if (!qFuzzyCompare(m_value, clamped)) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

double QTuiDoubleSpinBox::minimum() const
{
    return m_minimum;
}

void QTuiDoubleSpinBox::setMinimum(double min)
{
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value);
}

double QTuiDoubleSpinBox::maximum() const
{
    return m_maximum;
}

void QTuiDoubleSpinBox::setMaximum(double max)
{
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value);
}

void QTuiDoubleSpinBox::setRange(double min, double max)
{
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value);
}

double QTuiDoubleSpinBox::singleStep() const
{
    return m_singleStep;
}

void QTuiDoubleSpinBox::setSingleStep(double step)
{
    if (step > 0.0)
        m_singleStep = step;
}

int QTuiDoubleSpinBox::decimals() const
{
    return m_decimals;
}

void QTuiDoubleSpinBox::setDecimals(int decimals)
{
    if (decimals >= 0)
        m_decimals = decimals;
}

QString QTuiDoubleSpinBox::prefix() const
{
    return m_prefix;
}

void QTuiDoubleSpinBox::setPrefix(const QString &prefix)
{
    m_prefix = prefix;
}

QString QTuiDoubleSpinBox::suffix() const
{
    return m_suffix;
}

void QTuiDoubleSpinBox::setSuffix(const QString &suffix)
{
    m_suffix = suffix;
}

void QTuiDoubleSpinBox::stepUp()
{
    double newValue = m_value + m_singleStep;
    if (newValue > m_maximum)
        newValue = m_maximum;
    setValue(newValue);
}

void QTuiDoubleSpinBox::stepDown()
{
    double newValue = m_value - m_singleStep;
    if (newValue < m_minimum)
        newValue = m_minimum;
    setValue(newValue);
}

QSize QTuiDoubleSpinBox::minimumSize() const
{
    // Content: "▲ " + prefix + value + suffix + " ▼"
    QString maxStr = QString::number(m_maximum, 'f', m_decimals);
    QString minStr = QString::number(m_minimum, 'f', m_decimals);
    int valueWidth = qMax(maxStr.length(), minStr.length());
    int contentWidth = 2 + m_prefix.length() + valueWidth + m_suffix.length() + 2;

    // Add 2 for left/right border, 1 padding each side
    return QSize(contentWidth + 4, 3);
}

void QTuiDoubleSpinBox::paintEvent(ScreenBuffer &buffer)
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

    Color fg = theme.foreground;
    Color bg = theme.background;

    if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // Clear the entire widget area with background
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

    // Render top border
    if (geoH >= 1) {
        int py = geoY;
        for (int col = 0; col < geoW; ++col) {
            int px = geoX + col;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                if (col == 0)
                    cell.character = border.topLeft;
                else if (col == geoW - 1)
                    cell.character = border.topRight;
                else
                    cell.character = border.horizontal;
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
                    cell.character = border.bottomLeft;
                else if (col == geoW - 1)
                    cell.character = border.bottomRight;
                else
                    cell.character = border.horizontal;
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render left and right borders for middle rows
    for (int row = 1; row < geoH - 1; ++row) {
        int py = geoY + row;
        if (py < buffer.height()) {
            int pxLeft = geoX;
            if (pxLeft < buffer.width()) {
                Cell &cell = buffer.backCell(pxLeft, py);
                cell.character = border.vertical;
                cell.foreground = fg;
                cell.background = bg;
            }
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                Cell &cell = buffer.backCell(pxRight, py);
                cell.character = border.vertical;
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Build content: "▲ prefix+value+suffix ▼"
    QString valueStr = QString::number(m_value, 'f', m_decimals);
    QString content = QStringLiteral("\u25B2 ") + m_prefix + valueStr + m_suffix + QStringLiteral(" \u25BC");

    // Render content centered in the middle row
    if (geoH >= 3 && geoW >= 3) {
        int contentRow = geoY + geoH / 2;
        int innerWidth = geoW - 2;
        int textLen = qMin(static_cast<int>(content.length()), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2;

        for (int i = 0; i < textLen; ++i) {
            int px = geoX + startCol + i;
            int py = contentRow;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.character = content.at(i);
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold || hasFocus();
                cell.italic = theme.italic;
                cell.underline = theme.underline;
            }
        }
    }
}

void QTuiDoubleSpinBox::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Up) {
        stepUp();
    } else if (key == Qt::Key_Down) {
        stepDown();
    }
}

void QTuiDoubleSpinBox::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    int midX = geo.x() + geo.width() / 2;
    if (event->x() >= midX)
        stepUp();
    else
        stepDown();
}

} // namespace QtTUI
