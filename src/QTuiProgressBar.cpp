#include <QtTUI/QTuiProgressBar.h>
#include <QtTUI/QTuiApplication.h>
#include <QTimer>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

// Time between two steps of the busy block
static constexpr int kBusyStepMs = 100;

QTuiProgressBar::QTuiProgressBar(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_busyTimer(new QTimer(this))
{
    m_busyTimer->setInterval(kBusyStepMs);
    connect(m_busyTimer, &QTimer::timeout, this, [this]() {
        // Reduced motion set while the bar was busy: stop here
        if (QTuiApplication::reducedMotion()) {
            m_busyTimer->stop();
            return;
        }
        // Move the block only while the bar can show (all parents visible)
        for (QTuiWidget *w = this; w; w = w->parentWidget()) {
            if (!w->isVisible())
                return;
        }
        ++m_indeterminatePos;
        markDirty();
    });
    connect(this, &QTuiWidget::visibilityChanged, this, &QTuiProgressBar::updateBusyTimer);
}

void QTuiProgressBar::updateBusyTimer()
{
    // Reduced motion: the busy block stands still
    if (isIndeterminate() && isVisible() && !QTuiApplication::reducedMotion())
        m_busyTimer->start();
    else
        m_busyTimer->stop();
}

int QTuiProgressBar::value() const
{
    return m_value;
}

void QTuiProgressBar::setValue(int value)
{
    if (isIndeterminate())
        return;
    int clamped = qBound(m_minimum, value, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        emit valueChanged(m_value);
        markDirty();
    }
}

int QTuiProgressBar::minimum() const
{
    return m_minimum;
}

void QTuiProgressBar::setMinimum(int min)
{
    if (m_minimum == min)
        return;
    m_minimum = min;
    if (m_maximum < m_minimum)
        m_maximum = m_minimum;
    if (!isIndeterminate())
        setValue(m_value);
    updateBusyTimer();
    markDirty();
}

int QTuiProgressBar::maximum() const
{
    return m_maximum;
}

void QTuiProgressBar::setMaximum(int max)
{
    if (m_maximum == max)
        return;
    m_maximum = max;
    if (m_minimum > m_maximum)
        m_minimum = m_maximum;
    if (!isIndeterminate())
        setValue(m_value);
    updateBusyTimer();
    markDirty();
}

void QTuiProgressBar::setRange(int min, int max)
{
    if (m_minimum == min && m_maximum == qMax(min, max))
        return;
    m_minimum = min;
    m_maximum = qMax(min, max);
    if (!isIndeterminate())
        setValue(m_value);
    updateBusyTimer();
    markDirty();
}

Qt::Orientation QTuiProgressBar::orientation() const
{
    return m_orientation;
}

void QTuiProgressBar::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation)
        return;
    m_orientation = orientation;
    markDirty();
}

QString QTuiProgressBar::format() const
{
    return m_format;
}

void QTuiProgressBar::setFormat(const QString &format)
{
    if (m_format == format)
        return;
    m_format = format;
    markDirty();
}

QChar QTuiProgressBar::fillChar() const
{
    return m_fillChar;
}

void QTuiProgressBar::setFillChar(const QChar &ch)
{
    if (m_fillChar == ch)
        return;
    m_fillChar = ch;
    markDirty();
}

QChar QTuiProgressBar::emptyChar() const
{
    return m_emptyChar;
}

void QTuiProgressBar::setEmptyChar(const QChar &ch)
{
    if (m_emptyChar == ch)
        return;
    m_emptyChar = ch;
    markDirty();
}

bool QTuiProgressBar::isIndeterminate() const
{
    return (m_minimum == 0 && m_maximum == 0);
}

QSize QTuiProgressBar::minimumSize() const
{
    if (m_orientation == Qt::Horizontal) {
        // Border (2) + at least a few cells for the bar
        return QSize(10, 3);
    } else {
        return QSize(3, 5);
    }
}

void QTuiProgressBar::paintEvent(ScreenBuffer &buffer)
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

    // Inner content area (inside borders)
    const int innerX = geoX + 1;
    const int innerY = geoY + 1;
    const int innerW = geoW - 2;
    const int innerH = geoH - 2;

    if (innerW <= 0 || innerH <= 0)
        return;

    if (isIndeterminate()) {
        // Indeterminate mode: bouncing block animation
        // The bounce range is the bar length; position oscillates back and forth
        int barLen = (m_orientation == Qt::Horizontal) ? innerW : innerH;
        int blockSize = qMax(1, barLen / 5); // bouncing block is ~20% of bar
        int travelRange = barLen - blockSize;
        if (travelRange < 1) travelRange = 1;

        // Oscillate: period = 2 * travelRange
        int period = 2 * travelRange;
        int phase = m_indeterminatePos % period;
        int blockStart = (phase < travelRange) ? phase : (period - phase);

        if (m_orientation == Qt::Horizontal) {
            int contentRow = innerY + innerH / 2;
            for (int col = 0; col < innerW; ++col) {
                int px = innerX + col;
                if (px >= buffer.width() || contentRow >= buffer.height())
                    continue;
                Cell &cell = buffer.backCell(px, contentRow);
                cell.setChar(((col >= blockStart && col < blockStart + blockSize) ? m_fillChar : m_emptyChar).unicode());
                cell.foreground = fg;
                cell.background = bg;
            }
        } else {
            int contentCol = innerX + innerW / 2;
            for (int row = 0; row < innerH; ++row) {
                int py = innerY + row;
                if (contentCol >= buffer.width() || py >= buffer.height())
                    continue;
                Cell &cell = buffer.backCell(contentCol, py);
                // Vertical fills bottom-up, so invert
                int invertedRow = innerH - 1 - row;
                cell.setChar(((invertedRow >= blockStart && invertedRow < blockStart + blockSize) ? m_fillChar : m_emptyChar).unicode());
                cell.foreground = fg;
                cell.background = bg;
            }
        }
        return;
    }

    // Determinate mode
    int range = m_maximum - m_minimum;
    double fraction = (range > 0) ? static_cast<double>(m_value - m_minimum) / range : 0.0;

    // Build format text
    QString formatText;
    if (!m_format.isEmpty()) {
        formatText = m_format;
        int percent = (range > 0) ? static_cast<int>(fraction * 100.0 + 0.5) : 0;
        formatText.replace(QStringLiteral("%p"), QString::number(percent));
        formatText.replace(QStringLiteral("%v"), QString::number(m_value));
        formatText.replace(QStringLiteral("%m"), QString::number(m_maximum));
    }

    if (m_orientation == Qt::Horizontal) {
        int filledCols = static_cast<int>(fraction * innerW + 0.5);
        filledCols = qBound(0, filledCols, innerW);

        // Render bar on the middle row(s)
        int contentRow = innerY + innerH / 2;
        for (int col = 0; col < innerW; ++col) {
            int px = innerX + col;
            if (px >= buffer.width() || contentRow >= buffer.height())
                continue;
            Cell &cell = buffer.backCell(px, contentRow);
            cell.setChar(((col < filledCols) ? m_fillChar : m_emptyChar).unicode());
            cell.foreground = fg;
            cell.background = bg;
        }

        // Overlay format text centered on the bar
        if (!formatText.isEmpty()) {
            int textLen = qMin(static_cast<int>(formatText.length()), innerW);
            int startCol = (innerW - textLen) / 2;
            for (int i = 0; i < textLen; ++i) {
                int px = innerX + startCol + i;
                if (px >= buffer.width() || contentRow >= buffer.height())
                    continue;
                Cell &cell = buffer.backCell(px, contentRow);
                cell.setChar(formatText.at(i).unicode());
                cell.bold = true;
            }
        }
    } else {
        // Vertical: fill from bottom up
        int filledRows = static_cast<int>(fraction * innerH + 0.5);
        filledRows = qBound(0, filledRows, innerH);

        int contentCol = innerX + innerW / 2;
        for (int row = 0; row < innerH; ++row) {
            int py = innerY + row;
            if (contentCol >= buffer.width() || py >= buffer.height())
                continue;
            Cell &cell = buffer.backCell(contentCol, py);
            // Bottom-up: the bottom `filledRows` rows are filled
            bool filled = (row >= innerH - filledRows);
            cell.setChar((filled ? m_fillChar : m_emptyChar).unicode());
            cell.foreground = fg;
            cell.background = bg;
        }

        // Overlay format text vertically centered
        if (!formatText.isEmpty()) {
            int textLen = qMin(static_cast<int>(formatText.length()), innerH);
            int startRow = (innerH - textLen) / 2;
            for (int i = 0; i < textLen; ++i) {
                int py = innerY + startRow + i;
                if (contentCol >= buffer.width() || py >= buffer.height())
                    continue;
                Cell &cell = buffer.backCell(contentCol, py);
                cell.setChar(formatText.at(i).unicode());
                cell.bold = true;
            }
        }
    }
}

} // namespace QtTUI
