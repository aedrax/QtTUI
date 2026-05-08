#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>

namespace QtTUI {

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
        int col = 0;
        int i = 0;
        QtTuiUnicode::Cluster c;
        while (QtTuiUnicode::nextCluster(text, i, c)) {
            buffer.putChar(x + col, y, c.base, c.combining, style);
            col += c.width;
        }
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
