#ifndef QTTUI_QTUIMAINWINDOW_H
#define QTTUI_QTUIMAINWINDOW_H

#include <QtTUI/QTuiWidget.h>
#include <QSize>

namespace QtTUI {

class QTuiLayout;

class QTuiMainWindow : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiMainWindow(QTuiWidget *parent = nullptr);

    void setCentralWidget(QTuiWidget *widget);
    QTuiWidget *centralWidget() const;

    void setLayout(QTuiLayout *layout);
    QTuiLayout *layout() const;

    void show();

    void paintEvent(ScreenBuffer &buffer) override;

protected:
    void resizeEvent(TuiResizeEvent *event) override;
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    void relayoutChildren();
    void advanceFocus(bool forward);
    void rebuildFocusChain();

    QTuiWidget *m_centralWidget = nullptr;
    QTuiLayout *m_layout = nullptr;
    QList<QTuiWidget *> m_focusChain;
};

} // namespace QtTUI

#endif // QTTUI_QTUIMAINWINDOW_H
