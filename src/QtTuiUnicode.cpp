#include <QtTUI/QtTuiUnicode.h>

#include <QChar>
#include <QVarLengthArray>
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

constexpr char32_t kZwj = 0x200D;
constexpr char32_t kVs16 = 0xFE0F; // variation selector 16: emoji presentation

bool isRegionalIndicator(char32_t ch)
{
    return ch >= 0x1F1E6 && ch <= 0x1F1FF;
}

bool isEmojiModifier(char32_t ch)
{
    return ch >= 0x1F3FB && ch <= 0x1F3FF; // skin tones
}

// True for a code point that can be the base of an emoji sequence.
// ponytail: these are the blocks that contain the Extended_Pictographic
// code points, not the exact property. A symbol of these blocks that is
// not an emoji also joins a VS16, a skin tone or a ZWJ sequence, but valid
// text has no such sequence. Use the table of emoji-data.txt if this
// matters.
bool isEmojiBase(char32_t ch)
{
    if (ch >= 0x1F000 && ch <= 0x1FAFF)
        return true;
    if (ch < 0x2190)
        return ch == 0x00A9 || ch == 0x00AE || ch == 0x203C || ch == 0x2049
            || ch == 0x2122 || ch == 0x2139;
    return ch <= 0x21FF                      // arrows
        || (ch >= 0x2300 && ch <= 0x23FF)    // miscellaneous technical
        || ch == 0x24C2
        || (ch >= 0x25A0 && ch <= 0x27BF)    // shapes, symbols, dingbats
        || (ch >= 0x2900 && ch <= 0x297F)    // supplemental arrows
        || (ch >= 0x2B00 && ch <= 0x2BFF)    // symbols and arrows
        || ch == 0x3030 || ch == 0x303D || ch == 0x3297 || ch == 0x3299;
}

// Decodes the code point at index i (surrogate-aware) and sets len to its
// number of code units
char32_t codePointAt(const QString &text, int i, int &len)
{
    char32_t cp = text.at(i).unicode();
    len = 1;
    if (QChar::isHighSurrogate(cp) && i + 1 < text.size() && text.at(i + 1).isLowSurrogate()) {
        cp = QChar::surrogateToUcs4(static_cast<char16_t>(cp), text.at(i + 1).unicode());
        len = 2;
    }
    return cp;
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

QString withoutControlChars(const QString &text)
{
    QString out = text;
    out.removeIf([](QChar c) { return c.category() == QChar::Other_Control; });
    return out;
}

int clusterWidth(char32_t base, const QString &combining)
{
    const int width = std::max(1, charWidth(base)); // a stray zero-width base uses 1 column
    if (width == 2 || combining.isEmpty() || !isEmojiBase(base))
        return width;
    // A narrow emoji base: VS16 asks for the emoji presentation, and a
    // joined code point with a width (a second regional indicator, a skin
    // tone, an emoji after a ZWJ) makes an emoji sequence. Both are wide.
    for (int i = 0, len = 0; i < combining.size(); i += len) {
        const char32_t cp = codePointAt(combining, i, len);
        if (cp == kVs16 || !isZeroWidth(cp))
            return 2;
    }
    return width;
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
    int cpLen = 0;
    const char32_t base = codePointAt(text, index, cpLen);
    int i = index + cpLen;
    out.base = base;

    const bool emoji = isEmojiBase(base);
    // A flag is two regional indicators. A third one starts a new cluster.
    bool flagOpen = isRegionalIndicator(base);

    // Absorb the code points that join the base
    while (i < size) {
        const char32_t cp = codePointAt(text, i, cpLen);
        int joinLen = 0;
        if (cp == kZwj && emoji && i + cpLen < size) {
            // An emoji ZWJ sequence: the ZWJ joins the next emoji
            int nextLen = 0;
            if (isEmojiBase(codePointAt(text, i + cpLen, nextLen)))
                joinLen = cpLen + nextLen;
        }
        if (joinLen == 0) {
            if (isZeroWidth(cp)) {
                joinLen = cpLen; // combining marks, variation selectors, a ZWJ at the end
            } else if (emoji && isEmojiModifier(cp)) {
                joinLen = cpLen; // skin tone
            } else if (flagOpen && isRegionalIndicator(cp)) {
                joinLen = cpLen;
            } else {
                break;
            }
        }
        flagOpen = false;
        out.combining.append(QStringView(text).mid(i, joinLen));
        i += joinLen;
    }

    out.width = clusterWidth(base, out.combining);
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

bool hasRightToLeft(const QString &text)
{
    for (const QChar ch : text) {
        const char16_t u = ch.unicode();
        if (u < 0x0590)
            continue; // Latin, Greek, Cyrillic: the usual text stops here
        if (ch.isHighSurrogate()) {
            // U+10800 to U+10FFF and U+1E800 to U+1EFFF are right-to-left
            if ((u & 0xFFFE) == 0xD802 || (u & 0xFFFE) == 0xD83A)
                return true;
            continue;
        }
        switch (ch.direction()) {
        case QChar::DirR:
        case QChar::DirAL:
        case QChar::DirAN: // two Arabic numbers with a space between them change places
            return true;
        default:
            break;
        }
    }
    return false;
}

namespace {

// One character of the bidi algorithm: the base of a cluster, or a strong
// character (LRM, RLM, ALM) in the joined code points of a cluster
struct BidiItem {
    QChar::Direction type;
    int cluster; // index in the cluster list, or -1 for a joined code point
    int level = 0;
};

bool isStrong(QChar::Direction t)
{
    return t == QChar::DirL || t == QChar::DirR || t == QChar::DirAL;
}

bool isNeutral(QChar::Direction t)
{
    return t == QChar::DirB || t == QChar::DirS || t == QChar::DirWS || t == QChar::DirON;
}

// The direction of a type for the rules N0 to N2: a number is R. Call it
// only after the W rules, when a type that is not neutral is L, R, EN or AN.
QChar::Direction strongSide(QChar::Direction t)
{
    return t == QChar::DirL ? QChar::DirL : QChar::DirR;
}

} // namespace

QList<Cluster> visualClusters(const QString &line, Qt::LayoutDirection base)
{
    using Dir = QChar::Direction;

    QList<Cluster> clusters;
    QVarLengthArray<BidiItem, 128> items;
    {
        int i = 0;
        Cluster c;
        while (nextCluster(line, i, c)) {
            Dir type = QChar::direction(c.base);
            switch (type) {
            case QChar::DirL: case QChar::DirR: case QChar::DirAL:
            case QChar::DirEN: case QChar::DirES: case QChar::DirET:
            case QChar::DirAN: case QChar::DirCS: case QChar::DirNSM:
            case QChar::DirB: case QChar::DirS: case QChar::DirWS:
                break;
            default:
                // ponytail: no explicit levels (X1 to X8). An embedding,
                // override or isolate control is a neutral here, or it is in
                // the joined code points of a cluster and has no effect.
                // Add the X rules if text with these controls must show
                // right.
                type = QChar::DirON;
            }
            items.append({ type, int(clusters.size()) });
            // The joined code points have no type of their own (W1), but a
            // strong one (LRM, RLM, ALM) has an effect on the text near it
            for (int k = 0, len = 0; k < c.combining.size(); k += len) {
                const Dir joined = QChar::direction(codePointAt(c.combining, k, len));
                if (isStrong(joined))
                    items.append({ joined, -1 });
            }
            clusters.append(c);
        }
    }
    const int n = int(items.size());

    // P2, P3: the paragraph level is the direction of the first strong character
    int paragraphLevel = base == Qt::RightToLeft ? 1 : 0;
    if (base == Qt::LayoutDirectionAuto) {
        for (const BidiItem &item : items) {
            if (isStrong(item.type)) {
                paragraphLevel = item.type == QChar::DirL ? 0 : 1;
                break;
            }
        }
    }
    // The embedding direction. Without explicit levels the line is one run,
    // and this is also the type of the start and of the end of the run.
    const Dir e = paragraphLevel ? QChar::DirR : QChar::DirL;

    // W1: a nonspacing mark gets the type of the character before it
    // W2: a European number after an Arabic letter is an Arabic number
    // W3: an Arabic letter is R
    Dir previous = e;
    Dir strong = e;
    for (BidiItem &item : items) {
        if (item.type == QChar::DirNSM)
            item.type = previous;
        previous = item.type;
        if (item.type == QChar::DirEN && strong == QChar::DirAL) {
            item.type = QChar::DirAN;
        } else if (isStrong(item.type)) {
            strong = item.type;
            if (item.type == QChar::DirAL)
                item.type = QChar::DirR;
        }
    }
    // W4: one separator between two numbers of the same type is a number
    for (int i = 1; i + 1 < n; ++i) {
        const Dir before = items[i - 1].type;
        if (before != items[i + 1].type)
            continue;
        if ((items[i].type == QChar::DirES && before == QChar::DirEN)
            || (items[i].type == QChar::DirCS && (before == QChar::DirEN || before == QChar::DirAN)))
            items[i].type = before;
    }
    // W5: number terminators next to a European number are a European number
    for (int i = 1; i < n; ++i) {
        if (items[i].type == QChar::DirET && items[i - 1].type == QChar::DirEN)
            items[i].type = QChar::DirEN;
    }
    for (int i = n - 2; i >= 0; --i) {
        if (items[i].type == QChar::DirET && items[i + 1].type == QChar::DirEN)
            items[i].type = QChar::DirEN;
    }
    // W6: the other separators and terminators are neutral
    // W7: a European number after L is L
    strong = e;
    for (BidiItem &item : items) {
        if (item.type == QChar::DirES || item.type == QChar::DirET || item.type == QChar::DirCS)
            item.type = QChar::DirON;
        else if (item.type == QChar::DirL || item.type == QChar::DirR)
            strong = item.type;
        else if (item.type == QChar::DirEN && strong == QChar::DirL)
            item.type = QChar::DirL;
    }

    // N0: the two brackets of a pair get the same direction
    {
        // ponytail: a bracket is a Ps or Pe character with a mirror
        // character, not the exact Bidi_Paired_Bracket property, and
        // U+2329/U+232A do not match U+3008/U+3009. Use BidiBrackets.txt if
        // this matters.
        struct Pair { int open; int close; };
        QVarLengthArray<Pair, 16> pairs;
        QVarLengthArray<int, 63> stack; // positions of the open brackets
        for (int i = 0; i < n; ++i) {
            if (items[i].cluster < 0 || items[i].type != QChar::DirON)
                continue;
            const char32_t ch = clusters.at(items[i].cluster).base;
            const QChar::Category category = QChar::category(ch);
            if (category == QChar::Punctuation_Open && QChar::mirroredChar(ch) != ch) {
                if (stack.size() == 63)
                    break; // BD16: stop at the stack limit
                stack.append(i);
            } else if (category == QChar::Punctuation_Close) {
                for (int k = int(stack.size()) - 1; k >= 0; --k) {
                    if (clusters.at(items[stack[k]].cluster).base == QChar::mirroredChar(ch)) {
                        pairs.append({ stack[k], i });
                        stack.resize(k);
                        break;
                    }
                }
            }
        }
        std::sort(pairs.begin(), pairs.end(),
                  [](const Pair &a, const Pair &b) { return a.open < b.open; });
        for (const Pair &pair : pairs) {
            bool found = false;
            Dir inside = e;
            for (int i = pair.open + 1; i < pair.close; ++i) {
                if (isNeutral(items[i].type))
                    continue;
                found = true;
                inside = strongSide(items[i].type);
                if (inside == e)
                    break;
            }
            if (!found)
                continue; // only neutrals in the pair: N1 and N2 resolve it
            if (inside != e) {
                // Only the opposite direction is in the pair. The pair gets
                // it if the text before the pair also has it.
                Dir context = e;
                for (int i = pair.open - 1; i >= 0; --i) {
                    if (!isNeutral(items[i].type)) {
                        context = strongSide(items[i].type);
                        break;
                    }
                }
                if (context != inside)
                    inside = e;
            }
            items[pair.open].type = items[pair.close].type = inside;
        }
    }

    // N1: neutrals between two characters of the same direction get that
    // direction. N2: the other neutrals get the embedding direction.
    for (int i = 0; i < n;) {
        if (!isNeutral(items[i].type)) {
            ++i;
            continue;
        }
        int end = i;
        while (end < n && isNeutral(items[end].type))
            ++end;
        const Dir before = i > 0 ? strongSide(items[i - 1].type) : e;
        const Dir after = end < n ? strongSide(items[end].type) : e;
        for (; i < end; ++i)
            items[i].type = before == after ? before : e;
    }

    // I1, I2: the level of each character. The types are now L, R, EN and AN.
    int highest = paragraphLevel;
    for (BidiItem &item : items) {
        if (paragraphLevel == 0)
            item.level = item.type == QChar::DirL ? 0 : item.type == QChar::DirR ? 1 : 2;
        else
            item.level = item.type == QChar::DirR ? 1 : 2;
        highest = qMax(highest, item.level);
    }

    // L2: from the highest level down to level 1, reverse each run of
    // characters at that level or higher
    for (int level = highest; level >= 1; --level) {
        for (int i = 0; i < n;) {
            if (items[i].level < level) {
                ++i;
                continue;
            }
            int end = i;
            while (end < n && items[end].level >= level)
                ++end;
            std::reverse(items.begin() + i, items.begin() + end);
            i = end;
        }
    }

    // L4: a mirrored character at an odd level shows as its mirror character
    QList<Cluster> visual;
    visual.reserve(clusters.size());
    for (const BidiItem &item : items) {
        if (item.cluster < 0)
            continue;
        Cluster c = clusters.at(item.cluster);
        if (item.level & 1)
            c.base = QChar::mirroredChar(c.base);
        visual.append(c);
    }
    return visual;
}

QString visualOrder(const QString &line, Qt::LayoutDirection base)
{
    if (base != Qt::RightToLeft && !hasRightToLeft(line))
        return line;
    QString out;
    out.reserve(line.size());
    for (const Cluster &c : visualClusters(line, base)) {
        out.append(QChar::fromUcs4(c.base));
        out.append(c.combining);
    }
    return out;
}

} // namespace QtTuiUnicode
} // namespace QtTUI
