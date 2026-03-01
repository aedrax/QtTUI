#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/QTuiLayout.h>
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

    // Detach previous central widget (reparent away, don't delete)
    if (m_centralWidget) {
        m_centralWidget->setParent(nullptr);
    }

    m_centralWidget = widget;

    if (m_centralWidget) {
        m_centralWidget->setParent(this);
        // Fill available area
        m_centralWidget->setGeometry(geometry());
    }

    rebuildFocusChain();
}

QTuiWidget *QTuiMainWindow::centralWidget() const
{
    return m_centralWidget;
}

void QTuiMainWindow::setLayout(QTuiLayout *layout)
{
    if (m_layout == layout)
        return;

    // Take ownership of the new layout
    if (layout && layout->parent() != this)
        layout->setParent(this);

    m_layout = layout;
    // Only relayout if we already have a valid geometry (e.g. after show())
    if (!geometry().isEmpty())
        relayoutChildren();
}

QTuiLayout *QTuiMainWindow::layout() const
{
    return m_layout;
}

void QTuiMainWindow::show()
{
    setVisible(true);
    // Initialize geometry from terminal size if not yet set
    if (geometry().isEmpty()) {
        auto *app = QTuiApplication::instance();
        if (app && app->backend()) {
            QSize sz = app->backend()->terminalSize();
            setGeometry(QRect(0, 0, sz.width(), sz.height()));
        }
    }
    // Trigger initial layout and paint
    relayoutChildren();
}

void QTuiMainWindow::paintEvent(ScreenBuffer &buffer)
{
    // Delegate painting to all visible children
    for (auto *child : childWidgets()) {
        if (child->isVisible()) {
            child->paintEvent(buffer);
        }
    }
}

void QTuiMainWindow::resizeEvent(TuiResizeEvent *event)
{
    setGeometry(QRect(0, 0, event->size().width(), event->size().height()));
    relayoutChildren();
}

void QTuiMainWindow::keyPressEvent(TuiKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab) {
        if (event->modifiers() & Qt::ShiftModifier) {
            advanceFocus(false);
        } else {
            advanceFocus(true);
        }
        return;
    }

    // Forward to focused child if any
    for (auto *child : childWidgets()) {
        if (child->hasFocus()) {
            QCoreApplication::sendEvent(child, event);
            return;
        }
    }
}

void QTuiMainWindow::relayoutChildren()
{
    if (m_layout) {
        // Delegate to the layout manager
        m_layout->apply(geometry());
    } else if (m_centralWidget) {
        // No layout, central widget fills the entire area
        m_centralWidget->setGeometry(geometry());
    }

    // Clamp any non-layout-managed children to within bounds
    for (auto *child : childWidgets()) {
        if (m_layout)
            break; // Layout handles all managed widgets
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
        // No widget has focus, start at first (forward) or last (backward)
        nextIdx = forward ? 0 : n - 1;
    } else {
        nextIdx = forward ? (currentIdx + 1) % n : (currentIdx - 1 + n) % n;
    }

    m_focusChain[nextIdx]->setFocus();
}

void QTuiMainWindow::rebuildFocusChain()
{
    m_focusChain.clear();
    for (auto *child : childWidgets()) {
        if (child->isVisible()) {
            m_focusChain.append(child);
        }
    }
}

} // namespace QtTUI
