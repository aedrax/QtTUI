#ifndef QTTUI_RENDERUTILS_H
#define QTTUI_RENDERUTILS_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QRect>
#include <QString>

namespace QtTUI {

/** A cell style with the two colors and no text attributes. */
Cell cellStyle(const Color &fg, const Color &bg);
/** A cell style with the two colors and the bold, italic and underline of theme. */
Cell cellStyle(const ThemeProperties &theme, const Color &fg, const Color &bg);

/**
 * Sets each cell of rect to a space with the attributes of style. The
 * function clips rect to the buffer.
 */
void fillRect(ScreenBuffer &buffer, const QRect &rect, const Cell &style);

/**
 * Draws a border on the edge of rect with the characters of borderStyle
 * (see Theme_Engine::borderChars) and the attributes of style. The function
 * draws the top row if the height is at least 1, the bottom row if it is at
 * least 2, and the side columns of the rows between them. It clips to the
 * buffer.
 */
void drawBorder(ScreenBuffer &buffer, const QRect &rect, const QString &borderStyle,
                const Cell &style);

/**
 * Draws the frame of a window (a dock widget, an MDI sub-window) on rect
 * with the colors of theme: a border, a bold title from column 2 of the
 * top row, the button text (for example "[X]") at the right end of the top
 * row, and a clear interior. The buttons show only if they fit.
 */
void drawWindowFrame(ScreenBuffer &buffer, const QRect &rect, const ThemeProperties &theme,
                     const QString &title, const QString &buttons);

/**
 * Writes text into the buffer at (x, y), in maxWidth COLUMNS at most, and
 * returns the number of columns that it wrote. The function stops at
 * maxWidth and at the right edge of the buffer, without an ellipsis. A
 * wide character that does not fit is not written. Each character gets the
 * attributes of style.
 *
 * Text with a right-to-left character shows in display order (see
 * QtTuiUnicode::visualOrder). The function cuts the logical end of the
 * text at maxWidth first, then it reorders the text that stays. The text
 * starts at x for each direction: the caller sets the alignment.
 */
int renderText(ScreenBuffer &buffer, int x, int y, const QString &text,
               int maxWidth, const Cell &style);

/**
 * Writes text as a hyperlink to url (OSC 8), in other respects as the
 * function above. A terminal that knows hyperlinks opens the URL when the
 * user clicks the text. An empty url gives text without a link. The link is
 * Cell::link of each cell, so other drawing functions can set it too:
 * style.link = buffer.linkId(url).
 */
int renderText(ScreenBuffer &buffer, int x, int y, const QString &text,
               int maxWidth, const Cell &style, const QString &url);

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
 * Right-to-left text: the function cuts the logical end of the text, adds
 * the ellipsis after it, and then reorders the result (see renderText). So
 * the ellipsis of a right-to-left line is at the left end.
 *
 * Each character is written using the provided style Cell's attributes
 * (foreground, background, bold, italic, etc.) with the appropriate character.
 *
 * mode tells where the ellipsis goes, as in Qt. Qt::ElideRight is the rule
 * above. Qt::ElideLeft writes the ellipsis and then the end of the text.
 * Qt::ElideMiddle writes the start of the text, the ellipsis and the end of
 * the text. Qt::ElideNone cuts the text without an ellipsis, as renderText()
 * does.
 */
void renderTextTruncated(ScreenBuffer &buffer,
                         int x, int y,
                         const QString &text,
                         int maxWidth,
                         const Cell &style,
                         Qt::TextElideMode mode = Qt::ElideRight);

} // namespace QtTUI

#endif // QTTUI_RENDERUTILS_H
