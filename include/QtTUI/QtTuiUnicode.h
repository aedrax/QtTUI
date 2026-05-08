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
 * Known limitations (documented, deliberate):
 * - ZWJ emoji sequences (family, professions) are not merged into a single
 *   cluster; each emoji in the sequence gets its own 2-column cell and the
 *   ZWJ rides along as a zero-width combining suffix.
 * - Regional-indicator flag pairs render as two 1-column cells, not one
 *   2-column flag.
 * - UAX #11 "Ambiguous" width characters are treated as narrow (width 1),
 *   matching wcwidth and most terminal emulators.
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

/** Total columns of a string, iterating by code point (surrogate-aware). */
int stringWidth(const QString &text);

/**
 * A terminal cluster: one base code point plus any directly following
 * zero-width code points (combining marks, variation selectors, ZWJ).
 * This is a pragmatic approximation of a grapheme cluster that is
 * sufficient for cell-grid rendering.
 */
struct Cluster {
    char32_t base = 0;   ///< base code point
    QString combining;   ///< trailing zero-width code points (UTF-16), may be empty
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
