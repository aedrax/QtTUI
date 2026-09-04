#ifndef QTTUI_QTUIFRAME_H
#define QTTUI_QTUIFRAME_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

// Base class of widgets that can have a frame, like QFrame. NoFrame (the
// default) draws nothing; Box draws a border with the theme border style
// around the widget, and contentsRect() is the area inside it.
class QTuiFrame : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(Shape frameShape READ frameShape WRITE setFrameShape)
public:
    enum Shape { NoFrame, Box };
    Q_ENUM(Shape)

    explicit QTuiFrame(QTuiWidget *parent = nullptr);

    Shape frameShape() const;
    void setFrameShape(Shape shape);

    // Cells of the frame on each side: 1 for Box, 0 for NoFrame
    int frameWidth() const;
    // The geometry without the frame
    QRect contentsRect() const;

protected:
    // Draws the frame of this widget if it has one
    void drawFrame(ScreenBuffer &buffer) const;

private:
    Shape m_shape = NoFrame;
};

} // namespace QtTUI

#endif // QTTUI_QTUIFRAME_H
