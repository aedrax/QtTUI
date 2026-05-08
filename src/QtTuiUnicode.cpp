#include <QtTUI/QtTuiUnicode.h>

#include <QChar>
#include <algorithm>
#include <iterator>

namespace QtTUI {
namespace QtTuiUnicode {

namespace {

struct Interval {
    char32_t first;
    char32_t last;
};

// East Asian Wide (W) / Fullwidth (F) ranges plus emoji-presentation ranges
// that terminals render double-width. Based on Markus Kuhn's wcwidth
// (https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c) updated with UAX #11
// East Asian Width data and Unicode emoji-data wide ranges, as used by
// notcurses/tmux. Sorted, non-overlapping; queried by binary search.
constexpr Interval kWideRanges[] = {
    { 0x1100, 0x115F },   // Hangul Jamo leading consonants
    { 0x231A, 0x231B },   // watch, hourglass
    { 0x2329, 0x232A },   // angle brackets
    { 0x23E9, 0x23EC },   // fast-forward .. fast-down (emoji wide)
    { 0x23F0, 0x23F0 },   // alarm clock
    { 0x23F3, 0x23F3 },   // hourglass with flowing sand
    { 0x25FD, 0x25FE },   // small squares (emoji wide)
    { 0x2614, 0x2615 },   // umbrella with rain, hot beverage
    { 0x2648, 0x2653 },   // zodiac signs
    { 0x267F, 0x267F },   // wheelchair symbol
    { 0x2693, 0x2693 },   // anchor
    { 0x26A1, 0x26A1 },   // high voltage
    { 0x26AA, 0x26AB },   // circles (emoji wide)
    { 0x26BD, 0x26BE },   // soccer ball, baseball
    { 0x26C4, 0x26C5 },   // snowman, sun behind cloud
    { 0x26CE, 0x26CE },   // ophiuchus
    { 0x26D4, 0x26D4 },   // no entry
    { 0x26EA, 0x26EA },   // church
    { 0x26F2, 0x26F3 },   // fountain, golf flag
    { 0x26F5, 0x26F5 },   // sailboat
    { 0x26FA, 0x26FA },   // tent
    { 0x26FD, 0x26FD },   // fuel pump
    { 0x2705, 0x2705 },   // check mark button
    { 0x270A, 0x270B },   // raised fist, raised hand
    { 0x2728, 0x2728 },   // sparkles
    { 0x274C, 0x274C },   // cross mark
    { 0x274E, 0x274E },   // cross mark button
    { 0x2753, 0x2755 },   // question/exclamation marks (emoji)
    { 0x2757, 0x2757 },   // exclamation mark (emoji)
    { 0x2795, 0x2797 },   // heavy plus/minus/division
    { 0x27B0, 0x27B0 },   // curly loop
    { 0x27BF, 0x27BF },   // double curly loop
    { 0x2B1B, 0x2B1C },   // large squares
    { 0x2B50, 0x2B50 },   // star
    { 0x2B55, 0x2B55 },   // heavy large circle
    { 0x2E80, 0x303E },   // CJK Radicals .. CJK Symbols and Punctuation
    { 0x3041, 0x33FF },   // Hiragana .. CJK Compatibility (0x3040/0x303F narrow)
    { 0x3400, 0x4DBF },   // CJK Extension A
    { 0x4E00, 0x9FFF },   // CJK Unified Ideographs
    { 0xA000, 0xA4CF },   // Yi Syllables, Yi Radicals
    { 0xA960, 0xA97F },   // Hangul Jamo Extended-A
    { 0xAC00, 0xD7A3 },   // Hangul Syllables
    { 0xF900, 0xFAFF },   // CJK Compatibility Ideographs
    { 0xFE10, 0xFE19 },   // Vertical Forms
    { 0xFE30, 0xFE52 },   // CJK Compatibility Forms
    { 0xFE54, 0xFE66 },   // Small Form Variants
    { 0xFE68, 0xFE6B },   // Small Form Variants
    { 0xFF00, 0xFF60 },   // Fullwidth Forms
    { 0xFFE0, 0xFFE6 },   // Fullwidth signs
    { 0x16FE0, 0x16FE4 }, // Tangut/Nushu iteration marks
    { 0x17000, 0x187F7 }, // Tangut
    { 0x18800, 0x18CD5 }, // Tangut Components, Khitan Small Script
    { 0x1B000, 0x1B2FB }, // Kana Supplement/Extended
    { 0x1F004, 0x1F004 }, // mahjong red dragon
    { 0x1F0CF, 0x1F0CF }, // joker
    { 0x1F18E, 0x1F18E }, // AB button
    { 0x1F191, 0x1F19A }, // squared CL .. squared VS
    { 0x1F200, 0x1F202 }, // squared katakana
    { 0x1F210, 0x1F23B }, // squared CJK ideographs
    { 0x1F240, 0x1F248 }, // tortoise-shell bracketed ideographs
    { 0x1F250, 0x1F251 }, // circled ideographs
    { 0x1F260, 0x1F265 }, // rounded symbols
    { 0x1F300, 0x1F64F }, // Misc Symbols and Pictographs, Emoticons
    { 0x1F680, 0x1F6FC }, // Transport and Map Symbols
    { 0x1F7E0, 0x1F7EB }, // large colored circles/squares
    { 0x1F90C, 0x1F9FF }, // Supplemental Symbols and Pictographs
    { 0x1FA70, 0x1FAFF }, // Symbols and Pictographs Extended-A
    { 0x20000, 0x2FFFD }, // CJK Extension B and beyond
    { 0x30000, 0x3FFFD }, // CJK Extension G and beyond
};

bool inWideRanges(char32_t ch)
{
    const auto *end = std::end(kWideRanges);
    // First interval whose `last` is >= ch
    const auto *it = std::lower_bound(std::begin(kWideRanges), end, ch,
        [](const Interval &iv, char32_t c) { return iv.last < c; });
    return it != end && ch >= it->first;
}

} // namespace

int charWidth(char32_t ch)
{
    if (ch == 0)
        return 0; // NUL doubles as the ScreenBuffer continuation marker

    // Control characters have no defined width; render defensively as 1
    // column so buffer arithmetic stays sane (they should never be written).
    if (ch < 0x20 || (ch >= 0x7F && ch < 0xA0))
        return 1;

    if (ch == 0x00AD)
        return 1; // soft hyphen: category Cf but width 1 per wcwidth

    // Zero width: combining marks (Mn/Me), format characters (Cf: ZWJ, ZWSP,
    // variation selectors are Mn), and Hangul jamo medial vowels/final
    // consonants which fuse into the leading consonant's cell.
    if (ch >= 0x1160 && ch <= 0x11FF)
        return 0;
    if (ch == 0x200B)
        return 0; // zero width space (category Zs in some versions)
    switch (QChar::category(ch)) {
    case QChar::Mark_NonSpacing:   // Mn
    case QChar::Mark_Enclosing:    // Me
    case QChar::Other_Format:      // Cf (ZWJ, ZWNJ, LRM, ...)
        return 0;
    default:
        break;
    }

    return inWideRanges(ch) ? 2 : 1;
}

bool isZeroWidth(char32_t ch)
{
    return ch != 0 && charWidth(ch) == 0;
}

int stringWidth(const QString &text)
{
    int cols = 0;
    int i = 0;
    Cluster c;
    while (nextCluster(text, i, c))
        cols += c.width;
    return cols;
}

bool nextCluster(const QString &text, int &index, Cluster &out)
{
    const int size = text.size();
    if (index < 0 || index >= size)
        return false;

    out.index = index;
    out.combining.clear();

    // Decode the base code point (surrogate-aware)
    char32_t base = text.at(index).unicode();
    int i = index + 1;
    if (QChar::isHighSurrogate(base) && i < size && text.at(i).isLowSurrogate()) {
        base = QChar::surrogateToUcs4(static_cast<char16_t>(base), text.at(i).unicode());
        ++i;
    }
    out.base = base;
    out.width = std::max(1, charWidth(base)); // stray zero-width base degrades to 1 column

    // Absorb trailing zero-width code points (combining marks, VS, ZWJ)
    while (i < size) {
        char32_t cp = text.at(i).unicode();
        int cpLen = 1;
        if (QChar::isHighSurrogate(cp) && i + 1 < size && text.at(i + 1).isLowSurrogate()) {
            cp = QChar::surrogateToUcs4(static_cast<char16_t>(cp), text.at(i + 1).unicode());
            cpLen = 2;
        }
        if (!isZeroWidth(cp))
            break;
        out.combining.append(text.mid(i, cpLen));
        i += cpLen;
    }

    out.length = i - index;
    index = i;
    return true;
}

int columnOfIndex(const QString &text, int index)
{
    int col = 0;
    int i = 0;
    Cluster c;
    while (nextCluster(text, i, c)) {
        if (c.index + c.length > index)
            break; // index at or inside this cluster: column of its start
        col += c.width;
    }
    return col;
}

int indexOfColumn(const QString &text, int column)
{
    int col = 0;
    int i = 0;
    Cluster c;
    while (nextCluster(text, i, c)) {
        if (col + c.width > column)
            return c.index;
        col += c.width;
    }
    return text.size();
}

int nextClusterIndex(const QString &text, int index)
{
    int i = 0;
    Cluster c;
    while (nextCluster(text, i, c)) {
        if (c.index + c.length > index)
            return c.index + c.length;
    }
    return text.size();
}

int prevClusterIndex(const QString &text, int index)
{
    int prev = 0;
    int i = 0;
    Cluster c;
    while (nextCluster(text, i, c)) {
        if (c.index >= index)
            break;
        prev = c.index;
    }
    return prev;
}

} // namespace QtTuiUnicode
} // namespace QtTUI
