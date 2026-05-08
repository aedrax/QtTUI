#include <QtTUI/ScreenBuffer.h>
#include <QDebug>
#include <algorithm>
#include <vector>

namespace QtTUI {

Cell ScreenBuffer::s_defaultCell;

bool Cell::operator==(const Cell &other) const
{
    return ch == other.ch
        && width == other.width
        && combining == other.combining
        && foreground == other.foreground
        && background == other.background
        && bold == other.bold
        && italic == other.italic
        && underline == other.underline
        && strikethrough == other.strikethrough
        && inverse == other.inverse;
}

bool Cell::operator!=(const Cell &other) const
{
    return !(*this == other);
}

ScreenBuffer::ScreenBuffer(int width, int height)
    : m_width(std::max(1, width))
    , m_height(std::max(1, height))
    , m_front(m_width * m_height)
    , m_back(m_width * m_height)
{
}

void ScreenBuffer::resize(int width, int height)
{
    if (width < 1 || height < 1) {
        qWarning() << "ScreenBuffer::resize: clamping dimensions to minimum 1x1"
                    << "(requested" << width << "x" << height << ")";
    }
    m_width = std::max(1, width);
    m_height = std::max(1, height);
    const int size = m_width * m_height;
    m_back.fill(Cell(), size);
    // Invalidate front buffer so diff() returns all positions (full redraw)
    invalidateAll();
    m_resizedDuringRender = true;
}

bool ScreenBuffer::resizedDuringRender() const
{
    return m_resizedDuringRender;
}

void ScreenBuffer::setResizedDuringRender(bool value)
{
    m_resizedDuringRender = value;
}

int ScreenBuffer::width() const { return m_width; }
int ScreenBuffer::height() const { return m_height; }

// Shared handling for all out-of-bounds cell accesses: warn once per buffer
// (OOB paint loops would otherwise flood thousands of warnings per frame) and
// return the shared sentinel cell, reset to a default-constructed Cell each
// time so one caller's OOB write cannot leak into another caller's OOB read.
const Cell &ScreenBuffer::outOfBoundsCell(int x, int y) const
{
    if (!m_oobWarned) {
        m_oobWarned = true;
        qWarning() << "ScreenBuffer: out of bounds cell access at"
                   << x << "," << y
                   << "(buffer size" << m_width << "x" << m_height << ");"
                   << "further out-of-bounds warnings for this buffer are suppressed";
    }
    s_defaultCell = Cell();
    return s_defaultCell;
}

Cell &ScreenBuffer::backCell(int x, int y)
{
    if (!inBounds(x, y))
        return const_cast<Cell &>(outOfBoundsCell(x, y));
    return m_back[index(x, y)];
}

const Cell &ScreenBuffer::backCell(int x, int y) const
{
    if (!inBounds(x, y))
        return outOfBoundsCell(x, y);
    return m_back[index(x, y)];
}

const Cell &ScreenBuffer::frontCell(int x, int y) const
{
    if (!inBounds(x, y))
        return outOfBoundsCell(x, y);
    return m_front[index(x, y)];
}

// If the back cell at (x, y) is half of a wide pair, reset the OTHER half to
// a space (keeping its colors/attributes) so overwriting one half never
// leaves an orphaned glyph or continuation cell behind.
void ScreenBuffer::clearWidePairAt(int x, int y)
{
    Cell &c = m_back[index(x, y)];
    if (c.width == 2 && x + 1 < m_width) {
        Cell &cont = m_back[index(x + 1, y)];
        if (cont.isContinuation())
            cont.setChar(U' ');
    } else if (c.isContinuation() && x > 0) {
        Cell &glyph = m_back[index(x - 1, y)];
        if (glyph.width == 2)
            glyph.setChar(U' ');
    }
}

void ScreenBuffer::putChar(int x, int y, char32_t ch, const Cell &style)
{
    putChar(x, y, ch, QString(), style);
}

void ScreenBuffer::putChar(int x, int y, char32_t ch, const QString &combining,
                           const Cell &style)
{
    if (!inBounds(x, y)) {
        // Match backCell() semantics: warn once, write to the sentinel
        Cell &sink = const_cast<Cell &>(outOfBoundsCell(x, y));
        sink = style;
        sink.setChar(ch);
        return;
    }

    const int w = (ch == 0) ? 1 : std::max(1, QtTuiUnicode::charWidth(ch));

    clearWidePairAt(x, y);

    Cell &c = m_back[index(x, y)];
    c = style;

    if (w == 2 && x + 1 < m_width) {
        clearWidePairAt(x + 1, y); // continuation may half-overwrite another pair
        c.ch = ch;
        c.width = 2;
        c.combining = combining;
        Cell &cont = m_back[index(x + 1, y)];
        cont = style;
        cont.ch = 0;
        cont.width = 0;
        cont.combining.clear();
    } else if (w == 2) {
        // No room for a wide glyph in the last column: write a space
        c.setChar(U' ');
    } else {
        c.ch = ch;
        c.width = 1;
        c.combining = combining;
    }
}

QVector<QPoint> ScreenBuffer::diff() const
{
    QVector<QPoint> changed;
    std::vector<bool> rowChanged(m_width);
    for (int y = 0; y < m_height; ++y) {
        bool any = false;
        for (int x = 0; x < m_width; ++x) {
            const int i = index(x, y);
            const bool c = (m_front[i] != m_back[i]);
            rowChanged[x] = c;
            any = any || c;
        }
        if (!any)
            continue;

        // Pair atomicity: a wide glyph and its continuation cell change
        // together — if either half differs, emit both positions so the
        // renderer can redraw the glyph atomically.
        for (int x = 0; x + 1 < m_width; ++x) {
            if (m_back[index(x, y)].width == 2
                && (rowChanged[x] || rowChanged[x + 1])) {
                rowChanged[x] = true;
                rowChanged[x + 1] = true;
            }
        }

        for (int x = 0; x < m_width; ++x) {
            if (rowChanged[x])
                changed.append(QPoint(x, y));
        }
    }
    return changed;
}

void ScreenBuffer::swapBuffers()
{
    m_front = m_back;
}

void ScreenBuffer::invalidateAll()
{
    // Fill front with a sentinel cell that differs from any default back cell,
    // so diff() returns all positions (triggering a full redraw). U+FFFF is a
    // Unicode noncharacter and is never written by any paint path.
    Cell invalid;
    invalid.ch = 0xFFFF;
    m_front.fill(invalid, m_width * m_height);
}

int ScreenBuffer::index(int x, int y) const
{
    return y * m_width + x;
}

bool ScreenBuffer::inBounds(int x, int y) const
{
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

} // namespace QtTUI
