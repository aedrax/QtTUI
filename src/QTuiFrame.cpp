#include <QtTUI/QTuiFrame.h>
#include <QtTUI/RenderUtils.h>
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
    drawBorder(buffer, geometry(), theme.borderStyle, cellStyle(theme.foreground, theme.background));
}

} // namespace QtTUI
