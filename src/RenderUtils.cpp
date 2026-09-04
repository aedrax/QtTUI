#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>

namespace QtTUI {

Cell cellStyle(const Color &fg, const Color &bg)
{
    Cell style;
    style.foreground = fg;
    style.background = bg;
    return style;
}

Cell cellStyle(const ThemeProperties &theme, const Color &fg, const Color &bg)
{
    Cell style = cellStyle(fg, bg);
    style.bold = theme.bold;
    style.italic = theme.italic;
    style.underline = theme.underline;
    return style;
}

void fillRect(ScreenBuffer &buffer, const QRect &rect, const Cell &style)
{
    // QRect::intersected() normalizes a negative size, so check it first
    if (rect.isEmpty())
        return;
    const QRect area = rect.intersected(QRect(0, 0, buffer.width(), buffer.height()));
    for (int y = area.top(); y <= area.bottom(); ++y)
        for (int x = area.left(); x <= area.right(); ++x)
            buffer.putChar(x, y, U' ', style);
}

void drawBorder(ScreenBuffer &buffer, const QRect &rect, const QString &borderStyle,
                const Cell &style)
{
    const auto border = Theme_Engine::borderChars(borderStyle);
    const int x0 = rect.x();
    const int y0 = rect.y();
    const int w = rect.width();
    const int h = rect.height();
    auto put = [&](int x, int y, char32_t ch) {
        if (x >= 0 && y >= 0 && x < buffer.width() && y < buffer.height())
            buffer.putChar(x, y, ch, style);
    };

    if (h >= 1) {
        for (int col = 0; col < w; ++col)
            put(x0 + col, y0, col == 0 ? border.topLeft
                              : col == w - 1 ? border.topRight : border.horizontal);
    }
    if (h >= 2) {
        for (int col = 0; col < w; ++col)
            put(x0 + col, y0 + h - 1, col == 0 ? border.bottomLeft
                                      : col == w - 1 ? border.bottomRight : border.horizontal);
    }
    for (int row = 1; row < h - 1; ++row) {
        put(x0, y0 + row, border.vertical);
        put(x0 + w - 1, y0 + row, border.vertical);
    }
}

void drawWindowFrame(ScreenBuffer &buffer, const QRect &rect, const ThemeProperties &theme,
                     const QString &title, const QString &buttons)
{
    const Cell plain = cellStyle(theme.foreground, theme.background);
    Cell chrome = plain;
    chrome.bold = theme.bold;
    drawBorder(buffer, rect, theme.borderStyle, chrome);
    fillRect(buffer, rect.adjusted(1, 1, -1, -1), plain);

    const int buttonsWidth = QtTuiUnicode::stringWidth(buttons);
    if (!title.isEmpty() && rect.width() > 4) {
        // Leave the borders, the buttons and a space before the buttons
        Cell titleStyle = plain;
        titleStyle.bold = true;
        renderText(buffer, rect.x() + 2, rect.y(), title, rect.width() - 5 - buttonsWidth, titleStyle);
    }
    // The buttons end before the right border
    if (rect.width() >= buttonsWidth + 3)
        renderText(buffer, rect.right() - buttonsWidth, rect.y(), buttons, buttonsWidth, chrome);
}

int renderText(ScreenBuffer &buffer, int x, int y, const QString &text,
               int maxWidth, const Cell &style)
{
    if (y < 0 || y >= buffer.height())
        return 0;
    maxWidth = qMin(maxWidth, buffer.width() - x);
    int col = 0;
    int i = 0;
    QtTuiUnicode::Cluster c;
    while (QtTuiUnicode::nextCluster(text, i, c)) {
        if (col + c.width > maxWidth)
            break;
        if (x + col >= 0)
            buffer.putChar(x + col, y, c.base, c.combining, style);
        col += c.width;
    }
    return col;
}

void renderTextTruncated(ScreenBuffer &buffer,
                         int x, int y,
                         const QString &text,
                         int maxWidth,
                         const Cell &style)
{
    if (maxWidth <= 0)
        return;

    static constexpr char32_t ellipsis = U'…'; // U+2026 '…'

    const int totalCols = QtTuiUnicode::stringWidth(text);

    if (totalCols <= maxWidth) {
        // Full string fits — write without truncation
        renderText(buffer, x, y, text, maxWidth, style);
        return;
    }

    if (maxWidth == 1) {
        // Only room for the ellipsis
        buffer.putChar(x, y, ellipsis, style);
        return;
    }

    // maxWidth >= 2 and text is wider: write a prefix filling maxWidth-1
    // columns, then the ellipsis. A wide character that would straddle the
    // truncation boundary is replaced by a space.
    const int textCols = maxWidth - 1;
    int col = 0;
    int i = 0;
    QtTuiUnicode::Cluster c;
    while (col < textCols && QtTuiUnicode::nextCluster(text, i, c)) {
        if (col + c.width > textCols) {
            buffer.putChar(x + col, y, U' ', style);
            ++col;
            break;
        }
        buffer.putChar(x + col, y, c.base, c.combining, style);
        col += c.width;
    }
    // Defensive: pad any remaining gap (cannot normally occur because the
    // text is known to be wider than maxWidth)
    for (; col < textCols; ++col)
        buffer.putChar(x + col, y, U' ', style);

    buffer.putChar(x + textCols, y, ellipsis, style);
}

} // namespace QtTUI
