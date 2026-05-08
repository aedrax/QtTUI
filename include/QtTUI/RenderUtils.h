#ifndef QTTUI_RENDERUTILS_H
#define QTTUI_RENDERUTILS_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/ScreenBuffer.h>
#include <QString>

namespace QtTUI {

/**
 * Writes text into the buffer at (x, y) truncated to maxWidth COLUMNS.
 *
 * Width-aware: wide (East Asian / emoji) characters occupy two columns
 * (glyph + continuation cell, written via ScreenBuffer::putChar) and
 * combining marks attach to the preceding character's cell. All limits
 * below are display columns (QtTuiUnicode::stringWidth), which equal the
 * character count for narrow-only text:
 *
 * - When width(text) > maxWidth and maxWidth >= 2: fills the first
 *   maxWidth-1 columns with the text prefix, then U+2026 (ellipsis '…').
 *   A wide character straddling the truncation boundary becomes a space.
 * - When maxWidth == 1: writes only U+2026.
 * - When maxWidth <= 0: writes nothing.
 * - When width(text) <= maxWidth: writes the full string without truncation.
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
