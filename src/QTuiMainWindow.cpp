#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/QTuiStatusBar.h>
#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
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

    if (m_centralWidget)
        m_centralWidget->setParent(nullptr);

    m_centralWidget = widget;

    if (m_centralWidget) {
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

    m_layout = layout;
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

    if (m_menuBar)
        m_menuBar->setParent(nullptr);

    m_menuBar = menuBar;

    if (m_menuBar)
        m_menuBar->setParent(this);

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

    rebuildFocusChain();
    if (!geometry().isEmpty())
        relayout();
}

void QTuiMainWindow::removeToolBar(QTuiToolBar *toolBar)
{
    if (!toolBar || !m_toolBars.contains(toolBar))
        return;

    m_toolBars.removeOne(toolBar);
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

    if (m_statusBar)
        m_statusBar->setParent(nullptr);

    m_statusBar = statusBar;

    if (m_statusBar)
        m_statusBar->setParent(this);

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

void QTuiMainWindow::resizeEvent(TuiResizeEvent *event)
{
    setGeometry(QRect(0, 0, event->size().width(), event->size().height()));
    relayout();
}

void QTuiMainWindow::keyPressEvent(TuiKeyEvent *event)
{
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

    // Find the focused widget anywhere in the tree and dispatch to it
    QTuiWidget *focused = findFocusedWidget(this);
    if (focused && focused != this) {
        QCoreApplication::sendEvent(focused, event);
    }
}

void QTuiMainWindow::mousePressEvent(TuiMouseEvent *event)
{
    QTuiWidget *target = findWidgetAt(this, event->pos());
    if (target && target != this) {
        target->setFocus();
        QCoreApplication::sendEvent(target, event);
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

    if (m_layout) {
        m_layout->apply(geometry());
    } else if (m_centralWidget) {
        m_centralWidget->setGeometry(geometry());
    }

    for (auto *child : childWidgets()) {
        if (m_layout)
            break;
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

void QTuiMainWindow::advanceFocus(bool forward)
{
    rebuildFocusChain();

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
}

void QTuiMainWindow::rebuildFocusChain()
{
    m_focusChain.clear();
    collectFocusableWidgets(this, m_focusChain);
}

void QTuiMainWindow::collectFocusableWidgets(QTuiWidget *widget, QList<QTuiWidget *> &chain)
{
    auto children = widget->childWidgets();
    bool hasVisibleChild = false;
    for (auto *child : children) {
        if (child->isVisible()) {
            hasVisibleChild = true;
            collectFocusableWidgets(child, chain);
        }
    }
    // Leaf visible widgets (no visible children) are focusable
    if (!hasVisibleChild && widget != this && widget->isVisible()) {
        chain.append(widget);
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
        if (!child->isVisible())
            continue;
        QTuiWidget *found = findWidgetAt(child, pos);
        if (found)
            best = found;
    }
    return best;
}

} // namespace QtTUI
