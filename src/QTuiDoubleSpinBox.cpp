#include <QtTUI/QTuiDoubleSpinBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiDoubleSpinBox::QTuiDoubleSpinBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
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
    if (decimals >= 0 && m_decimals != decimals) {
        m_decimals = decimals;
        markDirty();
    }
}

QString QTuiDoubleSpinBox::prefix() const
{
    return m_prefix;
}

void QTuiDoubleSpinBox::setPrefix(const QString &prefix)
{
    if (m_prefix == prefix)
        return;
    m_prefix = prefix;
    markDirty();
}

QString QTuiDoubleSpinBox::suffix() const
{
    return m_suffix;
}

void QTuiDoubleSpinBox::setSuffix(const QString &suffix)
{
    if (m_suffix == suffix)
        return;
    m_suffix = suffix;
    markDirty();
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
                cell.setChar(U' ');
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
                    cell.setChar(border.topLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.topRight);
                else
                    cell.setChar(border.horizontal);
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
                    cell.setChar(border.bottomLeft);
                else if (col == geoW - 1)
                    cell.setChar(border.bottomRight);
                else
                    cell.setChar(border.horizontal);
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
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
            int pxRight = geoX + geoW - 1;
            if (pxRight < buffer.width()) {
                Cell &cell = buffer.backCell(pxRight, py);
                cell.setChar(border.vertical);
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
                cell.setChar(content.at(i).unicode());
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
    if (event->mouseType() == TuiMouseEvent::Wheel) {
        if (event->wheelDelta() > 0)
            stepUp();
        else if (event->wheelDelta() < 0)
            stepDown();
        return;
    }

    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    // Mirror paintEvent: the content "▲ <prefix><value><suffix> ▼" is centered
    // on the middle row, only when the widget is at least 3x3.
    if (geo.height() < 3 || geo.width() < 3)
        return;

    const QString content = QStringLiteral("\u25B2 ") + m_prefix
                            + QString::number(m_value, 'f', m_decimals)
                            + m_suffix + QStringLiteral(" \u25BC");
    const int contentRow = geo.y() + geo.height() / 2;
    const int innerWidth = geo.width() - 2; // exclude left/right borders
    const int textLen = qMin(static_cast<int>(content.length()), innerWidth);
    const int startX = geo.x() + 1 + (innerWidth - textLen) / 2;

    if (event->y() != contentRow)
        return;
    const int idx = event->x() - startX;
    if (idx < 0 || idx >= textLen)
        return;

    // Step only when the click lands exactly on a painted arrow glyph.
    if (content.at(idx) == QChar(0x25B2)) // ▲
        stepUp();
    else if (content.at(idx) == QChar(0x25BC)) // ▼
        stepDown();
}

} // namespace QtTUI
