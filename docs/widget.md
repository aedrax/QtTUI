# Widget basics

This page documents `QTuiWidget`, the base class of all QtTUI widgets, and
tells how to write a custom widget. It also documents `QTuiClipboard`.

## QTuiWidget

`#include <QtTUI/QTuiWidget.h>` · Inherits: `QObject`

`QTuiWidget` is the base class of all widgets. It has a geometry in
character cells, a visible flag, an enabled flag, a focus policy, a
tooltip and a context menu policy. It is a `QObject`, not a `QPaintDevice`:
a widget paints cells into a `ScreenBuffer` in `paintEvent()`.

These are the important differences from `QWidget`:

- The geometry is in screen cells, and it is **absolute**: it is not
  relative to the parent widget. The layouts give the children absolute
  geometries.
- `QTuiWidget` has no `show()`, `hide()` or `update()`. Use `setVisible()`
  and `markDirty()`. A new widget is visible. `QTuiMainWindow` has
  `show()`, and `QTuiDialog` has `open()` and `exec()`.
- Only the widgets in the tree of the main window get painted and get
  events. See [Main window](main-window.md).
- A key that the focus widget does not use does not go to the parent
  widget.

A plain `QTuiWidget` with a layout works as a container. Its default
`paintEvent()` clears the background, and the render pass paints the
children.

### Example

This custom widget shows a number. The Up and Down keys, the mouse wheel
and a left click change the number. The class has no `Q_OBJECT`, so it
needs no moc. It cannot have its own signals; use a callback or a child
`QObject` for that.

```cpp
#include <QtTUI/QtTUI>
#include <QtTUI/RenderUtils.h>

#include <functional>

using namespace QtTUI;

class Counter : public QTuiWidget
{
public:
    explicit Counter(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
    {
        setFocusPolicy(Qt::StrongFocus);
        setToolTip(QStringLiteral("Up and Down change the number"));
    }

    int value() const { return m_value; }
    void setValue(int value)
    {
        if (m_value == value)
            return;
        m_value = value;
        markDirty(); // the look changed: paint again
        if (onChanged)
            onChanged(m_value);
    }

    std::function<void(int)> onChanged;

    QSize sizeHint() const override { return QSize(12, 1); }

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        if (!isVisible() || geo.isEmpty())
            return;
        clearBackground(buffer, geo);

        const ThemeProperties theme = themeProperties();
        Color fg = theme.foreground;
        Color bg = theme.background;
        if (!isEffectivelyEnabled()) {
            fg = theme.disabledForeground;
            bg = theme.disabledBackground;
        } else if (hasFocus()) {
            fg = theme.focusForeground;
            bg = theme.focusBackground;
        }
        const Cell style = cellStyle(theme, fg, bg);
        fillRect(buffer, geo, style);
        renderTextTruncated(buffer, geo.x(), geo.y(),
                            QStringLiteral("[ %1 ]").arg(m_value), geo.width(), style);
    }

protected:
    void keyPressEvent(TuiKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Up)
            setValue(m_value + 1);
        else if (event->key() == Qt::Key_Down)
            setValue(m_value - 1);
    }

    void mousePressEvent(TuiMouseEvent *event) override
    {
        if (event->mouseType() == TuiMouseEvent::Press && event->button() == Qt::LeftButton)
            setValue(m_value + 1);
        else if (event->mouseType() == TuiMouseEvent::Wheel)
            setValue(m_value + (event->wheelDelta() > 0 ? 1 : -1));
    }

private:
    int m_value = 0;
};

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *layout = createVBoxLayout(&window);
    auto *counter = new Counter(&window);
    auto *label = new QTuiLabel(QStringLiteral("Value: 0"), &window);
    layout->addWidget(counter);
    layout->addWidget(label);
    layout->addStretch(1);
    window.setLayout(layout);

    counter->onChanged = [label](int value) {
        label->setText(QStringLiteral("Value: %1").arg(value));
    };

    counter->setFocus();
    window.show();
    return app.exec();
}
```

A widget class without `Q_OBJECT` uses the meta-object of `QTuiWidget`.
Thus `Theme_Engine` gives it the theme properties of `QTuiWidget`. Add
`Q_OBJECT` (and moc) if the class needs its own theme entry, signals or
`qobject_cast`.

### Geometry and size

`setGeometry()` sets the position and the size in screen cells. A negative
width or height becomes 0, with a warning. If the geometry changes, the
widget marks the old and the new area dirty, and it emits
`geometryChanged()`. A widget with a layout applies the layout to its new
geometry.

`minimumSize()`, `sizeHint()` and `sizePolicy()` tell a layout how to size
the widget. For the rules, see [Layouts](layouts.md).

### Visible and enabled

`setVisible(false)` hides the widget and its children, and marks its area
dirty. If the focus is in the widget, no widget has the focus after it.
`isVisible()` returns only the flag of the widget, not of its parents.

`setEnabled(false)` disables the widget and all its children.
`isEnabled()` returns the flag of the widget. `isEffectivelyEnabled()`
returns `true` only if the widget and all its parents are enabled. A widget
that is not effectively enabled gets no key, mouse or paste events. If the
focus is in a widget that you disable, the focus goes away. A custom widget
must use `isEffectivelyEnabled()` in `paintEvent()` to show the disabled
colors.

### Focus

There is one focus widget for the application. `focusWidget()` returns it.
Key events and paste events go to the focus widget.

The focus policy controls how a widget gets the focus. The default is
`Qt::NoFocus`. The QtTUI widgets that take keys set `Qt::StrongFocus`.

| Policy | Tab | Click | `setFocus()` |
| --- | --- | --- | --- |
| `Qt::NoFocus` | No | No | Does nothing |
| `Qt::TabFocus` | Yes | No | Yes |
| `Qt::ClickFocus` | No | Yes | Yes |
| `Qt::StrongFocus` | Yes | Yes | Yes |
| `Qt::WheelFocus` | Yes | Yes | Yes |

When the focus moves, the old focus widget emits `focusLost()`, and the new
focus widget emits `focusGained()`. Both widgets paint again. Then the
application calls `focusEntered()` on the new focus widget and on each of
its parents. A container, for example a scroll area, uses
`focusEntered()` to scroll the focus widget into view.

### Tab order

Tab moves the focus to the next widget in the focus chain, and Shift+Tab
moves it to the previous widget. The chain wraps at the two ends.
`QTuiMainWindow` makes the chain again for each Tab key. The chain has the
widgets that agree with all these conditions:

- The widget and all its parents are visible.
- The focus policy has the `Qt::TabFocus` bit.
- The widget is effectively enabled.

The order is the order of the widget tree: a parent comes before its
children, and the children are in the order that you made them.
`raise()` moves a widget to the end of this order. There is no
`setTabOrder()`.

While a modal dialog is open, Tab moves only between the widgets of the
dialog. While the focus is in a popup (a menu or a dropdown list), Tab does
not leave the popup. Tab and a mouse click also show the tooltip of the
new focus widget. `setFocus()` does not show it.

### Hover

The main window follows the mouse. `underMouse()` returns `true` while the
mouse is on the widget. `hoverPos()` returns the mouse position in screen
cells. A change of `underMouse()` marks the widget dirty. A change of
`hoverPos()` marks only the old and the new `hoverRect()` dirty.

The render pass shows a hover highlight on `hoverRect()`. The highlight
moves the background color 15% toward the foreground color, only on the
cells that have the theme background. It shows only on enabled widgets
with the `Qt::ClickFocus` bit. The default `hoverRect()` is the full
geometry. Override it to highlight a smaller area, for example one row.
Return an empty rectangle for no highlight.

The hover needs the `QTTUI_MOUSE=all` mode, which is the default. See
[Events and rendering](events-and-rendering.md).

### Tooltips

`setToolTip()` sets the text that `QTuiToolTip` shows. The tooltip shows
below the widget, or above it if there is no space below. It shows in these
cases:

- The user moves the focus to the widget with Tab, Shift+Tab or a click.
- The mouse stays on the widget for 700 ms.
- The user pushes F1. F1 shows the tooltip of the focus widget again, so
  the tooltip is also available without a mouse.

A key press or a mouse press hides the tooltip.

### Context menu

A right click on a widget, or Shift+F10 or the Menu key on the focus
widget, asks for a context menu. The context menu policy of the widget
tells what occurs:

| Policy | Action |
| --- | --- |
| `Qt::DefaultContextMenu` | The parent widget gets the request. This is the default. |
| `Qt::CustomContextMenu` | The widget emits `customContextMenuRequested(pos)`. `pos` is relative to the top-left corner of the widget. |
| `Qt::ActionsContextMenu` | A `QTuiMenu` with `actions()` opens. With no actions, nothing occurs. |
| `Qt::NoContextMenu`, `Qt::PreventContextMenu` | Nothing occurs, and the parent does not get the request. |

For Shift+F10, `pos` is the cell below the top-left corner of the widget.

### Layout direction

`layoutDirection()` returns the direction of the widget. A widget without
its own direction uses the direction of its parent. A widget without a
parent uses `QTuiApplication::layoutDirection()`. In a right-to-left
widget, the box, grid and form layouts mirror horizontally. QtTUI does not
reorder bidirectional text: the text in a cell shows in its stored order.

### Dirty tracking

The render pass paints only the widgets in the dirty areas. Call
`markDirty()` each time a change has an effect on the look of the widget.
`markDirty()` marks the full geometry. `markDirty(QRect)` marks only one
area, in screen cells. Many calls in one event loop iteration cause one
render pass.

The setters of `QTuiWidget` call `markDirty()` for you, for example
`setGeometry()`, `setVisible()`, `setEnabled()` and `setFocus()`. In a
subclass, each setter that changes the look must call `markDirty()`. A
child of a widget that paints its own children (`isPaintedByParent()`)
marks the painting parent dirty instead. For the full render path, see
[Events and rendering](events-and-rendering.md).

### Paint and events in a subclass

- `paintEvent(ScreenBuffer &buffer)`: write cells into the back buffer, in
  your own geometry. Call `clearBackground()` first. The render pass clips
  each widget to the area of its parent. The `RenderUtils.h` helpers
  (`fillRect()`, `renderText()` and others) write cells for you.
- `keyPressEvent(TuiKeyEvent *event)`: the key is `event->key()` (a
  `Qt::Key`), with `event->modifiers()` and `event->text()`.
- `mousePressEvent(TuiMouseEvent *event)`: this function gets all mouse
  events. Use `event->mouseType()` to find the type: `Press`, `Release`,
  `Move`, `Wheel` or `DoubleClick`. `event->pos()` is in screen cells.
  After a left press, the widget also gets the drag motion and the release,
  also outside its geometry.
- `pasteEvent(TuiPasteEvent *event)`: the text that the user pastes into
  the terminal, as one block.
- `resizeEvent(TuiResizeEvent *event)`: the terminal size changed.
  `QTuiMainWindow` gets this event.

For the event classes, see [Events and rendering](events-and-rendering.md).

### Properties

| Property | Type | Access |
| --- | --- | --- |
| `geometry` | `QRect` | `geometry()`, `setGeometry()`, notify `geometryChanged()` |
| `visible` | `bool` | `isVisible()`, `setVisible()`, notify `visibilityChanged()` |
| `enabled` | `bool` | `isEnabled()`, `setEnabled()`, notify `enabledChanged()` |
| `focusPolicy` | `Qt::FocusPolicy` | `focusPolicy()`, `setFocusPolicy()` |
| `toolTip` | `QString` | `toolTip()`, `setToolTip()` |
| `accessibleName` | `QString` | `accessibleName()`, `setAccessibleName()` |
| `accessibleDescription` | `QString` | `accessibleDescription()`, `setAccessibleDescription()` |

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiWidget(QTuiWidget *parent = nullptr)` | Makes a widget. The parent owns it, as a `QObject` child. |
| `~QTuiWidget()` | Destroys the widget. If the widget is the focus widget, no widget has the focus after it. |
| `QRect geometry() const` | Returns the geometry in absolute screen cells. |
| `void setGeometry(const QRect &rect)` | Sets the geometry. Marks the old and the new area dirty, applies the layout, and emits `geometryChanged()`. |
| `bool isVisible() const` | Returns the visible flag of the widget. The default is `true`. |
| `void setVisible(bool visible)` | Shows or hides the widget, marks its area dirty, and emits `visibilityChanged()`. For a popup, it also keeps and restores the focus. For other widgets, a hide clears the focus if it is in the widget. |
| `bool isEnabled() const` | Returns the enabled flag of the widget. |
| `void setEnabled(bool enabled)` | Enables or disables the widget and its children. Clears the focus if the focus is in the widget. Emits `enabledChanged()`. |
| `bool isEffectivelyEnabled() const` | Returns `true` if the widget and all its parents are enabled. |
| `Qt::FocusPolicy focusPolicy() const` | Returns the focus policy. The default is `Qt::NoFocus`. |
| `void setFocusPolicy(Qt::FocusPolicy policy)` | Sets the focus policy. With `Qt::NoFocus`, the widget loses the focus. |
| `virtual bool acceptsTextInput() const` | Returns `true` for a widget that uses printable keys as text. The default is `false`. While such a widget has the focus, a shortcut without a modifier (for example "Q") does not fire, and Ctrl+Z goes to the widget. |
| `bool hasFocus() const` | Returns `true` if the widget is the focus widget. |
| `void setFocus()` | Makes the widget the focus widget. Does nothing for `Qt::NoFocus`. |
| `static QTuiWidget *focusWidget()` | Returns the focus widget, or `nullptr`. |
| `bool isPopup() const` | Returns `true` if the widget is a popup. |
| `void setPopup(bool popup)` | Makes the widget a popup. A popup paints last, above all other widgets, clipped only to the screen. When a popup shows, it keeps the current focus widget. When it hides with the focus inside, it gives the focus back. Menus, dropdown lists and dialogs use it. |
| `bool isPaintedByParent() const` | Returns `true` if the parent paints this widget. |
| `void setPaintedByParent(bool paintedByParent)` | Tells that the parent paints the widget and that its geometry is in the coordinates of the parent (for example the content of a `QTuiScrollArea`). The render pass and the mouse hit test skip it. |
| `QString toolTip() const` | Returns the tooltip text. |
| `void setToolTip(const QString &toolTip)` | Sets the tooltip text. |
| `QString accessibleName() const` | Returns the accessible name. |
| `void setAccessibleName(const QString &name)` | Sets the accessible name. QtTUI only keeps it; it has no screen reader interface. |
| `QString accessibleDescription() const` | Returns the accessible description. |
| `void setAccessibleDescription(const QString &description)` | Sets the accessible description. QtTUI only keeps it. |
| `bool underMouse() const` | Returns `true` while the mouse is on the widget. |
| `QPoint hoverPos() const` | Returns the mouse position in screen cells, while `underMouse()` is `true`. |
| `virtual QRect hoverRect() const` | Returns the area of the hover highlight in screen cells. The default is `geometry()`. |
| `Qt::LayoutDirection layoutDirection() const` | Returns the layout direction of the widget, of its parent, or of the application. |
| `void setLayoutDirection(Qt::LayoutDirection direction)` | Sets the layout direction of the widget. Marks the layout for a new calculation and marks the widget dirty. |
| `void unsetLayoutDirection()` | Removes the direction of the widget. The widget then uses the direction of its parent. |
| `bool isRightToLeft() const` | Returns `true` if `layoutDirection()` is `Qt::RightToLeft`. |
| `Qt::ContextMenuPolicy contextMenuPolicy() const` | Returns the context menu policy. The default is `Qt::DefaultContextMenu`. |
| `void setContextMenuPolicy(Qt::ContextMenuPolicy policy)` | Sets the context menu policy. |
| `void addAction(QTuiAction *action)` | Adds an action for `Qt::ActionsContextMenu`. The widget does not take ownership. The function ignores `nullptr` and an action that is already in the list. |
| `void removeAction(QTuiAction *action)` | Removes an action. |
| `QList<QTuiAction *> actions() const` | Returns the actions. A deleted action is not in the list. |
| `virtual void popupOutsideClick()` | The main window calls it on each visible popup when a mouse press is outside all visible popups. The default hides the popup. The press does not go to the widget below. |
| `bool isAncestorOf(const QTuiWidget *child) const` | Returns `true` if `child` is this widget, or a child of it at any level. |
| `virtual QPoint terminalCursorPos() const` | Returns the position of the terminal cursor in screen cells while the widget has the focus. The default `(-1, -1)` hides the cursor. An editable widget returns its edit position. |
| `QTuiWidget *parentWidget() const` | Returns the parent widget, or `nullptr`. |
| `QList<QTuiWidget *> childWidgets() const` | Returns the child widgets in their order. |
| `void raise()` | Moves the widget to the end of its siblings. It then paints last and gets the mouse first. |
| `virtual QSize minimumSize() const` | Returns the minimum size. The default is the minimum size of the layout, or 0x0 with no layout. |
| `virtual QSize sizeHint() const` | Returns the preferred size. The default is `minimumSize()`. |
| `QTuiSizePolicy sizePolicy() const` | Returns the size policy. The default is `Preferred` on the two axes. |
| `void setSizePolicy(const QTuiSizePolicy &policy)` | Sets the size policy, and marks the layout of the parent for a new calculation. |
| `void setSizePolicy(QTuiSizePolicy::Policy horizontal, QTuiSizePolicy::Policy vertical)` | Sets the size policy from two values. |
| `virtual void paintEvent(ScreenBuffer &buffer)` | Paints the widget. The default clears the background. |
| `virtual void setLayout(QTuiLayout *layout)` | Sets the layout. The widget takes ownership and applies the layout each time its geometry changes. |
| `virtual QTuiLayout *layout() const` | Returns the layout, or `nullptr`. |
| `void clearBackground(ScreenBuffer &buffer, const QRect &clipRect)` | Fills the geometry, clipped to `clipRect`, with spaces in the theme colors. If the widget became smaller, it also clears the old area with the colors of the parent. |
| `ThemeProperties themeProperties() const` | Returns the theme properties of the widget: a per-widget override, or the entry for its class name. See [Themes](themes.md). |
| `void markDirty()` | Marks the full geometry for a repaint. |
| `void markDirty(const QRect &region)` | Marks an area in screen cells for a repaint. |
| `bool hasDirtyRegions() const` | Returns `true` after a geometry change, until `clearDirtyRegions()`. |
| `bool isDirty() const` | Returns `true` after `markDirty()`, until `clearDirtyRegions()`. |
| `QRect dirtyOldRegion() const` | Returns the geometry before the last geometry change. |
| `QRect dirtyNewRegion() const` | Returns the geometry after the last geometry change. |
| `QRect dirtyRegion() const` | Returns the union of the areas from `markDirty()`. |
| `void clearDirtyRegions()` | Clears the dirty state. The render pass calls it. |

### Protected functions

| Function | Description |
| --- | --- |
| `bool event(QEvent *event) override` | Sends the QtTUI events to the handlers below. Drops key, mouse and paste events if the widget is not effectively enabled. |
| `virtual void keyPressEvent(TuiKeyEvent *event)` | Gets a key for the focus widget. The default does nothing. |
| `virtual void mousePressEvent(TuiMouseEvent *event)` | Gets all mouse events: press, release, move, wheel and double click. The default does nothing. |
| `virtual void resizeEvent(TuiResizeEvent *event)` | Gets a terminal resize. The default does nothing. |
| `virtual void pasteEvent(TuiPasteEvent *event)` | Gets pasted text for the focus widget. The default does nothing. |
| `virtual void focusEntered(QTuiWidget *focus)` | Called on the new focus widget and on each of its parents. The default does nothing. |

### Signals

| Signal | Description |
| --- | --- |
| `geometryChanged(const QRect &rect)` | The geometry changed. |
| `visibilityChanged(bool visible)` | The visible flag changed. |
| `enabledChanged(bool enabled)` | The enabled flag changed. |
| `focusGained()` | The widget became the focus widget. |
| `focusLost()` | The widget is no longer the focus widget. |
| `customContextMenuRequested(const QPoint &pos)` | The user asked for a context menu, and the policy is `Qt::CustomContextMenu`. `pos` is relative to the top-left corner of the widget. |

### Keys

The main window handles these keys for all widgets, before the focus
widget gets the key:

| Key | Action |
| --- | --- |
| Tab | Move the focus to the next widget in the focus chain. |
| Shift+Tab | Move the focus to the previous widget. |
| F1 | Show the tooltip of the focus widget. |
| Shift+F10, Menu | Ask the focus widget for a context menu. |
| Ctrl+Z | Suspend the application, if the focus widget does not accept text input. |

### Mouse

- A press of a mouse button on a widget with the `Qt::ClickFocus` bit gives it
  the focus and shows its tooltip.
- A right press asks the widget for a context menu.
- Two presses of the same button on the same cell in 400 ms send a
  `DoubleClick` event after the second press.
- Mouse motion moves the hover. Mouse motion without a button does not go
  to the widget.

## QTuiClipboard

`#include <QtTUI/QTuiClipboard.h>` · Inherits: `QObject`

`QTuiClipboard` keeps text for copy and paste in the application. It also
sends the text to the system clipboard with an OSC 52 escape sequence.
Different from `QClipboard`, it cannot read the system clipboard: most
terminals block OSC 52 reads.

Use `QTuiApplication::clipboard()` to get the application clipboard. To
paste from the system clipboard, the user pastes into the terminal. The
focus widget then gets a `TuiPasteEvent`. A terminal without OSC 52 ignores
the sequence.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

void copyName(QTuiLineEdit *edit)
{
    QTuiClipboard *clipboard = QTuiApplication::clipboard();
    QObject::connect(clipboard, &QTuiClipboard::dataChanged, edit, [clipboard]() {
        qInfo() << "Clipboard:" << clipboard->text();
    });
    clipboard->setText(edit->text());
}
```

### Functions

| Function | Description |
| --- | --- |
| `explicit QTuiClipboard(QObject *parent = nullptr)` | Makes a clipboard. Usually you use `QTuiApplication::clipboard()`. |
| `QString text() const` | Returns the last text that the application set. |
| `void setText(const QString &text)` | Keeps the text, sends it to the system clipboard with OSC 52 if a terminal is in use, and emits `dataChanged()`. |
| `void clear()` | Sets the text to an empty string. This also sends an empty OSC 52 sequence. |
| `static QByteArray osc52Sequence(const QString &text)` | Returns the OSC 52 sequence that sets the system clipboard to `text`: `ESC ] 52 ; c ;`, the UTF-8 text in base64, then `BEL`. |

### Signals

| Signal | Description |
| --- | --- |
| `dataChanged()` | `setText()` or `clear()` changed the text. The signal comes also if the text is the same. |
