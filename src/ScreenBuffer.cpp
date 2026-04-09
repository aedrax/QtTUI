#include <QtTUI/ScreenBuffer.h>
#include <QDebug>
#include <algorithm>

namespace QtTUI {

Cell ScreenBuffer::s_defaultCell;

bool Cell::operator==(const Cell &other) const
{
    return character == other.character
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

Cell &ScreenBuffer::backCell(int x, int y)
{
    if (!inBounds(x, y)) {
        qWarning() << "ScreenBuffer::backCell: out of bounds access at"
                    << x << "," << y
                    << "(buffer size" << m_width << "x" << m_height << ")";
        return s_defaultCell;
    }
    return m_back[index(x, y)];
}

const Cell &ScreenBuffer::backCell(int x, int y) const
{
    if (!inBounds(x, y)) {
        qWarning() << "ScreenBuffer::backCell: out of bounds access at"
                    << x << "," << y
                    << "(buffer size" << m_width << "x" << m_height << ")";
        return s_defaultCell;
    }
    return m_back[index(x, y)];
}

const Cell &ScreenBuffer::frontCell(int x, int y) const
{
    if (!inBounds(x, y)) {
        qWarning() << "ScreenBuffer::frontCell: out of bounds access at"
                    << x << "," << y
                    << "(buffer size" << m_width << "x" << m_height << ")";
        return s_defaultCell;
    }
    return m_front[index(x, y)];
}

QVector<QPoint> ScreenBuffer::diff() const
{
    QVector<QPoint> changed;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const int i = index(x, y);
            if (m_front[i] != m_back[i])
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
    // so diff() returns all positions (triggering a full redraw).
    Cell invalid;
    invalid.character = QChar(0xFFFF); // Sentinel: will never match a real back cell default
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
