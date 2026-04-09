#ifndef QTTUI_SCREENBUFFER_H
#define QTTUI_SCREENBUFFER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QChar>
#include <QPoint>
#include <QVector>

#include <cstdint>

namespace QtTUI {

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

    bool operator==(const Color &other) const
    {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const Color &other) const { return !(*this == other); }
};

struct Cell {
    QChar character = QLatin1Char(' ');
    Color foreground = Color(255, 255, 255);
    Color background = Color(0, 0, 0);
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    bool inverse = false;

    bool operator==(const Cell &other) const;
    bool operator!=(const Cell &other) const;
};

class ScreenBuffer {
public:
    explicit ScreenBuffer(int width = 1, int height = 1);

    void resize(int width, int height);
    int width() const;
    int height() const;

    Cell &backCell(int x, int y);
    const Cell &backCell(int x, int y) const;
    const Cell &frontCell(int x, int y) const;

    QVector<QPoint> diff() const;
    void swapBuffers();
    void invalidateAll();

    bool resizedDuringRender() const;
    void setResizedDuringRender(bool value);

private:
    int m_width;
    int m_height;
    QVector<Cell> m_front;
    QVector<Cell> m_back;
    bool m_resizedDuringRender = false;

    static Cell s_defaultCell;

    int index(int x, int y) const;
    bool inBounds(int x, int y) const;
};

} // namespace QtTUI

#endif // QTTUI_SCREENBUFFER_H
