#include <QtTUI/RenderUtils.h>

namespace QtTUI {

void renderTextTruncated(ScreenBuffer &buffer,
                         int x, int y,
                         const QString &text,
                         int maxWidth,
                         const Cell &style)
{
    if (maxWidth <= 0)
        return;

    const int len = text.length();
    static const QChar ellipsis = QChar(0x2026); // U+2026 '…'

    if (len <= maxWidth) {
        // Full string fits — write without truncation
        for (int i = 0; i < len; ++i) {
            Cell cell = style;
            cell.character = text.at(i);
            buffer.backCell(x + i, y) = cell;
        }
    } else if (maxWidth == 1) {
        // Only room for the ellipsis
        Cell cell = style;
        cell.character = ellipsis;
        buffer.backCell(x, y) = cell;
    } else {
        // maxWidth >= 2 and text is longer: write prefix + ellipsis
        for (int i = 0; i < maxWidth - 1; ++i) {
            Cell cell = style;
            cell.character = text.at(i);
            buffer.backCell(x + i, y) = cell;
        }
        Cell cell = style;
        cell.character = ellipsis;
        buffer.backCell(x + maxWidth - 1, y) = cell;
    }
}

} // namespace QtTUI
