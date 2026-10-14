#include <QtTUI/QTuiUndoView.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiUndoView::QTuiUndoView(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

QTuiUndoView::QTuiUndoView(QTuiUndoStack *stack, QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setStack(stack);
}

void QTuiUndoView::setStack(QTuiUndoStack *stack)
{
    if (m_stack == stack)
        return;

    if (m_stack) {
        disconnect(m_stack, &QTuiUndoStack::indexChanged,
                   this, &QTuiUndoView::onStackChanged);
        disconnect(m_stack, &QObject::destroyed, this, nullptr);
    }

    m_stack = stack;
    m_currentIndex = 0;
    m_scrollOffset = 0;

    if (m_stack) {
        connect(m_stack, &QTuiUndoStack::indexChanged,
                this, &QTuiUndoView::onStackChanged);
        // Guard against the stack being deleted while still set on the view
        connect(m_stack, &QObject::destroyed, this, [this]() {
            m_stack = nullptr;
            m_currentIndex = 0;
            m_scrollOffset = 0;
            markDirty();
        });
        m_currentIndex = m_stack->index();
    }
    markDirty();
}

QTuiUndoStack *QTuiUndoView::stack() const
{
    return m_stack;
}

void QTuiUndoView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0)
        return;

    const auto theme = themeProperties();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();
    const int rows = rowCount();
    const int stackIndex = m_stack ? m_stack->index() : 0;

    for (int vr = 0; vr < viewHeight; ++vr) {
        int itemRow = m_scrollOffset + vr;
        int screenY = geo.y() + vr;
        bool isCurrent = (itemRow == m_currentIndex && itemRow < rows);

        if (itemRow < rows) {
            // Row 0 is the state before the first command, as in QUndoView
            bool isActive = (itemRow <= stackIndex);

            QString text = itemRow == 0 ? QStringLiteral("<empty>") : m_stack->text(itemRow - 1);

            Color fg, bg;
            if (isCurrent) {
                fg = theme.focusForeground;
                bg = theme.focusBackground;
            } else if (isActive) {
                fg = theme.foreground;
                bg = theme.background;
            } else {
                fg = theme.disabledForeground;
                bg = theme.disabledBackground;
            }

            Cell style = cellStyle(fg, bg);
            style.bold = isCurrent;
            style.inverse = isCurrent;
            fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1), style);
            renderText(buffer, geo.x(), screenY, text, viewWidth, style);
        } else {
            fillRect(buffer, QRect(geo.x(), screenY, viewWidth, 1),
                     cellStyle(theme.foreground, theme.background));
        }
    }
}

void QTuiUndoView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_stack)
        return;

    switch (event->key()) {
    case Qt::Key_Up:
        if (m_currentIndex > 0) {
            --m_currentIndex;
            scrollToVisible();
            markDirty();
        }
        break;

    case Qt::Key_Down:
        if (m_currentIndex < rowCount() - 1) {
            ++m_currentIndex;
            scrollToVisible();
            markDirty();
        }
        break;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_stack->setIndex(m_currentIndex);
        break;

    default:
        break;
    }
}

void QTuiUndoView::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    if (!m_stack)
        return;

    const QRect geo = geometry();
    int clickedRow = (event->y() - geo.y()) + m_scrollOffset;
    if (clickedRow >= 0 && clickedRow < rowCount()) {
        m_currentIndex = clickedRow;
        m_stack->setIndex(m_currentIndex);
        scrollToVisible();
        markDirty();
    }
}

void QTuiUndoView::onStackChanged()
{
    if (!m_stack)
        return;

    // The current row follows the index of the stack
    m_currentIndex = m_stack->index();
    scrollToVisible();
    markDirty();
}

int QTuiUndoView::rowCount() const
{
    return m_stack ? m_stack->count() + 1 : 0;
}

void QTuiUndoView::scrollToVisible()
{
    int visibleRows = geometry().height();
    if (visibleRows <= 0)
        return;

    if (m_currentIndex < m_scrollOffset)
        m_scrollOffset = m_currentIndex;
    else if (m_currentIndex >= m_scrollOffset + visibleRows)
        m_scrollOffset = m_currentIndex - visibleRows + 1;
}

QRect QTuiUndoView::hoverRect() const
{
    // Only the row under the mouse, if it has a command
    const QRect geo = geometry();
    const int row = hoverPos().y() - geo.y() + m_scrollOffset;
    if (row < 0 || row >= rowCount())
        return QRect();
    return QRect(geo.x(), hoverPos().y(), geo.width(), 1);
}

} // namespace QtTUI
