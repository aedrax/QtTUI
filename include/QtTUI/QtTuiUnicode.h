#ifndef QTTUI_QTTUIUNICODE_H
#define QTTUI_QTTUIUNICODE_H

#include <QtTUI/QtTUIGlobal.h>
#include <QString>

namespace QtTUI {

/**
 * Terminal column-width helpers for Unicode text.
 *
 * Terminals are cell grids: every code point occupies 0, 1 or 2 columns.
 * These helpers implement a wcwidth-style width function (Markus Kuhn's
 * wcwidth plus UAX #11 East Asian Width W/F ranges and emoji presentation
 * ranges) and the string/index/column arithmetic built on top of it.
 *
 * Emoji sequences are one cluster of 2 columns: a ZWJ sequence (family,
 * professions), a flag (two regional indicators), an emoji with a skin
 * tone modifier, and a text-default emoji with variation selector 16.
 *
 * Known limitations (documented, deliberate):
 * - Terminals do not agree on the width of emoji sequences. A terminal
 *   without grapheme clusters shows a ZWJ sequence as separate emoji, wider
 *   than the 2 columns that QtTUI gives it.
 * - A keycap sequence (a digit, VS16, U+20E3) is narrow (width 1).
 * - One regional indicator without a partner is narrow (width 1).
 * - The emoji base test uses Unicode blocks, not the exact
 *   Extended_Pictographic property.
 * - UAX #11 "Ambiguous" width characters are treated as narrow (width 1),
 *   matching wcwidth and most terminal emulators.
 * - Text is not reordered for right-to-left scripts (no bidi).
 */
namespace QtTuiUnicode {

/**
 * Columns occupied by a single code point: 0 for combining marks and other
 * zero-width characters (category Mn/Me/Cf except U+00AD, Hangul jamo
 * vowels/finals), 2 for East Asian Wide/Fullwidth and wide emoji, else 1.
 * Returns 0 for U+0000 (the ScreenBuffer continuation marker).
 */
int charWidth(char32_t ch);

/** True when charWidth(ch) == 0 and ch != 0. */
bool isZeroWidth(char32_t ch);

/**
 * Columns of a cluster: a base code point and the code points that join it
 * (see Cluster). This is charWidth(base), but 2 for an emoji sequence with a
 * narrow base: a flag, a text-default emoji with variation selector 16, or
 * a narrow emoji with a skin tone or a ZWJ sequence. The result is 1 or 2.
 */
int clusterWidth(char32_t base, const QString &combining);

/**
 * The text without control characters (category Cc: U+0000 to U+001F and
 * U+007F to U+009F). Use it for text that goes into an escape sequence (a
 * window title, a link URL), so the text cannot end the sequence or start
 * a new one.
 */
QString withoutControlChars(const QString &text);

/** Total columns of a string: the sum of its cluster widths (see nextCluster). */
int stringWidth(const QString &text);

/**
 * A terminal cluster: one base code point plus the code points that join
 * it. These are the zero-width code points directly after it (combining
 * marks, variation selectors, ZWJ) and the other parts of an emoji
 * sequence: the emoji after a ZWJ, a skin tone modifier, and the second
 * regional indicator of a flag.
 * This is a pragmatic approximation of a grapheme cluster that is
 * sufficient for cell-grid rendering.
 */
struct Cluster {
    char32_t base = 0;   ///< base code point
    QString combining;   ///< the joined code points after the base (UTF-16), may be empty
    int index = 0;       ///< start position in the source string (code units)
    int length = 0;      ///< length in the source string (code units)
    int width = 1;       ///< columns occupied (1 or 2)
};

/**
 * Decodes the cluster starting at code-unit position `index` and advances
 * `index` past it. Returns false when `index` is at or past the end.
 * A string consisting only of cluster boundaries is guaranteed: every
 * code unit belongs to exactly one cluster.
 */
bool nextCluster(const QString &text, int &index, Cluster &out);

/**
 * Display column of the cluster containing code-unit `index` (i.e. the sum
 * of widths of all clusters that end at or before `index`). An index past
 * the end returns stringWidth(text).
 */
int columnOfIndex(const QString &text, int index);

/**
 * Code-unit index of the cluster whose span covers display column `column`
 * (snapping into the middle of a wide character returns that character's
 * start). A column at or past the end returns text.size().
 */
int indexOfColumn(const QString &text, int column);

/** Cluster boundary after the cluster containing `index` (clamped to size). */
int nextClusterIndex(const QString &text, int index);

/** Cluster boundary before `index` (clamped to 0). */
int prevClusterIndex(const QString &text, int index);

} // namespace QtTuiUnicode

} // namespace QtTUI

#endif // QTTUI_QTTUIUNICODE_H
