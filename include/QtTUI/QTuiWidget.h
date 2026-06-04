#ifndef QTTUI_QTUIWIDGET_H
#define QTTUI_QTUIWIDGET_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QList>

namespace QtTUI {

struct ThemeProperties;
class QTuiLayout;
class ScreenBuffer;
class TuiKeyEvent;
class TuiMouseEvent;
class TuiResizeEvent;
class TuiPasteEvent;

class QTuiWidget : public QObject {
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry WRITE setGeometry NOTIFY geometryChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibilityChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(Qt::FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy)
public:
    explicit QTuiWidget(QTuiWidget *parent = nullptr);
    ~QTuiWidget() override;

    QRect geometry() const;
    void setGeometry(const QRect &rect);

    bool isVisible() const;
    void setVisible(bool visible);

    // Enabled state. isEnabled() is this widget's own flag;
    // isEffectivelyEnabled() also requires every ancestor to be enabled
    // (a disabled parent disables its whole subtree, like QWidget). The base
    // event() drops key/mouse events for effectively-disabled widgets, and
    // disabling a widget that contains the focus clears the focus.
    bool isEnabled() const;
    void setEnabled(bool enabled);
    bool isEffectivelyEnabled() const;

    // Focus policy (Qt::NoFocus by default; interactive widgets set
    // Qt::StrongFocus in their constructors). Governs focus acquisition:
    // Tab navigation requires the TabFocus bit, mouse-press focus requires
    // the ClickFocus bit, and setFocus() is a no-op for NoFocus widgets.
    Qt::FocusPolicy focusPolicy() const;
    void setFocusPolicy(Qt::FocusPolicy policy);

    // True for widgets that consume printable keys as text (line/text edits,
    // spin boxes, editable combos). Bare printable-key shortcuts ("Q") must
    // not fire while such a widget has focus (see QTuiMainWindow).
    virtual bool acceptsTextInput() const;

    bool hasFocus() const;
    void setFocus();
    static QTuiWidget *focusWidget();

    // Popup layer: a popup widget is skipped by the normal paint recursion and
    // painted by the RenderScheduler in a second, topmost pass clipped only to
    // the screen — not to its parent widget. Menus and dropdown lists use this
    // to escape their parent's rect. Showing a popup records the previously
    // focused widget; hiding it restores that focus (see setVisible()).
    bool isPopup() const;
    void setPopup(bool popup);

    // A widget painted by its parent keeps its geometry in the parent's own
    // coordinate space (for example the content of a QTuiScrollArea, which is
    // relative to the scroll area). The render pass and the mouse hit test
    // skip it, because its geometry is not a screen position.
    bool isPaintedByParent() const;
    void setPaintedByParent(bool paintedByParent);

    // Text that QTuiToolTip shows when the widget gets focus
    QString toolTip() const;
    void setToolTip(const QString &toolTip);

    // Called on each visible popup when a mouse press lands outside every
    // visible popup (see QTuiMainWindow::mousePressEvent). The default
    // implementation hides the popup; subclasses override it to run their full
    // close logic (signals, owner state sync).
    virtual void popupOutsideClick();

    // Returns true if this widget is child, or an ancestor of child through
    // the parentWidget() chain (mirrors QWidget::isAncestorOf).
    bool isAncestorOf(const QTuiWidget *child) const;

    // Terminal cursor position (absolute screen coords) for focused editable
    // widgets; (-1, -1) means the hardware cursor stays hidden.
    virtual QPoint terminalCursorPos() const;

    QTuiWidget *parentWidget() const;
    QList<QTuiWidget *> childWidgets() const;

    // The default minimum size is the layout's minimum size, or 0x0 if the
    // widget has no layout.
    virtual QSize minimumSize() const;

    // The default paintEvent() clears the background, so a plain QTuiWidget
    // works as a container. The render pass paints the children.
    virtual void paintEvent(ScreenBuffer &buffer);

    // The widget takes ownership of the layout and applies it to its
    // geometry each time the geometry changes. QTuiGroupBox and
    // QTuiMainWindow apply the layout to their content area instead.
    virtual void setLayout(QTuiLayout *layout);
    virtual QTuiLayout *layout() const;

    // Clears the widget's geometry with theme background color and space character.
    // Should be called at the start of paintEvent() implementations.
    void clearBackground(ScreenBuffer &buffer, const QRect &clipRect);

    // Theme integration: returns effective theme properties for this widget
    ThemeProperties themeProperties() const;

    // Dirty region API: marks regions for repaint and notifies the scheduler
    void markDirty();
    void markDirty(const QRect &region);

signals:
    void geometryChanged(const QRect &rect);
    void visibilityChanged(bool visible);
    void enabledChanged(bool enabled);
    void focusGained();
    void focusLost();

protected:
    bool event(QEvent *event) override;
    virtual void keyPressEvent(TuiKeyEvent *event);
    virtual void mousePressEvent(TuiMouseEvent *event);
    virtual void resizeEvent(TuiResizeEvent *event);
    // Pasted text for the focus widget. The default ignores it.
    virtual void pasteEvent(TuiPasteEvent *event);

private:
    static void setFocusWidget(QTuiWidget *widget);
    static QTuiWidget *s_focusWidget;

    QRect m_geometry;
    bool m_visible = true;
    bool m_enabled = true;
    bool m_hasFocus = false;
    bool m_isPopup = false;
    bool m_paintedByParent = false;
    QPointer<QTuiLayout> m_layout;
    QString m_toolTip;
    Qt::FocusPolicy m_focusPolicy = Qt::NoFocus;

    // Widget focused before this popup became visible; restored when it hides
    QPointer<QTuiWidget> m_focusBeforePopup;

    // Dirty region tracking for geometry changes
    QRect m_dirtyOld;
    QRect m_dirtyNew;
    bool m_hasDirtyRegions = false;

    // Accumulated dirty region for scheduler integration
    QRect m_dirtyRegion;
    bool m_isDirty = false;

public:
    // Dirty region accessors (for rendering integration)
    bool hasDirtyRegions() const;
    bool isDirty() const;
    QRect dirtyOldRegion() const;
    QRect dirtyNewRegion() const;
    QRect dirtyRegion() const;
    void clearDirtyRegions();
};

} // namespace QtTUI

#endif // QTTUI_QTUIWIDGET_H
