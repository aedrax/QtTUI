#ifndef QTTUI_QTUI_FOCUSFRAME_H
#define QTTUI_QTUI_FOCUSFRAME_H

#include <QtTUI/QTuiWidget.h>

namespace QtTUI {

class QTuiFocusFrame : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiFocusFrame(QTuiWidget *parent = nullptr);

    void setWidget(QTuiWidget *widget);
    QTuiWidget *widget() const;

    void paintEvent(ScreenBuffer &buffer) override;

private slots:
    void onTargetFocusGained();
    void onTargetFocusLost();
    void onTargetGeometryChanged(const QRect &rect);

private:
    void updateFrameGeometry();
    QTuiWidget *m_target = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUI_FOCUSFRAME_H
