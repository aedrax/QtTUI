#include <QtTUI/QTuiUndoView.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiUndoView::QTuiUndoView(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

QTuiUndoView::QTuiUndoView(QTuiUndoStack *stack, QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setStack(stack);
}

void QTuiUndoView::setStack(QTuiUndoStack *stack)
{
    if (m_stack == stack)
        return;

    if (m_stack) {
        disconnect(m_stack, &QTuiUndoStack::indexChanged,
                   this, &QTuiUndoView::onStackChanged);
    }

    m_stack = stack;
    m_currentIndex = 0;
    m_scrollOffset = 0;

    if (m_stack) {
        connect(m_stack, &QTuiUndoStack::indexChanged,
                this, &QTuiUndoView::onStackChanged);
        if (m_stack->index() > 0)
            m_currentIndex = m_stack->index() - 1;
        else
            m_currentIndex = 0;
    }
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
    const int commandCount = m_stack ? m_stack->count() : 0;
    const int stackIndex = m_stack ? m_stack->index() : 0;

    for (int vr = 0; vr < viewHeight; ++vr) {
        int itemRow = m_scrollOffset + vr;
        int screenY = geo.y() + vr;
        bool isCurrent = (itemRow == m_currentIndex && itemRow < commandCount);

        if (itemRow < commandCount) {
            bool isActive = (itemRow < stackIndex);

            QString text = m_stack->text(itemRow);
            if (text.isEmpty())
                text = QStringLiteral("<empty>");

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

            for (int x = 0; x < viewWidth; ++x) {
                Cell &cell = buffer.backCell(geo.x() + x, screenY);
                cell.character = (x < text.length()) ? text.at(x) : QLatin1Char(' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = isCurrent;
                cell.italic = false;
                cell.underline = false;
                cell.inverse = isCurrent;
            }
        } else {
            for (int x = 0; x < viewWidth; ++x) {
                Cell &cell = buffer.backCell(geo.x() + x, screenY);
                cell.character = QLatin1Char(' ');
                cell.foreground = theme.foreground;
                cell.background = theme.background;
                cell.bold = false;
                cell.italic = false;
                cell.underline = false;
                cell.inverse = false;
            }
        }
    }
}

void QTuiUndoView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_stack || m_stack->count() == 0)
        return;

    const int commandCount = m_stack->count();

    switch (event->key()) {
    case Qt::Key_Up:
        if (m_currentIndex > 0) {
            --m_currentIndex;
            scrollToVisible();
        }
        break;

    case Qt::Key_Down:
        if (m_currentIndex < commandCount - 1) {
            ++m_currentIndex;
            scrollToVisible();
        }
        break;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_stack->setIndex(m_currentIndex + 1);
        break;

    default:
        break;
    }
}

void QTuiUndoView::onStackChanged()
{
    if (!m_stack)
        return;

    const int commandCount = m_stack->count();
    if (m_currentIndex >= commandCount)
        m_currentIndex = qMax(0, commandCount - 1);

    scrollToVisible();
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

} // namespace QtTUI
