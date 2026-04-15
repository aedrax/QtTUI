#ifndef QTTUI_RENDERSCHEDULER_H
#define QTTUI_RENDERSCHEDULER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRect>

namespace QtTUI {

class QTuiApplication;
class QTuiWidget;

class RenderScheduler : public QObject {
    Q_OBJECT
public:
    explicit RenderScheduler(QTuiApplication *app);

    void scheduleDirtyRegion(const QRect &region);
    void renderNow();
    bool hasPendingRender() const;

private slots:
    void executeRenderPass();

private:
    void paintTree(QTuiWidget *widget, const QRect &parentClipRect);

    QTuiApplication *m_app;
    bool m_pending = false;
    QRect m_dirtyUnion;
    bool m_fullRepaint = false;
};

} // namespace QtTUI

#endif // QTTUI_RENDERSCHEDULER_H
