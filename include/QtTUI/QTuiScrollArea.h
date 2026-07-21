#ifndef QTTUI_QTUISCROLLAREA_H
#define QTTUI_QTUISCROLLAREA_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

class QTuiScrollBar;

class QTuiScrollArea : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiScrollArea(QTuiWidget *parent = nullptr);

    void setWidget(QTuiWidget *widget);
    QTuiWidget *widget() const;

    QTuiScrollBar *verticalScrollBar() const;
    QTuiScrollBar *horizontalScrollBar() const;

    void ensureVisible(int x, int y);

    QRect contentArea() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    // A click in the viewport goes to the content widget under it, in
    // content coordinates; the wheel scrolls
    void mousePressEvent(TuiMouseEvent *event) override;
    void resizeEvent(TuiResizeEvent *event) override;

private:
    void updateScrollBars();

    QTuiWidget *m_widget = nullptr;
    QTuiScrollBar *m_verticalScrollBar;
    QTuiScrollBar *m_horizontalScrollBar;
    int m_scrollX = 0;
    int m_scrollY = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUISCROLLAREA_H
