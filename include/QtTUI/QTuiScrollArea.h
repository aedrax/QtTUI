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

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    // A click in the viewport goes to the content widget under it, in
    // content coordinates; the wheel scrolls
    void mousePressEvent(TuiMouseEvent *event) override;
    void resizeEvent(TuiResizeEvent *event) override;
    // Tab to a widget in the content scrolls to it
    void focusEntered(QTuiWidget *focus) override;

private:
    void updateScrollBars();

    QTuiWidget *m_widget = nullptr;
    QTuiScrollBar *m_verticalScrollBar;
    QTuiScrollBar *m_horizontalScrollBar;
    int m_scrollX = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUISCROLLAREA_H
