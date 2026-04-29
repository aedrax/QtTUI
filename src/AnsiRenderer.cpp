#include <QtTUI/AnsiRenderer.h>
#include <QDebug>
#include <algorithm>

namespace QtTUI {

// Maximum terminal bounds for clamping cursor positions
static constexpr int kMaxTerminalCols = 9999;
static constexpr int kMaxTerminalRows = 9999;

QByteArray AnsiRenderer::render(const ScreenBuffer &buffer)
{
    QVector<QPoint> changed = buffer.diff();
    if (changed.isEmpty())
        return QByteArray();

    // Sort row-major: by y first, then by x
    std::sort(changed.begin(), changed.end(), [](const QPoint &a, const QPoint &b) {
        return a.y() < b.y() || (a.y() == b.y() && a.x() < b.x());
    });

    QByteArray output;
    output.reserve(changed.size() * 30); // rough estimate

    int i = 0;
    while (i < changed.size()) {
        // Start a new run: emit cursor position for the first cell
        const int runY = changed[i].y();
        int runStartX = changed[i].x();

        output.append(cursorPosition(runStartX, runY));

        // Emit SGR + character for the first cell in the run
        const Cell &cell = buffer.backCell(runStartX, runY);
        output.append(sgrSequence(cell));
        if (cell.character.unicode()) {
            output.append(QString(cell.character).toUtf8());
        } else {
            output.append(' ');
        }

        int prevX = runStartX;
        ++i;

        // Batch adjacent cells on the same row
        while (i < changed.size()
               && changed[i].y() == runY
               && changed[i].x() == prevX + 1) {
            const Cell &c = buffer.backCell(changed[i].x(), runY);
            output.append(sgrSequence(c));
            if (c.character.unicode()) {
                output.append(QString(c.character).toUtf8());
            } else {
                output.append(' ');
            }
            prevX = changed[i].x();
            ++i;
        }
    }

    // Reset attributes at the end
    output.append(resetAttributes());

    return output;
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
    if (cell.inverse)
        seq.append(";7");

    // Foreground: 24-bit TrueColor \e[38;2;R;G;Bm
    seq.append(";38;2;");
    seq.append(QByteArray::number(cell.foreground.r));
    seq.append(';');
    seq.append(QByteArray::number(cell.foreground.g));
    seq.append(';');
    seq.append(QByteArray::number(cell.foreground.b));

    // Background: 24-bit TrueColor \e[48;2;R;G;Bm
    seq.append(";48;2;");
    seq.append(QByteArray::number(cell.background.r));
    seq.append(';');
    seq.append(QByteArray::number(cell.background.g));
    seq.append(';');
    seq.append(QByteArray::number(cell.background.b));

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
