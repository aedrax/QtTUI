#include <QtTUI/QTuiFrame.h>
#include <QtTUI/Theme_Engine.h>

namespace QtTUI {

QTuiFrame::QTuiFrame(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

QTuiFrame::Shape QTuiFrame::frameShape() const
{
    return m_shape;
}

void QTuiFrame::setFrameShape(Shape shape)
{
    if (m_shape == shape)
        return;
    m_shape = shape;
    markDirty();
}

int QTuiFrame::frameWidth() const
{
    return m_shape == Box ? 1 : 0;
}

QRect QTuiFrame::contentsRect() const
{
    const int w = frameWidth();
    return geometry().adjusted(w, w, -w, -w);
}

void QTuiFrame::drawFrame(ScreenBuffer &buffer) const
{
    if (m_shape == NoFrame)
        return;
    const auto theme = themeProperties();
    drawBorder(buffer, geometry(), theme.borderStyle, theme.foreground, theme.background);
}

void QTuiFrame::drawBorder(ScreenBuffer &buffer, const QRect &rect, const QString &style,
                           const Color &fg, const Color &bg)
{
    const auto border = Theme_Engine::borderChars(style);
    const int x0 = rect.x();
    const int y0 = rect.y();
    const int w = rect.width();
    const int h = rect.height();
    auto put = [&](int x, int y, char32_t ch) {
        if (x < 0 || y < 0 || x >= buffer.width() || y >= buffer.height())
            return;
        Cell &cell = buffer.backCell(x, y);
        cell.setChar(ch);
        cell.foreground = fg;
        cell.background = bg;
    };

    if (h >= 1) {
        for (int col = 0; col < w; ++col)
            put(x0 + col, y0, col == 0 ? border.topLeft
                              : col == w - 1 ? border.topRight : border.horizontal);
    }
    if (h >= 2) {
        for (int col = 0; col < w; ++col)
            put(x0 + col, y0 + h - 1, col == 0 ? border.bottomLeft
                                      : col == w - 1 ? border.bottomRight : border.horizontal);
    }
    for (int row = 1; row < h - 1; ++row) {
        put(x0, y0 + row, border.vertical);
        put(x0 + w - 1, y0 + row, border.vertical);
    }
}

} // namespace QtTUI
