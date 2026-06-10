#ifndef QTTUI_RENDERSCHEDULER_H
#define QTTUI_RENDERSCHEDULER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRect>
#include <QVector>

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
    void onThemeChanged();

private:
    void paintTree(QTuiWidget *widget, const QRect &parentClipRect);
    void paintPopupLayer(QTuiWidget *root, const QRect &screenRect);
    void clearHiddenPopupRegions(QTuiWidget *widget, const QRect &screenRect);
    static void collectVisiblePopups(QTuiWidget *widget, QVector<QTuiWidget *> &popups);
    void markAllVisibleWidgetsDirty(QTuiWidget *widget);
    void clearVacatedRegion(QTuiWidget *parent, const QRect &region);
    bool intersectsPassRects(const QRect &rect) const;
    // True if one pass damage rect already contains all of rect
    bool intersectsPassRectsFully(const QRect &rect) const;

    QTuiApplication *m_app;
    bool m_pending = false;
    QRect m_dirtyUnion;
    // Individual damage rects for the pending pass; widgets intersecting any of
    // them are repainted even if not dirty themselves (restores content under
    // hidden/moved widgets). Collapsed to the union when the list grows large.
    QVector<QRect> m_dirtyRects;
    QVector<QRect> m_passRects;
    bool m_fullRepaint = false;
    bool m_cursorShown = false;
};

} // namespace QtTUI

#endif // QTTUI_RENDERSCHEDULER_H
