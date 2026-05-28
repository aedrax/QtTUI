#include <QtTUI/AnsiRenderer.h>
#include <climits>
#include <QDebug>
#include <algorithm>

namespace QtTUI {

// Maximum terminal bounds for clamping cursor positions
static constexpr int kMaxTerminalCols = 9999;
static constexpr int kMaxTerminalRows = 9999;

// True when two cells render with identical SGR attributes (character ignored)
static bool sameSgrAttributes(const Cell &a, const Cell &b)
{
    return a.foreground == b.foreground
        && a.background == b.background
        && a.bold == b.bold
        && a.italic == b.italic
        && a.underline == b.underline
        && a.strikethrough == b.strikethrough
        && a.inverse == b.inverse;
}

// Encodes one code point as UTF-8 (no QString round-trip)
static void appendUtf8(QByteArray &output, char32_t u)
{
    if (u < 0x80) {
        output.append(static_cast<char>(u));
    } else if (u < 0x800) {
        output.append(static_cast<char>(0xC0 | (u >> 6)));
        output.append(static_cast<char>(0x80 | (u & 0x3F)));
    } else if (u < 0x10000) {
        output.append(static_cast<char>(0xE0 | (u >> 12)));
        output.append(static_cast<char>(0x80 | ((u >> 6) & 0x3F)));
        output.append(static_cast<char>(0x80 | (u & 0x3F)));
    } else {
        output.append(static_cast<char>(0xF0 | (u >> 18)));
        output.append(static_cast<char>(0x80 | ((u >> 12) & 0x3F)));
        output.append(static_cast<char>(0x80 | ((u >> 6) & 0x3F)));
        output.append(static_cast<char>(0x80 | (u & 0x3F)));
    }
}

// Appends the cell's character (base + combining marks) as UTF-8, with an
// allocation-free fast path for ASCII
static void appendCellChar(QByteArray &output, const Cell &cell)
{
    const char32_t u = cell.ch;
    if (u == 0) {
        output.append(' ');
    } else if (u < 0x80 && cell.combining.isEmpty()) {
        output.append(static_cast<char>(u));
        return;
    } else {
        appendUtf8(output, u);
    }
    if (!cell.combining.isEmpty())
        output.append(cell.combining.toUtf8());
}

QByteArray AnsiRenderer::render(const ScreenBuffer &buffer)
{
    // diff() produces positions in row-major order by construction; no sort needed
    const QVector<QPoint> changed = buffer.diff();
    if (changed.isEmpty())
        return QByteArray();

    QByteArray output;
    output.reserve(changed.size() * 8); // rough estimate with SGR state tracking

    // Track the last-emitted SGR attributes across the whole frame: cursor
    // movement does not affect SGR state, so runs share it too.
    Cell lastAttrs;
    bool haveAttrs = false;

    int i = 0;
    while (i < changed.size()) {
        // Start a new run: emit cursor position for the first cell
        const int runY = changed[i].y();
        int runStartX = changed[i].x();

        output.append(cursorPosition(runStartX, runY));

        int prevX = runStartX - 1;

        // Batch adjacent cells on the same row, emitting SGR only on change
        while (i < changed.size()
               && changed[i].y() == runY
               && changed[i].x() == prevX + 1) {
            const int x = changed[i].x();
            const Cell &c = buffer.backCell(x, runY);
            if (!haveAttrs || !sameSgrAttributes(c, lastAttrs)) {
                output.append(sgrSequence(c));
                lastAttrs = c;
                haveAttrs = true;
            }

            if (c.isContinuation()) {
                // A continuation cell alone (its glyph did not precede it in
                // this run) means the pair was broken by a direct cell write;
                // emit a space so the terminal column is well-defined.
                output.append(' ');
                prevX = x;
                ++i;
                continue;
            }

            if (c.width == 2) {
                // A wide glyph needs 2 columns; in the last column that is
                // impossible, so emit a space defensively.
                if (x + 1 >= buffer.width()) {
                    output.append(' ');
                    prevX = x;
                    ++i;
                    continue;
                }
                const Cell &next = buffer.backCell(x + 1, runY);
                appendCellChar(output, c);
                if (next.isContinuation()) {
                    // The terminal advanced 2 columns by itself: consume the
                    // continuation position from the diff (pair-atomic diff
                    // guarantees it is the next entry) and skip it.
                    ++i;
                    if (i < changed.size() && changed[i].y() == runY
                        && changed[i].x() == x + 1) {
                        ++i;
                    }
                    prevX = x + 1;
                    continue;
                }
                // Broken pair (direct write of a wide char without a
                // continuation cell): the terminal cursor advanced 2 columns
                // but the model has independent content at x+1. End the run so
                // the next cell repositions explicitly and overwrites the
                // glyph's right half.
                ++i;
                break;
            }

            appendCellChar(output, c);
            prevX = x;
            ++i;
        }
    }

    // Reset attributes at the end
    output.append(resetAttributes());

    return output;
}

static AnsiRenderer::ColorMode s_colorMode = AnsiRenderer::TrueColor;

AnsiRenderer::ColorMode AnsiRenderer::colorMode()
{
    return s_colorMode;
}

void AnsiRenderer::setColorMode(ColorMode mode)
{
    s_colorMode = mode;
}

AnsiRenderer::ColorMode AnsiRenderer::detectColorMode()
{
    const QByteArray forced = qgetenv("QTTUI_COLOR").toLower();
    if (forced == "truecolor" || forced == "24bit")
        return TrueColor;
    if (forced == "256")
        return Palette256;
    if (forced == "16")
        return Palette16;
    if (forced == "none")
        return NoColor;

    // https://no-color.org: a non-empty NO_COLOR disables color
    if (!qgetenv("NO_COLOR").isEmpty())
        return NoColor;

    const QByteArray colorTerm = qgetenv("COLORTERM").toLower();
    if (colorTerm == "truecolor" || colorTerm == "24bit")
        return TrueColor;

    const QByteArray term = qgetenv("TERM");
    if (term == "dumb")
        return NoColor;
    if (term.contains("256color"))
        return Palette256;
    return Palette16;
}

// Nearest index in the xterm 256-color palette: the 6x6x6 cube (16-231)
// or the gray ramp (232-255), whichever is closer
static int nearestPalette256(const Color &c)
{
    static const int levels[6] = {0, 95, 135, 175, 215, 255};
    auto cubeIndex = [](int v) {
        return v < 48 ? 0 : v < 115 ? 1 : (v - 35) / 40;
    };
    const int ri = cubeIndex(c.r), gi = cubeIndex(c.g), bi = cubeIndex(c.b);
    const int cubeDist = (c.r - levels[ri]) * (c.r - levels[ri])
        + (c.g - levels[gi]) * (c.g - levels[gi])
        + (c.b - levels[bi]) * (c.b - levels[bi]);

    const int avg = (c.r + c.g + c.b) / 3;
    const int grayIndex = avg > 238 ? 23 : qMax(0, (avg - 3) / 10);
    const int gray = 8 + grayIndex * 10;
    const int grayDist = (c.r - gray) * (c.r - gray) + (c.g - gray) * (c.g - gray)
        + (c.b - gray) * (c.b - gray);

    if (grayDist < cubeDist)
        return 232 + grayIndex;
    return 16 + 36 * ri + 6 * gi + bi;
}

// Nearest of the 16 ANSI colors (xterm default values). Returns 0-15.
static int nearestPalette16(const Color &c)
{
    static const int ansi[16][3] = {
        {0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0},
        {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
        {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
        {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
    int best = 0;
    int bestDist = INT_MAX;
    for (int i = 0; i < 16; ++i) {
        const int dr = c.r - ansi[i][0], dg = c.g - ansi[i][1], db = c.b - ansi[i][2];
        const int dist = dr * dr + dg * dg + db * db;
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

static int luminance(const Color &c)
{
    return 299 * c.r + 587 * c.g + 114 * c.b;
}

// Appends ";38;..." (foreground) or ";48;..." (background) for the mode
static void appendColor(QByteArray &seq, const Color &c, bool background)
{
    switch (s_colorMode) {
    case AnsiRenderer::TrueColor:
        seq.append(background ? ";48;2;" : ";38;2;");
        seq.append(QByteArray::number(c.r));
        seq.append(';');
        seq.append(QByteArray::number(c.g));
        seq.append(';');
        seq.append(QByteArray::number(c.b));
        break;
    case AnsiRenderer::Palette256:
        seq.append(background ? ";48;5;" : ";38;5;");
        seq.append(QByteArray::number(nearestPalette256(c)));
        break;
    case AnsiRenderer::Palette16: {
        // 30-37 / 90-97 foreground, 40-47 / 100-107 background
        const int index = nearestPalette16(c);
        const int base = (index < 8 ? 30 : 90) + (background ? 10 : 0);
        seq.append(';');
        seq.append(QByteArray::number(base + index % 8));
        break;
    }
    case AnsiRenderer::NoColor:
        break;
    }
}

QByteArray AnsiRenderer::sgrSequence(const Cell &cell)
{
    QByteArray seq;

    // Reset first, then apply attributes
    seq.append("\x1b[0");

    // Text attributes
    if (cell.bold)
        seq.append(";1");
    if (cell.italic)
        seq.append(";3");
    if (cell.underline)
        seq.append(";4");
    if (cell.strikethrough)
        seq.append(";9");
    // Without color, a cell with a background lighter than its foreground
    // (focus, selection, a pressed button) uses reverse video instead
    const bool lightBackground = s_colorMode == NoColor
        && luminance(cell.background) > luminance(cell.foreground);
    if (cell.inverse != lightBackground)
        seq.append(";7");

    appendColor(seq, cell.foreground, false);
    appendColor(seq, cell.background, true);

    seq.append('m');

    return seq;
}

QByteArray AnsiRenderer::cursorPosition(int x, int y)
{
    // Clamp to valid range (0-based input, convert to 1-based CUP)
    int col = std::clamp(x, 0, kMaxTerminalCols) + 1;
    int row = std::clamp(y, 0, kMaxTerminalRows) + 1;

    // CUP: \e[row;colH
    QByteArray seq;
    seq.append("\x1b[");
    seq.append(QByteArray::number(row));
    seq.append(';');
    seq.append(QByteArray::number(col));
    seq.append('H');

    return seq;
}

QByteArray AnsiRenderer::cursorMove(int dx, int dy)
{
    QByteArray seq;

    // Vertical movement: CUU (up) / CUD (down)
    if (dy < 0) {
        // CUU: \e[nA (cursor up)
        seq.append("\x1b[");
        seq.append(QByteArray::number(-dy));
        seq.append('A');
    } else if (dy > 0) {
        // CUD: \e[nB (cursor down)
        seq.append("\x1b[");
        seq.append(QByteArray::number(dy));
        seq.append('B');
    }

    // Horizontal movement: CUF (forward/right) / CUB (backward/left)
    if (dx > 0) {
        // CUF: \e[nC (cursor forward)
        seq.append("\x1b[");
        seq.append(QByteArray::number(dx));
        seq.append('C');
    } else if (dx < 0) {
        // CUB: \e[nD (cursor backward)
        seq.append("\x1b[");
        seq.append(QByteArray::number(-dx));
        seq.append('D');
    }

    return seq;
}

QByteArray AnsiRenderer::resetAttributes()
{
    return QByteArray("\x1b[0m");
}

Cell AnsiRenderer::parseSgrSequence(const QByteArray &seq)
{
    Cell cell;
    // Default: white on black, no attributes (already set by Cell default constructor)

    // Validate basic structure: must start with \e[ and end with m
    if (seq.size() < 4 || seq[0] != '\x1b' || seq[1] != '[' || !seq.endsWith('m'))
        return cell;

    // Extract parameters between \e[ and m
    QByteArray params = seq.mid(2, seq.size() - 3);
    QList<QByteArray> parts = params.split(';');

    int i = 0;
    while (i < parts.size()) {
        bool ok = false;
        int code = parts[i].toInt(&ok);
        if (!ok) {
            ++i;
            continue;
        }

        switch (code) {
        case 0:
            // Reset, clear all attributes (already default)
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;
            cell.strikethrough = false;
            cell.inverse = false;
            cell.foreground = Color(255, 255, 255);
            cell.background = Color(0, 0, 0);
            break;
        case 1:
            cell.bold = true;
            break;
        case 3:
            cell.italic = true;
            break;
        case 4:
            cell.underline = true;
            break;
        case 7:
            cell.inverse = true;
            break;
        case 9:
            cell.strikethrough = true;
            break;
        case 38:
            // Foreground color
            if (i + 1 < parts.size()) {
                int colorType = parts[i + 1].toInt(&ok);
                if (ok && colorType == 2 && i + 4 < parts.size()) {
                    // 24-bit TrueColor: 38;2;R;G;B
                    bool rOk, gOk, bOk;
                    int r = parts[i + 2].toInt(&rOk);
                    int g = parts[i + 3].toInt(&gOk);
                    int b = parts[i + 4].toInt(&bOk);
                    if (rOk && gOk && bOk) {
                        cell.foreground = Color(
                            static_cast<uint8_t>(std::clamp(r, 0, 255)),
                            static_cast<uint8_t>(std::clamp(g, 0, 255)),
                            static_cast<uint8_t>(std::clamp(b, 0, 255)));
                    }
                    i += 4;
                } else if (ok && colorType == 5 && i + 2 < parts.size()) {
                    // 256-color: 38;5;N store as-is in a simplified way
                    // For round-trip, we only fully support TrueColor
                    i += 2;
                } else {
                    ++i;
                }
            }
            break;
        case 48:
            // Background color
            if (i + 1 < parts.size()) {
                int colorType = parts[i + 1].toInt(&ok);
                if (ok && colorType == 2 && i + 4 < parts.size()) {
                    // 24-bit TrueColor: 48;2;R;G;B
                    bool rOk, gOk, bOk;
                    int r = parts[i + 2].toInt(&rOk);
                    int g = parts[i + 3].toInt(&gOk);
                    int b = parts[i + 4].toInt(&bOk);
                    if (rOk && gOk && bOk) {
                        cell.background = Color(
                            static_cast<uint8_t>(std::clamp(r, 0, 255)),
                            static_cast<uint8_t>(std::clamp(g, 0, 255)),
                            static_cast<uint8_t>(std::clamp(b, 0, 255)));
                    }
                    i += 4;
                } else if (ok && colorType == 5 && i + 2 < parts.size()) {
                    // 256-color: 48;5;N
                    i += 2;
                } else {
                    ++i;
                }
            }
            break;
        default:
            break;
        }
        ++i;
    }

    return cell;
}

QPoint AnsiRenderer::parseCupSequence(const QByteArray &seq)
{
    // CUP format: \e[row;colH
    // Returns 0-based (x, y) from 1-based (row, col)

    if (seq.size() < 4 || seq[0] != '\x1b' || seq[1] != '[' || !seq.endsWith('H'))
        return QPoint(0, 0);

    QByteArray params = seq.mid(2, seq.size() - 3);
    QList<QByteArray> parts = params.split(';');

    if (parts.size() != 2)
        return QPoint(0, 0);

    bool rowOk, colOk;
    int row = parts[0].toInt(&rowOk);
    int col = parts[1].toInt(&colOk);

    if (!rowOk || !colOk || row < 1 || col < 1)
        return QPoint(0, 0);

    // Convert 1-based to 0-based
    return QPoint(col - 1, row - 1);
}

} // namespace QtTUI
