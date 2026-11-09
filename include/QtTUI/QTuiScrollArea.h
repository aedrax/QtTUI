#ifndef QTTUI_QTUISCROLLAREA_H
#define QTTUI_QTUISCROLLAREA_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractScrollArea.h>

namespace QtTUI {

class QTuiScrollBar;

class QTuiScrollArea : public QTuiAbstractScrollArea {
    Q_OBJECT
public:
    explicit QTuiScrollArea(QTuiWidget *parent = nullptr);

    void setWidget(QTuiWidget *widget);
    QTuiWidget *widget() const;

    QTuiScrollBar *verticalScrollBar() const;
    QTuiScrollBar *horizontalScrollBar() const;

    void ensureVisible(int x, int y);
    // Scrolls so that a widget in the content is visible, as in Qt
    void ensureWidgetVisible(QTuiWidget *child);

    QRect contentArea() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;
    QRect hoverRect() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    // A click in the viewport goes to the content widget under it, in
    // content coordinates; the wheel scrolls
    void mousePressEvent(TuiMouseEvent *event) override;
    void resizeEvent(TuiResizeEvent *event) override;
    // Tab to a widget in the content scrolls to it
    void focusEntered(QTuiWidget *focus) override;
    // Paints the visible part of the content into viewport (in buffer
    // coordinates). paintEvent() calls it if there is a visible content
    // widget. The default paints all the content into a second buffer and
    // copies the visible part. A subclass with large content paints only
    // the visible part; scrollOffset() and m_scrollX give the first row
    // and the first column that show.
    virtual void paintContent(ScreenBuffer &buffer, const QRect &viewport);

    // Shows or hides the scroll bars and sets their ranges for the size
    // of the content. paintEvent() calls it; call it after a change of
    // the content size if the scroll position must be correct before the
    // next paint.
    void updateScrollBars();

    int m_scrollX = 0;

private:
    // A screen position in content coordinates
    QPoint contentPos(const QPoint &pos) const;
    // The deepest visible content widget at a content position, or null
    QTuiWidget *contentWidgetAt(const QPoint &content) const;

    QTuiWidget *m_widget = nullptr;
    QTuiScrollBar *m_verticalScrollBar;
    QTuiScrollBar *m_horizontalScrollBar;
};

} // namespace QtTUI

#endif // QTTUI_QTUISCROLLAREA_H
