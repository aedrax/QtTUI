#ifndef QTTUI_QTUIUNDOVIEW_H
#define QTTUI_QTUIUNDOVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiUndoStack.h>

namespace QtTUI {

class QTuiUndoView : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiUndoView(QTuiWidget *parent = nullptr);
    explicit QTuiUndoView(QTuiUndoStack *stack, QTuiWidget *parent = nullptr);

    void setStack(QTuiUndoStack *stack);
    QTuiUndoStack *stack() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QRect hoverRect() const override;

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private slots:
    void onStackChanged();

private:
    // The "<empty>" row and one row for each command
    int rowCount() const;
    void scrollToVisible();

    QTuiUndoStack *m_stack = nullptr;
    int m_currentIndex = 0;
    int m_scrollOffset = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIUNDOVIEW_H
