#include <QtTUI/QTuiCollapsible.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiCollapsible::QTuiCollapsible(const QString &title, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_title(title)
{
    setFocusPolicy(Qt::StrongFocus);
}

QString QTuiCollapsible::title() const
{
    return m_title;
}

void QTuiCollapsible::setTitle(const QString &title)
{
    if (m_title == title)
        return;
    m_title = title;
    markDirty();
}

bool QTuiCollapsible::isCollapsed() const
{
    return m_collapsed;
}

void QTuiCollapsible::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed)
        return;
    m_collapsed = collapsed;
    updateChildrenVisibility();
    emit toggled(!m_collapsed); // signal emits expanded state
    markDirty();
}

bool QTuiCollapsible::isExpanded() const
{
    return !m_collapsed;
}

void QTuiCollapsible::setExpanded(bool expanded)
{
    setCollapsed(!expanded);
}

QSize QTuiCollapsible::minimumSize() const
{
    if (m_collapsed) {
        // Title bar only: indicator + space + title
        int titleWidth = 2 + m_title.length(); // "▶ " or "▼ " prefix
        return QSize(qMax(4, titleWidth), 1);
    }
    // Expanded: title bar + the children stacked in rows (at least 1 row)
    int width = qMax(4, 2 + int(m_title.length()));
    int contentHeight = 0;
    for (auto *child : childWidgets()) {
        const QSize ms = child->minimumSize();
        width = qMax(width, ms.width());
        contentHeight += ms.height();
    }
    return QSize(width, 1 + qMax(1, contentHeight));
}

void QTuiCollapsible::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.foreground;
    const Color bg = theme.background;
    const Color focusFg = hasFocus() ? theme.focusForeground : fg;
    const Color focusBg = hasFocus() ? theme.focusBackground : bg;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();

    // Draw title bar
    // Indicator: ▼ (expanded) or ▶ (collapsed)
    QString indicator = m_collapsed ? QStringLiteral("\u25B6 ") : QStringLiteral("\u25BC ");
    QString titleLine = indicator + m_title;

    if (titleLine.length() > w)
        titleLine = titleLine.left(w);

    // Render title bar row
    for (int col = 0; col < w; ++col) {
        int px = x0 + col;
        if (px >= buffer.width() || y0 >= buffer.height())
            continue;
        Cell &cell = buffer.backCell(px, y0);
        if (col < titleLine.length())
            cell.setChar(titleLine.at(col).unicode());
        else
            cell.setChar(U' ');
        cell.foreground = focusFg;
        cell.background = focusBg;
        cell.bold = theme.bold;
    }

    if (m_collapsed) {
        // When collapsed, hide children - nothing more to render
        return;
    }

    // When expanded, clear the content area below the title bar
    int h = geo.height();
    for (int row = 1; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int px = x0 + col;
            int py = y0 + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.setChar(U' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
            }
        }
    }

    // Stack the visible children in rows below the title bar, each at its
    // minimum height; the last child gets the remaining rows
    const auto children = childWidgets();
    int y = y0 + 1;
    const int bottom = y0 + h;
    for (int i = 0; i < children.size(); ++i) {
        QTuiWidget *child = children.at(i);
        if (!child->isVisible())
            continue;
        const bool last = (i == children.size() - 1);
        const int rows = last ? bottom - y
                              : qMin(child->minimumSize().height(), bottom - y);
        child->setGeometry(QRect(x0, y, w, qMax(0, rows)));
        y += qMax(0, rows);
    }

    for (auto *child : children) {
        if (child->isVisible())
            child->paintEvent(buffer);
    }
}

void QTuiCollapsible::keyPressEvent(TuiKeyEvent *event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
        setCollapsed(!m_collapsed);
        return;
    }
    QTuiWidget::keyPressEvent(event);
}

void QTuiCollapsible::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    // Only clicks on the header row toggle collapse. Mirror paintEvent: the
    // title bar occupies the entire top row (row geo.y(), full width).
    if (event->y() != geometry().y())
        return;

    setCollapsed(!m_collapsed);
}

void QTuiCollapsible::updateChildrenVisibility()
{
    const auto children = childWidgets();
    for (auto *child : children) {
        child->setVisible(!m_collapsed);
    }
}

} // namespace QtTUI
