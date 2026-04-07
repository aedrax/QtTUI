#ifndef QTTUI_QTUIMDIAREA_H
#define QTTUI_QTUIMDIAREA_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QList>

namespace QtTUI {

class QTuiMdiSubWindow;

class QTuiMdiArea : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiMdiArea(QTuiWidget *parent = nullptr);

    QTuiMdiSubWindow *addSubWindow(QTuiWidget *widget);
    void removeSubWindow(QTuiWidget *widget);

    QList<QTuiMdiSubWindow *> subWindowList() const;
    QTuiMdiSubWindow *activeSubWindow() const;
    void setActiveSubWindow(QTuiMdiSubWindow *window);

    void cascadeSubWindows();
    void tileSubWindows();

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void subWindowActivated(QTuiMdiSubWindow *window);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QList<QTuiMdiSubWindow *> m_subWindows;
    QTuiMdiSubWindow *m_activeWindow = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMDIAREA_H
