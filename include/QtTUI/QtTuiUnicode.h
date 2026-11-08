#ifndef QTTUI_QTTUIUNICODE_H
#define QTTUI_QTTUIUNICODE_H

#include <QtTUI/QtTUIGlobal.h>
#include <QList>
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
 * - visualOrder() does not do the explicit levels of UAX #9 and does not
 *   shape Arabic letters (see visualOrder).
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

/**
 * The switch for the reorder of right-to-left text in text paint
 * (renderText(), renderTextTruncated(), QTuiLabel). It has no effect on
 * visualOrder() itself.
 *
 * Turn the reorder off for a terminal that reorders the text itself and
 * has no explicit bidi mode. If not, the text shows reversed two times.
 * The start value comes from the environment variable QTTUI_BIDI (0 is
 * off, another number is on). Without the variable the reorder is on, but
 * it is off in Konsole (the variable KONSOLE_VERSION is set): Konsole
 * reorders the text itself and has no explicit mode.
 *
 * Set the switch before you make the QTuiApplication. The terminal backend
 * sends the bidi mode of the terminal (see bidiModeSequence()) only when
 * it starts, and a change does not repaint the widgets.
 */
bool bidiEnabled();
void setBidiEnabled(bool enabled);

/**
 * True if the text has a character that visualOrder() moves: a
 * right-to-left letter (Hebrew, Arabic and others) or an Arabic number.
 * This is one scan of the code units with no allocation. Text paint uses it
 * to skip the reorder for left-to-right text.
 */
bool hasRightToLeft(const QString &text);

/**
 * The line in display order: the Unicode Bidirectional Algorithm (UAX #9)
 * for one line. Right-to-left runs are reversed, and a mirrored character
 * (a bracket) in such a run changes to its mirror character. A cluster
 * stays together, with the base before its marks.
 *
 * base is the paragraph direction. Qt::LayoutDirectionAuto uses the first
 * strong character (rules P2 and P3), and left-to-right if there is none.
 * A line with no right-to-left character and a base that is not
 * Qt::RightToLeft returns unchanged, with no allocation.
 *
 * The rules: P2, P3, W1 to W7, N0 to N2, I1, I2, L2 and L4.
 *
 * Known limitations (documented, deliberate):
 * - No explicit levels (rules X1 to X8). The function ignores the
 *   embedding, override and isolate control characters (U+202A to U+202E,
 *   U+2066 to U+2069). The marks LRM, RLM and ALM work.
 * - A bracket pair (rule N0) is a character of the category Ps and a
 *   character of the category Pe that are mirror characters of each other.
 *   This is not the exact Bidi_Paired_Bracket property.
 * - No rule L1 for a tab or a paragraph separator in the line.
 * - No Arabic shaping (joined letter forms). A terminal that shapes text
 *   does this itself.
 */
QString visualOrder(const QString &line, Qt::LayoutDirection base = Qt::LayoutDirectionAuto);

/**
 * The clusters of the line in display order (see visualOrder). The index
 * of each cluster is its position in line, so a caller can find the source
 * of each cell. The base of a mirrored cluster is the mirror character.
 */
QList<Cluster> visualClusters(const QString &line, Qt::LayoutDirection base = Qt::LayoutDirectionAuto);

} // namespace QtTuiUnicode

} // namespace QtTUI

#endif // QTTUI_QTTUIUNICODE_H
