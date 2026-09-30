#ifndef QTTUI_QTUIWIDGET_H
#define QTTUI_QTUIWIDGET_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiSizePolicy.h>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QList>

namespace QtTUI {

struct ThemeProperties;
class QTuiAction;
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
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip)
    Q_PROPERTY(QString accessibleName READ accessibleName WRITE setAccessibleName)
    Q_PROPERTY(QString accessibleDescription READ accessibleDescription WRITE setAccessibleDescription)
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

    // Names for assistive technology, as in QWidget. QtTUI stores them for
    // the application; it has no screen reader interface of its own.
    QString accessibleName() const;
    void setAccessibleName(const QString &name);
    QString accessibleDescription() const;
    void setAccessibleDescription(const QString &description);

    // True while the mouse is over the widget (the main window sets it)
    bool underMouse() const;
    // The mouse position in screen coordinates, while underMouse() is true
    QPoint hoverPos() const;
    // The area that the hover highlight covers, in screen coordinates. The
    // default is the whole widget. A view gives only the row under the
    // mouse, and an empty rectangle if no item is there.
    virtual QRect hoverRect() const;

    // Layout direction, as in QWidget. A widget without its own direction
    // uses its parent's, and a top-level widget uses
    // QTuiApplication::layoutDirection(). In a right-to-left widget, box,
    // grid and form layouts mirror horizontally. There is no bidirectional
    // text reordering: text in a cell shows in its stored order.
    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection direction);
    void unsetLayoutDirection();
    bool isRightToLeft() const;

    // Context menu, as in QWidget. A right click on the widget, or
    // Shift+F10 while it has focus, asks for a context menu:
    // - DefaultContextMenu (default): the parent widget answers instead
    // - CustomContextMenu: emits customContextMenuRequested()
    // - ActionsContextMenu: shows a menu with actions()
    // - NoContextMenu: no menu, and the parent is not asked
    Qt::ContextMenuPolicy contextMenuPolicy() const;
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);

    // Actions of the widget, for ActionsContextMenu. The widget does not
    // take ownership.
    void addAction(QTuiAction *action);
    void removeAction(QTuiAction *action);
    QList<QTuiAction *> actions() const;

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

    // The size the widget prefers; a box layout gives it this size if
    // there is space. The default is minimumSize().
    virtual QSize sizeHint() const;

    // How a box layout sizes the widget. On the axis of the layout, a Fixed
    // widget never grows and an Expanding widget without a stretch factor
    // takes free space. On the other axis, a Fixed or Maximum widget gets
    // its size hint; all others fill the space. Default: Preferred.
    QTuiSizePolicy sizePolicy() const;
    void setSizePolicy(const QTuiSizePolicy &policy);
    void setSizePolicy(QTuiSizePolicy::Policy horizontal, QTuiSizePolicy::Policy vertical);

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
    // pos is in cells, relative to the top-left corner of the widget
    void customContextMenuRequested(const QPoint &pos);
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
    friend class QTuiMainWindow;
    void setUnderMouse(bool under);
    void setHoverPos(const QPoint &pos);

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
    QString m_accessibleName;
    QString m_accessibleDescription;
    QTuiSizePolicy m_sizePolicy;
    Qt::ContextMenuPolicy m_contextMenuPolicy = Qt::DefaultContextMenu;
    bool m_underMouse = false;
    QPoint m_hoverPos;
    Qt::LayoutDirection m_layoutDirection = Qt::LayoutDirectionAuto; // Auto: inherit
    QList<QPointer<QTuiAction>> m_actions;
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
