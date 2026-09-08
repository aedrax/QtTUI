#include <QtTUI/QTuiLCDNumber.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiLCDNumber::QTuiLCDNumber(int digitCount, QTuiWidget *parent)
    : QTuiFrame(parent)
    , m_digitCount(qMax(1, digitCount))
{
    setFrameShape(Box); // paintEvent() draws the border itself
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

    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    fillRect(buffer, geo, cellStyle(theme, fg, bg));

    // Border: the LCD number is a Box frame
    drawBorder(buffer, geo, theme.borderStyle, cellStyle(theme, fg, bg));

    // Render segment art for each character in display text
    // Inner area starts at (geo.x()+1, geo.y()+1), size (geo.width()-2, geo.height()-2)
    if (geo.height() < 5 || geo.width() < 3)
        return; // Not enough space for content

    int innerW = geo.width() - 2;

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
    Cell segmentStyle = cellStyle(theme, fg, bg);
    segmentStyle.bold = true;
    for (int line = 0; line < 3; ++line) {
        int col = startCol;
        for (int d = 0; d < allSegments.size(); ++d) {
            const QString &artLine = allSegments[d][line];
            renderText(buffer, geo.x() + col, geo.y() + 1 + line, artLine.left(3), geo.width() - 1 - col, segmentStyle);
            col += 3;
            // Add gap between digits
            if (d < allSegments.size() - 1)
                col += 1;
        }
    }
}

} // namespace QtTUI
