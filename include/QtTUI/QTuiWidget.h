#ifndef QTTUI_QTUIWIDGET_H
#define QTTUI_QTUIWIDGET_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QList>

namespace QtTUI {

struct ThemeProperties;
class ScreenBuffer;
class TuiKeyEvent;
class TuiMouseEvent;
class TuiResizeEvent;

class QTuiWidget : public QObject {
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry WRITE setGeometry NOTIFY geometryChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)
public:
    explicit QTuiWidget(QTuiWidget *parent = nullptr);
    ~QTuiWidget() override;

    QRect geometry() const;
    void setGeometry(const QRect &rect);

    bool isVisible() const;
    void setVisible(bool visible);

    bool hasFocus() const;
    void setFocus();

    QTuiWidget *parentWidget() const;
    QList<QTuiWidget *> childWidgets() const;

    virtual QSize minimumSize() const;
    virtual void paintEvent(ScreenBuffer &buffer) = 0;

    // Theme integration: returns effective theme properties for this widget
    ThemeProperties themeProperties() const;

signals:
    void geometryChanged(const QRect &rect);
    void visibilityChanged(bool visible);
    void focusGained();
    void focusLost();

protected:
    bool event(QEvent *event) override;
    virtual void keyPressEvent(TuiKeyEvent *event);
    virtual void mousePressEvent(TuiMouseEvent *event);
    virtual void resizeEvent(TuiResizeEvent *event);

private:
    static void setFocusWidget(QTuiWidget *widget);
    static QTuiWidget *s_focusWidget;

    QRect m_geometry;
    bool m_visible = true;
    bool m_hasFocus = false;

    // Dirty region tracking for geometry changes
    QRect m_dirtyOld;
    QRect m_dirtyNew;
    bool m_hasDirtyRegions = false;

public:
    // Dirty region accessors (for rendering integration)
    bool hasDirtyRegions() const;
    QRect dirtyOldRegion() const;
    QRect dirtyNewRegion() const;
    void clearDirtyRegions();
};

} // namespace QtTUI

#endif // QTTUI_QTUIWIDGET_H
