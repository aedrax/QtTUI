#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiSpinBox::QTuiSpinBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

int QTuiSpinBox::value() const
{
    return m_value;
}

void QTuiSpinBox::setValue(int value)
{
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

int QTuiSpinBox::minimum() const
{
    return m_minimum;
}

void QTuiSpinBox::setMinimum(int min)
{
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    setValue(m_value); // re-clamp
}

int QTuiSpinBox::maximum() const
{
    return m_maximum;
}

void QTuiSpinBox::setMaximum(int max)
{
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    setValue(m_value); // re-clamp
}

void QTuiSpinBox::setRange(int min, int max)
{
    m_minimum = min;
    m_maximum = qMax(min, max);
    setValue(m_value); // re-clamp
}

int QTuiSpinBox::singleStep() const
{
    return m_singleStep;
}

void QTuiSpinBox::setSingleStep(int step)
{
    if (step > 0)
        m_singleStep = step;
}

bool QTuiSpinBox::wrapping() const
{
    return m_wrapping;
}

void QTuiSpinBox::setWrapping(bool wrapping)
{
    m_wrapping = wrapping;
}

QString QTuiSpinBox::prefix() const
{
    return m_prefix;
}

void QTuiSpinBox::setPrefix(const QString &prefix)
{
    if (m_prefix == prefix)
        return;
    m_prefix = prefix;
    markDirty();
}

QString QTuiSpinBox::suffix() const
{
    return m_suffix;
}

void QTuiSpinBox::setSuffix(const QString &suffix)
{
    if (m_suffix == suffix)
        return;
    m_suffix = suffix;
    markDirty();
}

void QTuiSpinBox::stepUp()
{
    int newValue = m_value + m_singleStep;
    if (newValue > m_maximum) {
        if (m_wrapping)
            newValue = m_minimum;
        else
            newValue = m_maximum;
    }
    setValue(newValue);
}

void QTuiSpinBox::stepDown()
{
    int newValue = m_value - m_singleStep;
    if (newValue < m_minimum) {
        if (m_wrapping)
            newValue = m_maximum;
        else
            newValue = m_minimum;
    }
    setValue(newValue);
}

QSize QTuiSpinBox::minimumSize() const
{
    // Content: "▲ " + prefix + value + suffix + " ▼"
    // Value display: max digits from min/max range
    QString maxStr = QString::number(m_maximum);
    QString minStr = QString::number(m_minimum);
    int valueWidth = qMax(maxStr.length(), minStr.length());
    int contentWidth = 2 + m_prefix.length() + valueWidth + m_suffix.length() + 2; // "▲ " + content + " ▼"

    // Add 2 for left/right border, 1 padding each side
    return QSize(contentWidth + 4, 3); // 3 rows: top border, content, bottom border
}

void QTuiSpinBox::paintEvent(ScreenBuffer &buffer)
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

    // Determine colors based on focus state
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
    QString valueStr = QString::number(m_value);
    QString content = QStringLiteral("\u25B2 ") + m_prefix + valueStr + m_suffix + QStringLiteral(" \u25BC");

    // Render content centered in the middle row
    if (geoH >= 3 && geoW >= 3) {
        int contentRow = geoY + geoH / 2;
        int innerWidth = geoW - 2; // Exclude left/right borders
        int textLen = qMin(static_cast<int>(content.length()), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center within inner area

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

void QTuiSpinBox::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Up) {
        stepUp();
    } else if (key == Qt::Key_Down) {
        stepDown();
    }
}

void QTuiSpinBox::mousePressEvent(TuiMouseEvent *event)
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

    const QString content = QStringLiteral("\u25B2 ") + m_prefix + QString::number(m_value)
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
