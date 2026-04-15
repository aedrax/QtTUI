#ifndef QTTUI_RENDERUTILS_H
#define QTTUI_RENDERUTILS_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/ScreenBuffer.h>
#include <QString>

namespace QtTUI {

/**
 * Writes text into the buffer at (x, y) truncated to maxWidth cells.
 *
 * - When len(text) > maxWidth and maxWidth >= 2: writes first maxWidth-1 chars
 *   then U+2026 (ellipsis '…').
 * - When maxWidth == 1: writes only U+2026.
 * - When maxWidth <= 0: writes nothing.
 * - When len(text) <= maxWidth: writes the full string without truncation.
 *
 * Each character is written using the provided style Cell's attributes
 * (foreground, background, bold, italic, etc.) with the appropriate character.
 */
void renderTextTruncated(ScreenBuffer &buffer,
                         int x, int y,
                         const QString &text,
                         int maxWidth,
                         const Cell &style);

} // namespace QtTUI

#endif // QTTUI_RENDERUTILS_H
