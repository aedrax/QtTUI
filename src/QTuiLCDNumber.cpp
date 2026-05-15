#include <QtTUI/QTuiLCDNumber.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiLCDNumber::QTuiLCDNumber(int digitCount, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_digitCount(qMax(1, digitCount))
{
    m_displayText = QStringLiteral("0");
}

void QTuiLCDNumber::display(int value)
{
    m_intValue = value;
    m_doubleValue = static_cast<double>(value);

    switch (m_mode) {
    case Hexadecimal:
        m_displayText = QString::number(qAbs(value), 16).toUpper();
        break;
    case Octal:
        m_displayText = QString::number(qAbs(value), 8);
        break;
    case Binary:
        m_displayText = QString::number(qAbs(value), 2);
        break;
    default:
        m_displayText = QString::number(qAbs(value));
        break;
    }

    if (value < 0)
        m_displayText.prepend(QLatin1Char('-'));

    if (checkOverflow(value))
        emit overflow();
    markDirty();
}

void QTuiLCDNumber::display(double value)
{
    m_intValue = static_cast<int>(value);
    m_doubleValue = value;

    // Format with reasonable precision, trim trailing zeros
    m_displayText = QString::number(qAbs(value), 'f', 6);
    // Remove trailing zeros after decimal point
    if (m_displayText.contains(QLatin1Char('.'))) {
        while (m_displayText.endsWith(QLatin1Char('0')))
            m_displayText.chop(1);
        if (m_displayText.endsWith(QLatin1Char('.')))
            m_displayText.chop(1);
    }

    if (value < 0.0)
        m_displayText.prepend(QLatin1Char('-'));

    if (checkOverflow(value))
        emit overflow();
    markDirty();
}

void QTuiLCDNumber::display(const QString &text)
{
    m_displayText = text;
    m_intValue = text.toInt();
    m_doubleValue = text.toDouble();

    // Check overflow based on text length vs digit count
    int effectiveLen = text.length();
    // Decimal point doesn't count as a digit
    if (text.contains(QLatin1Char('.')))
        --effectiveLen;
    if (text.startsWith(QLatin1Char('-')))
        --effectiveLen;

    if (effectiveLen > m_digitCount)
        emit overflow();
    markDirty();
}

int QTuiLCDNumber::digitCount() const
{
    return m_digitCount;
}

void QTuiLCDNumber::setDigitCount(int count)
{
    count = qMax(1, count);
    if (m_digitCount == count)
        return;
    m_digitCount = count;
    markDirty();
}

QTuiLCDNumber::Mode QTuiLCDNumber::mode() const
{
    return m_mode;
}

void QTuiLCDNumber::setMode(Mode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    // Re-display with new mode (marks dirty)
    display(m_intValue);
}

int QTuiLCDNumber::intValue() const
{
    return m_intValue;
}

double QTuiLCDNumber::doubleValue() const
{
    return m_doubleValue;
}

bool QTuiLCDNumber::checkOverflow(int value) const
{
    QString str;
    switch (m_mode) {
    case Hexadecimal:
        str = QString::number(qAbs(value), 16);
        break;
    case Octal:
        str = QString::number(qAbs(value), 8);
        break;
    case Binary:
        str = QString::number(qAbs(value), 2);
        break;
    default:
        str = QString::number(qAbs(value));
        break;
    }
    return str.length() > m_digitCount;
}

bool QTuiLCDNumber::checkOverflow(double value) const
{
    QString str = QString::number(qAbs(value), 'f', 6);
    // Remove trailing zeros
    if (str.contains(QLatin1Char('.'))) {
        while (str.endsWith(QLatin1Char('0')))
            str.chop(1);
        if (str.endsWith(QLatin1Char('.')))
            str.chop(1);
    }
    // Count digits only (exclude decimal point)
    int digitLen = str.length();
    if (str.contains(QLatin1Char('.')))
        --digitLen;
    return digitLen > m_digitCount;
}

QStringList QTuiLCDNumber::segmentArt(QChar digit)
{
    // Each digit is 3 lines tall, 3 characters wide
    // Seven-segment ASCII art representations
    switch (digit.toLatin1()) {
    case '0': return { QStringLiteral(" _ "), QStringLiteral("| |"), QStringLiteral("|_|") };
    case '1': return { QStringLiteral("   "), QStringLiteral("  |"), QStringLiteral("  |") };
    case '2': return { QStringLiteral(" _ "), QStringLiteral(" _|"), QStringLiteral("|_ ") };
    case '3': return { QStringLiteral(" _ "), QStringLiteral(" _|"), QStringLiteral(" _|") };
    case '4': return { QStringLiteral("   "), QStringLiteral("|_|"), QStringLiteral("  |") };
    case '5': return { QStringLiteral(" _ "), QStringLiteral("|_ "), QStringLiteral(" _|") };
    case '6': return { QStringLiteral(" _ "), QStringLiteral("|_ "), QStringLiteral("|_|") };
    case '7': return { QStringLiteral(" _ "), QStringLiteral("  |"), QStringLiteral("  |") };
    case '8': return { QStringLiteral(" _ "), QStringLiteral("|_|"), QStringLiteral("|_|") };
    case '9': return { QStringLiteral(" _ "), QStringLiteral("|_|"), QStringLiteral(" _|") };
    case 'A': case 'a': return { QStringLiteral(" _ "), QStringLiteral("|_|"), QStringLiteral("| |") };
    case 'B': case 'b': return { QStringLiteral("   "), QStringLiteral("|_ "), QStringLiteral("|_|") };
    case 'C': case 'c': return { QStringLiteral(" _ "), QStringLiteral("|  "), QStringLiteral("|_ ") };
    case 'D': case 'd': return { QStringLiteral("   "), QStringLiteral(" _|"), QStringLiteral("|_|") };
    case 'E': case 'e': return { QStringLiteral(" _ "), QStringLiteral("|_ "), QStringLiteral("|_ ") };
    case 'F': case 'f': return { QStringLiteral(" _ "), QStringLiteral("|_ "), QStringLiteral("|  ") };
    case '-':           return { QStringLiteral("   "), QStringLiteral(" _ "), QStringLiteral("   ") };
    case '.':           return { QStringLiteral("   "), QStringLiteral("   "), QStringLiteral(" . ") };
    default:            return { QStringLiteral("   "), QStringLiteral("   "), QStringLiteral("   ") };
    }
}

QSize QTuiLCDNumber::minimumSize() const
{
    // Each digit is 3 chars wide, plus 1 char gap between digits
    // Border: 2 chars horizontal, 2 chars vertical
    int numChars = m_digitCount;
    int contentWidth = numChars * 3 + (numChars - 1); // digits + gaps
    return QSize(contentWidth + 2, 5); // +2 for borders, 5 = top border + 3 digit rows + bottom border
}

void QTuiLCDNumber::paintEvent(ScreenBuffer &buffer)
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

    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    // Clear entire widget area
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

    // Top border
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

    // Bottom border
    if (geoH >= 2) {
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

    // Left and right borders for middle rows
    for (int row = 1; row < geoH - 1; ++row) {
        int py = geoY + row;
        if (py < buffer.height()) {
            int pxL = geoX;
            if (pxL < buffer.width()) {
                Cell &cell = buffer.backCell(pxL, py);
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
            int pxR = geoX + geoW - 1;
            if (pxR < buffer.width()) {
                Cell &cell = buffer.backCell(pxR, py);
                cell.setChar(border.vertical);
                cell.foreground = fg;
                cell.background = bg;
            }
        }
    }

    // Render segment art for each character in display text
    // Inner area starts at (geoX+1, geoY+1), size (geoW-2, geoH-2)
    if (geoH < 5 || geoW < 3)
        return; // Not enough space for content

    int innerW = geoW - 2;

    // Pad or truncate display text to fit digit count
    QString displayStr = m_displayText;

    // Right-align the display text within digitCount positions
    // Build the segment art for each character
    QList<QStringList> allSegments;
    for (int i = 0; i < displayStr.length(); ++i) {
        allSegments.append(segmentArt(displayStr.at(i)));
    }

    // Calculate total width: each char is 3 wide, 1 gap between
    int totalWidth = 0;
    if (!allSegments.isEmpty())
        totalWidth = allSegments.size() * 3 + (allSegments.size() - 1);

    // Right-align within inner area
    int startCol = 1 + (innerW - totalWidth);
    if (startCol < 1)
        startCol = 1;

    // Render 3 rows of segment art
    for (int line = 0; line < 3; ++line) {
        int col = startCol;
        for (int d = 0; d < allSegments.size(); ++d) {
            const QString &artLine = allSegments[d][line];
            for (int c = 0; c < artLine.length() && c < 3; ++c) {
                int px = geoX + col + c;
                int py = geoY + 1 + line;
                if (px < buffer.width() && py < buffer.height() && (col + c) < geoW - 1) {
                    Cell &cell = buffer.backCell(px, py);
                    cell.setChar(artLine.at(c).unicode());
                    cell.foreground = fg;
                    cell.background = bg;
                    cell.bold = true;
                }
            }
            col += 3;
            // Add gap between digits
            if (d < allSegments.size() - 1)
                col += 1;
        }
    }
}

} // namespace QtTUI
