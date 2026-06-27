#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiKeySequence.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/QTuiStatusBar.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/QTuiToolTip.h>
#include <QtTUI/QTuiDialog.h>
#include <QtTUI/QTuiMenu.h>
#include <QCoreApplication>
#include <QDebug>

namespace QtTUI {

QTuiMainWindow::QTuiMainWindow(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

void QTuiMainWindow::setCentralWidget(QTuiWidget *widget)
{
    if (m_centralWidget == widget)
        return;

    if (m_centralWidget) {
        disconnect(m_centralWidget, &QObject::destroyed, this, nullptr);
        m_centralWidget->setParent(nullptr);
    }

    m_centralWidget = widget;

    if (m_centralWidget) {
        // Clear the cached pointer if the widget is destroyed externally
        connect(m_centralWidget, &QObject::destroyed, this, [this]() {
            m_centralWidget = nullptr;
            markDirty();
        });
        m_centralWidget->setParent(this);
        m_centralWidget->setGeometry(geometry());
    }

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

QTuiWidget *QTuiMainWindow::centralWidget() const
{
    return m_centralWidget;
}

void QTuiMainWindow::setLayout(QTuiLayout *layout)
{
    if (m_layout == layout)
        return;

    if (layout && layout->parent() != this)
        layout->setParent(this);

    if (m_layout)
        disconnect(m_layout, &QObject::destroyed, this, nullptr);

    m_layout = layout;

    if (m_layout) {
        // Clear the cached pointer if the layout is destroyed externally
        connect(m_layout, &QObject::destroyed, this, [this]() {
            m_layout = nullptr;
        });
    }
    if (!geometry().isEmpty())
        relayoutChildren();
}

QTuiLayout *QTuiMainWindow::layout() const
{
    return m_layout;
}

// --- Menu bar ---

void QTuiMainWindow::setMenuBar(QTuiMenuBar *menuBar)
{
    if (m_menuBar == menuBar)
        return;

    if (m_menuBar) {
        disconnect(m_menuBar, &QObject::destroyed, this, nullptr);
        m_menuBar->setParent(nullptr);
    }

    m_menuBar = menuBar;

    if (m_menuBar) {
        // Clear the cached pointer if the menu bar is destroyed externally
        connect(m_menuBar, &QObject::destroyed, this, [this]() {
            m_menuBar = nullptr;
            markDirty();
        });
        m_menuBar->setParent(this);
    }

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

QTuiMenuBar *QTuiMainWindow::menuBar() const
{
    return m_menuBar;
}

// --- Toolbars ---

void QTuiMainWindow::addToolBar(QTuiToolBar *toolBar)
{
    if (!toolBar || m_toolBars.contains(toolBar))
        return;

    toolBar->setParent(this);
    m_toolBars.append(toolBar);

    // Remove the cached pointer if the toolbar is destroyed externally
    connect(toolBar, &QObject::destroyed, this, [this, toolBar]() {
        m_toolBars.removeOne(toolBar);
        markDirty();
    });

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

void QTuiMainWindow::removeToolBar(QTuiToolBar *toolBar)
{
    if (!toolBar || !m_toolBars.contains(toolBar))
        return;

    m_toolBars.removeOne(toolBar);
    disconnect(toolBar, &QObject::destroyed, this, nullptr);
    toolBar->setParent(nullptr);

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

QList<QTuiToolBar *> QTuiMainWindow::toolBars() const
{
    return m_toolBars;
}

// --- Status bar ---

void QTuiMainWindow::setStatusBar(QTuiStatusBar *statusBar)
{
    if (m_statusBar == statusBar)
        return;

    if (m_statusBar) {
        disconnect(m_statusBar, &QObject::destroyed, this, nullptr);
        m_statusBar->setParent(nullptr);
    }

    m_statusBar = statusBar;

    if (m_statusBar) {
        // Clear the cached pointer if the status bar is destroyed externally
        connect(m_statusBar, &QObject::destroyed, this, [this]() {
            m_statusBar = nullptr;
            markDirty();
        });
        m_statusBar->setParent(this);
    }

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

QTuiStatusBar *QTuiMainWindow::statusBar() const
{
    return m_statusBar;
}

// --- Dock widgets ---

void QTuiMainWindow::addDockWidget(Qt::DockWidgetArea area, QTuiDockWidget *dockWidget)
{
    if (!dockWidget)
        return;

    // Remove from any existing area first
    for (auto it = m_dockWidgets.begin(); it != m_dockWidgets.end(); ++it) {
        it.value().removeOne(dockWidget);
    }

    dockWidget->setParent(this);
    m_dockWidgets[area].append(dockWidget);

    // Remove the cached pointer if the dock widget is destroyed externally.
    // Disconnect first: re-adding a dock to another area must not stack
    // duplicate connections.
    disconnect(dockWidget, &QObject::destroyed, this, nullptr);
    connect(dockWidget, &QObject::destroyed, this, [this, dockWidget]() {
        for (auto it = m_dockWidgets.begin(); it != m_dockWidgets.end(); ++it)
            it.value().removeOne(dockWidget);
        markDirty();
    });

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

void QTuiMainWindow::removeDockWidget(QTuiDockWidget *dockWidget)
{
    if (!dockWidget)
        return;

    for (auto it = m_dockWidgets.begin(); it != m_dockWidgets.end(); ++it) {
        if (it.value().removeOne(dockWidget)) {
            disconnect(dockWidget, &QObject::destroyed, this, nullptr);
            dockWidget->setParent(nullptr);
            break;
        }
    }

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

// --- Show ---

void QTuiMainWindow::show()
{
    setVisible(true);
    if (geometry().isEmpty()) {
        auto *app = QTuiApplication::instance();
        if (app && app->backend()) {
            QSize sz = app->backend()->terminalSize();
            setGeometry(QRect(0, 0, sz.width(), sz.height()));
        }
    }
    relayout();
}

// --- Paint ---

void QTuiMainWindow::paintEvent(ScreenBuffer &buffer)
{
    // The main window itself has no chrome to paint.
    // Child painting is handled by RenderScheduler::paintTree() which
    // recurses into children with clip-rect propagation.
    Q_UNUSED(buffer);
}

// --- Events ---

// Collects visible popups in pre-order: a parent popup precedes the popups in
// its subtree, so the LAST popup containing a point is the topmost one.
static void collectVisiblePopupsIn(QTuiWidget *widget, QList<QTuiWidget *> &popups)
{
    for (auto *child : widget->childWidgets()) {
        if (!child->isVisible())
            continue;
        if (child->isPopup())
            popups.append(child);
        collectVisiblePopupsIn(child, popups);
    }
}

// True if widget is a visible popup or lives inside one.
static bool insideVisiblePopup(const QTuiWidget *widget)
{
    for (const QTuiWidget *w = widget; w; w = w->parentWidget()) {
        if (w->isPopup() && w->isVisible())
            return true;
    }
    return false;
}

void QTuiMainWindow::resizeEvent(TuiResizeEvent *event)
{
    setGeometry(QRect(0, 0, event->size().width(), event->size().height()));
    relayout();
}

// True if the sequence's key would type text into the focused widget: bare
// printable-key shortcuts (e.g. "Q") must yield to text editing.
// The standard edit keys that a focused editor uses before any shortcut,
// as QLineEdit does through ShortcutOverride: undo, redo, copy, cut,
// paste and select all
static bool isStandardEditKey(int key, Qt::KeyboardModifiers mods)
{
    if (mods == Qt::ControlModifier)
        return key == Qt::Key_Z || key == Qt::Key_Y || key == Qt::Key_C
            || key == Qt::Key_X || key == Qt::Key_V || key == Qt::Key_A;
    return key == Qt::Key_Z && mods == (Qt::ControlModifier | Qt::ShiftModifier);
}

static bool shortcutShadowedByEditor(const QTuiKeySequence &seq, QTuiWidget *focused)
{
    if (!focused || !focused->acceptsTextInput())
        return false;
    return seq.isBareTextKey() || isStandardEditKey(seq.key(), seq.modifiers());
}

// As in Qt, a shortcut works only while the widgets that own its action are
// visible: an action on a hidden tab page does not take the key. A popup
// (a closed menu) is hidden by design, so it does not block its actions.
// An action without a widget owner is global.
static bool actionReachable(const QTuiAction *action)
{
    for (QObject *o = action->parent(); o; o = o->parent()) {
        auto *w = qobject_cast<QTuiWidget *>(o);
        if (w && !w->isPopup() && !w->isVisible())
            return false;
    }
    return true;
}

// Global shortcut dispatch (backlog #9): runs before Tab handling and
// focus-widget delivery. Returns true when the event was consumed.
//
// Shortcuts fire even while a popup (menu, dropdown) has focus: modified
// shortcuts and function keys don't collide with popup navigation keys, and
// triggering an action closes menus anyway. Bare printable-key shortcuts
// are suppressed only while the focused widget accepts text input.
bool QTuiMainWindow::dispatchShortcut(TuiKeyEvent *event)
{
    QTuiWidget *focused = QTuiWidget::focusWidget();
    // A modal dialog blocks the shortcuts of all other widgets
    QTuiDialog *modal = activeModalDialog();

    const auto actions = QTuiAction::registeredActions();
    for (QTuiAction *action : actions) {
        if (!action->isEnabled())
            continue;
        const QTuiKeySequence seq = action->shortcutSequence();
        if (!seq.matches(event->key(), event->modifiers()))
            continue;
        if (shortcutShadowedByEditor(seq, focused))
            continue;
        if (!actionReachable(action))
            continue;
        if (modal) {
            auto *owner = qobject_cast<QTuiWidget *>(action->parent());
            if (!owner || (owner != modal && !modal->isAncestorOf(owner)))
                continue;
        }
        action->trigger();
        return true;
    }

    // Menu bar Alt+mnemonics work globally, not only while the bar has focus:
    // Alt+letter (no Ctrl) opens the matching menu.
    if (!modal && m_menuBar && m_menuBar->isVisible()
        && (event->modifiers() & Qt::AltModifier)
        && !(event->modifiers() & Qt::ControlModifier)
        && !event->text().isEmpty()) {
        if (m_menuBar->triggerMnemonic(event->text().at(0)))
            return true;
    }

    return false;
}

void QTuiMainWindow::pasteEvent(TuiPasteEvent *event)
{
    // Pasted text goes to the focused widget, as typed keys do
    QTuiWidget *focused = findFocusedWidget(this);
    if (focused && focused != this)
        QCoreApplication::sendEvent(focused, event);
}

void QTuiMainWindow::keyPressEvent(TuiKeyEvent *event)
{
    QTuiToolTip::hideText();

    if (dispatchShortcut(event))
        return;

    // Shift+F10 (or the Menu key) asks for the context menu of the focus widget
    if ((event->key() == Qt::Key_F10 && event->modifiers() == Qt::ShiftModifier)
        || event->key() == Qt::Key_Menu) {
        if (QTuiWidget *focus = QTuiWidget::focusWidget()) {
            requestContextMenu(focus, focus->geometry().topLeft() + QPoint(0, 1));
            return;
        }
    }

    // Ctrl+Z suspends the program, unless an action took it above or a
    // focused editor uses it for undo
    QTuiWidget *editor = QTuiWidget::focusWidget();
    if (event->key() == Qt::Key_Z && event->modifiers() == Qt::ControlModifier
        && !(editor && editor->acceptsTextInput())) {
        if (auto *app = QTuiApplication::instance())
            app->suspend();
        return;
    }

    // Find the focused widget anywhere in the tree
    QTuiWidget *focused = findFocusedWidget(this);

    // While focus is inside an open popup, the popup is modal for keys: Tab
    // must not walk the window focus chain out from under it. A dialog
    // moves focus between its own widgets; a menu has one Tab stop.
    const bool popupHasFocus = focused && insideVisiblePopup(focused);

    if (QTuiDialog *dialog = activeModalDialog()) {
        if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) {
            const bool forward = event->key() == Qt::Key_Tab
                && !(event->modifiers() & Qt::ShiftModifier);
            advanceFocus(forward, dialog);
            return;
        }
        // Escape rejects the dialog, unless a popup inside it (a completer
        // or a dropdown) is open: that popup closes first
        if (event->key() == Qt::Key_Escape) {
            QList<QTuiWidget *> inner;
            collectVisiblePopupsIn(dialog, inner);
            if (inner.isEmpty()) {
                dialog->reject();
                return;
            }
        }
        // Keys reach only the dialog
        if (!focused || !dialog->isAncestorOf(focused))
            return;
    }

    if (!popupHasFocus) {
        // Tab forward
        if (event->key() == Qt::Key_Tab && !(event->modifiers() & Qt::ShiftModifier)) {
            advanceFocus(true);
            return;
        }

        // Shift+Tab backward (terminals send Key_Backtab for Shift+Tab)
        if (event->key() == Qt::Key_Backtab
            || (event->key() == Qt::Key_Tab && (event->modifiers() & Qt::ShiftModifier))) {
            advanceFocus(false);
            return;
        }
    }

    // Dispatch to the focused widget
    if (focused && focused != this) {
        QCoreApplication::sendEvent(focused, event);
    }
}

void QTuiMainWindow::mousePressEvent(TuiMouseEvent *event)
{
    // Hide the tooltip first: it is a popup, and a click outside a visible
    // popup is swallowed
    if (event->mouseType() == TuiMouseEvent::Press)
        QTuiToolTip::hideText();

    // Popups float above everything, so hit-test them first; the topmost
    // (deepest / most recently opened, i.e. last collected) popup wins.
    QList<QTuiWidget *> popups;
    collectVisiblePopupsIn(this, popups);

    QTuiWidget *target = nullptr;
    for (int i = popups.size() - 1; i >= 0 && !target; --i) {
        if (popups.at(i)->geometry().contains(event->pos()))
            target = findWidgetAt(popups.at(i), event->pos());
    }

    // A press outside every visible popup dismisses them all and is swallowed:
    // the click does not reach the widget underneath (matching QMenu). Each
    // popup runs its own close logic via popupOutsideClick().
    if (!target && !popups.isEmpty() && event->mouseType() == TuiMouseEvent::Press) {
        for (auto *popup : popups) {
            if (popup->isVisible()) // an earlier popup may have closed this one
                popup->popupOutsideClick();
        }
        return;
    }

    // A modal dialog takes no part in events outside it: a release, move
    // or wheel there must not reach the widgets behind the dialog
    if (!target) {
        if (QTuiDialog *dialog = activeModalDialog()) {
            if (!dialog->geometry().contains(event->pos()))
                return;
        }
        target = findWidgetAt(this, event->pos());
    }

    if (target && target != this) {
        // Only an actual press moves focus, and only onto widgets whose
        // policy has the ClickFocus bit and that are effectively enabled;
        // Move/Release/Wheel and NoFocus widgets must not steal focus. The
        // event is still delivered (a disabled target consumes it in the
        // base event handler).
        if (event->mouseType() == TuiMouseEvent::Press
            && (target->focusPolicy() & Qt::ClickFocus)
            && target->isEffectivelyEnabled()) {
            target->setFocus();
            QTuiToolTip::showFor(target);
        }
        QCoreApplication::sendEvent(target, event);

        // A right press asks for a context menu
        if (event->mouseType() == TuiMouseEvent::Press && event->button() == Qt::RightButton)
            requestContextMenu(target, event->pos());

        // A second press of the same button on the same cell soon after the
        // first one is a double click; a third press starts again
        if (event->mouseType() == TuiMouseEvent::Press) {
            static constexpr qint64 kDoubleClickMs = 400;
            const bool isDouble = m_lastPressTimer.isValid()
                && m_lastPressTimer.elapsed() <= kDoubleClickMs
                && m_lastPressPos == event->pos() && m_lastPressButton == event->button();
            if (isDouble) {
                m_lastPressTimer.invalidate();
                TuiMouseEvent dbl(TuiMouseEvent::DoubleClick, event->button(), event->pos(),
                                  event->modifiers());
                QCoreApplication::sendEvent(target, &dbl);
            } else {
                m_lastPressTimer.start();
                m_lastPressPos = event->pos();
                m_lastPressButton = event->button();
            }
        }
    }
}

// --- Layout ---

void QTuiMainWindow::relayout()
{
    const QRect bounds = geometry();
    if (bounds.isEmpty())
        return;

    int top = bounds.y();
    int bottom = bounds.y() + bounds.height();
    int left = bounds.x();
    int right = bounds.x() + bounds.width();

    // 1. Menu bar: 1 row at top
    if (m_menuBar && m_menuBar->isVisible()) {
        m_menuBar->setGeometry(QRect(left, top, right - left, 1));
        top += 1;
    }

    // 2. Toolbars: 1 row each below menu bar
    for (auto *tb : m_toolBars) {
        if (tb && tb->isVisible()) {
            tb->setGeometry(QRect(left, top, right - left, 1));
            top += 1;
        }
    }

    // 3. Status bar: 1 row at bottom
    if (m_statusBar && m_statusBar->isVisible()) {
        bottom -= 1;
        m_statusBar->setGeometry(QRect(left, bottom, right - left, 1));
    }

    // 4. Dock widgets from corresponding edges
    // Top docks
    const auto topDocks = m_dockWidgets.value(Qt::TopDockWidgetArea);
    for (auto *dock : topDocks) {
        if (dock && dock->isVisible()) {
            int h = dock->minimumSize().height();
            if (h < 1) h = 3;
            if (top + h > bottom) h = bottom - top;
            if (h > 0) {
                dock->setGeometry(QRect(left, top, right - left, h));
                top += h;
            }
        }
    }

    // Bottom docks
    const auto bottomDocks = m_dockWidgets.value(Qt::BottomDockWidgetArea);
    for (auto *dock : bottomDocks) {
        if (dock && dock->isVisible()) {
            int h = dock->minimumSize().height();
            if (h < 1) h = 3;
            if (top + h > bottom) h = bottom - top;
            if (h > 0) {
                bottom -= h;
                dock->setGeometry(QRect(left, bottom, right - left, h));
            }
        }
    }

    // Left docks
    const auto leftDocks = m_dockWidgets.value(Qt::LeftDockWidgetArea);
    for (auto *dock : leftDocks) {
        if (dock && dock->isVisible()) {
            int w = dock->minimumSize().width();
            if (w < 1) w = 10;
            if (left + w > right) w = right - left;
            if (w > 0) {
                dock->setGeometry(QRect(left, top, w, bottom - top));
                left += w;
            }
        }
    }

    // Right docks
    const auto rightDocks = m_dockWidgets.value(Qt::RightDockWidgetArea);
    for (auto *dock : rightDocks) {
        if (dock && dock->isVisible()) {
            int w = dock->minimumSize().width();
            if (w < 1) w = 10;
            if (left + w > right) w = right - left;
            if (w > 0) {
                right -= w;
                dock->setGeometry(QRect(right, top, w, bottom - top));
            }
        }
    }

    // 5. Central widget / layout: remaining area
    QRect centralArea(left, top, right - left, bottom - top);
    if (m_layout) {
        m_layout->apply(centralArea);
    } else if (m_centralWidget) {
        m_centralWidget->setGeometry(centralArea);
    }
}

void QTuiMainWindow::relayoutChildren()
{
    // If we have window components, use relayout which handles everything
    if (m_menuBar || !m_toolBars.isEmpty() || m_statusBar || !m_dockWidgets.isEmpty()) {
        relayout();
        return;
    }

    // With a layout installed, geometry management is entirely the layout's
    // job; the manual clamping below only applies to layout-less windows.
    if (m_layout) {
        m_layout->apply(geometry());
        return;
    }

    if (m_centralWidget)
        m_centralWidget->setGeometry(geometry());

    for (auto *child : childWidgets()) {
        if (child != m_centralWidget && child->isVisible()) {
            QRect childGeo = child->geometry();
            QRect bounds = geometry();
            if (childGeo.right() > bounds.right() || childGeo.bottom() > bounds.bottom()) {
                childGeo = childGeo.intersected(bounds);
                child->setGeometry(childGeo);
            }
        }
    }
}

void QTuiMainWindow::requestContextMenu(QTuiWidget *widget, const QPoint &pos)
{
    for (QTuiWidget *w = widget; w && w != this; w = w->parentWidget()) {
        switch (w->contextMenuPolicy()) {
        case Qt::NoContextMenu:
        case Qt::PreventContextMenu:
            return;
        case Qt::CustomContextMenu:
            emit w->customContextMenuRequested(pos - w->geometry().topLeft());
            return;
        case Qt::ActionsContextMenu: {
            const QList<QTuiAction *> actions = w->actions();
            if (actions.isEmpty())
                return;
            // A menu for this request only; it deletes itself when it closes
            auto *menu = new QTuiMenu(QString(), w);
            for (QTuiAction *action : actions)
                menu->addAction(action);
            connect(menu, &QTuiMenu::aboutToHide, menu, &QObject::deleteLater);
            menu->popup(pos);
            return;
        }
        case Qt::DefaultContextMenu:
        default:
            break; // ask the parent
        }
    }
}

QTuiDialog *QTuiMainWindow::activeModalDialog() const
{
    QList<QTuiWidget *> popups;
    collectVisiblePopupsIn(const_cast<QTuiMainWindow *>(this), popups);
    for (int i = popups.size() - 1; i >= 0; --i) {
        if (auto *dialog = qobject_cast<QTuiDialog *>(popups.at(i)))
            return dialog;
    }
    return nullptr;
}

void QTuiMainWindow::advanceFocus(bool forward, QTuiWidget *root)
{
    // With a root (a popup that holds focus), Tab moves only inside it
    if (root) {
        m_focusChain.clear();
        collectFocusableWidgets(root, m_focusChain);
    } else {
        rebuildFocusChain();
    }

    if (m_focusChain.isEmpty())
        return;

    const int n = m_focusChain.size();
    int currentIdx = -1;

    for (int i = 0; i < n; ++i) {
        if (m_focusChain[i]->hasFocus()) {
            currentIdx = i;
            break;
        }
    }

    int nextIdx;
    if (currentIdx < 0) {
        nextIdx = forward ? 0 : n - 1;
    } else {
        nextIdx = forward ? (currentIdx + 1) % n : (currentIdx - 1 + n) % n;
    }

    m_focusChain[nextIdx]->setFocus();
    QTuiToolTip::showFor(m_focusChain[nextIdx]);
}

void QTuiMainWindow::rebuildFocusChain()
{
    m_focusChain.clear();
    collectFocusableWidgets(this, m_focusChain);
}

void QTuiMainWindow::collectFocusableWidgets(QTuiWidget *widget, QList<QTuiWidget *> &chain)
{
    // Tab stops are widgets that opted in via focusPolicy (TabFocus bit),
    // are visible, and are effectively enabled — labels, containers without
    // key handling, and disabled widgets are skipped (backlog #12). Pre-order:
    // a focusable container precedes its children.
    if (widget != this && widget->isVisible()
        && (widget->focusPolicy() & Qt::TabFocus)
        && widget->isEffectivelyEnabled()) {
        chain.append(widget);
    }
    for (auto *child : widget->childWidgets()) {
        if (child->isVisible())
            collectFocusableWidgets(child, chain);
    }
}

QTuiWidget *QTuiMainWindow::findFocusedWidget(QTuiWidget *root)
{
    if (root->hasFocus())
        return root;
    for (auto *child : root->childWidgets()) {
        QTuiWidget *found = findFocusedWidget(child);
        if (found)
            return found;
    }
    return nullptr;
}

QTuiWidget *QTuiMainWindow::findWidgetAt(QTuiWidget *root, const QPoint &pos)
{
    // Depth-first: find the deepest visible child whose geometry contains pos
    QTuiWidget *best = nullptr;
    if (root->isVisible() && root->geometry().contains(pos))
        best = root;

    for (auto *child : root->childWidgets()) {
        // A child painted by its parent has no screen geometry to hit
        if (!child->isVisible() || child->isPaintedByParent())
            continue;
        QTuiWidget *found = findWidgetAt(child, pos);
        if (found)
            best = found;
    }
    return best;
}

} // namespace QtTUI
