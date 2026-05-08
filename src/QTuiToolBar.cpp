#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiToolBar::QTuiToolBar(const QString &title, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_title(title)
{
}

void QTuiToolBar::addAction(QTuiAction *action)
{
    if (!action)
        return;
    ToolBarItem item;
    item.action = action;
    m_items.append(item);
}

void QTuiToolBar::addSeparator()
{
    ToolBarItem item;
    item.isSeparator = true;
    m_items.append(item);
}

void QTuiToolBar::addWidget(QTuiWidget *widget)
{
    if (!widget)
        return;
    ToolBarItem item;
    item.widget = widget;
    m_items.append(item);
}

bool QTuiToolBar::isMovable() const
{
    return m_movable;
}

void QTuiToolBar::setMovable(bool movable)
{
    m_movable = movable;
}

QString QTuiToolBar::title() const
{
    return m_title;
}

QSize QTuiToolBar::minimumSize() const
{
    int width = 0;
    for (const auto &item : m_items) {
        if (item.isSeparator) {
            width += 3; // " | "
        } else if (item.action) {
            width += item.action->text().length() + 2; // padding
        } else if (item.widget) {
            width += item.widget->minimumSize().width();
        }
    }
    return QSize(qMax(width, 1), 1);
}

void QTuiToolBar::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    Color fg = theme.foreground;
    Color bg = theme.background;

    // Clear the bar row
    for (int col = 0; col < geo.width() && (geo.x() + col) < buffer.width(); ++col) {
        int px = geo.x() + col;
        int py = geo.y();
        if (py < buffer.height()) {
            Cell &cell = buffer.backCell(px, py);
            cell.character = QLatin1Char(' ');
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;
        }
    }

    int xPos = geo.x();
    int py = geo.y();
    if (py >= buffer.height())
        return;

    int navigableIndex = 0; // tracks which navigable item we're on

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &item = m_items.at(i);

        if (item.isSeparator) {
            // Render separator as " | "
            const QString sep = QStringLiteral(" | ");
            for (int c = 0; c < sep.length(); ++c) {
                if (xPos < buffer.width()) {
                    Cell &cell = buffer.backCell(xPos, py);
                    cell.character = sep.at(c);
                    cell.foreground = fg;
                    cell.background = bg;
                }
                xPos++;
            }
        } else if (item.action) {
            bool isFocused = hasFocus() && (navigableIndex == m_focusedItem);
            Color itemFg = isFocused ? bg : fg;
            Color itemBg = isFocused ? fg : bg;

            // Leading space
            if (xPos < buffer.width()) {
                Cell &cell = buffer.backCell(xPos, py);
                cell.character = QLatin1Char(' ');
                cell.foreground = itemFg;
                cell.background = itemBg;
            }
            xPos++;

            // Action text
            const QString text = item.action->text();
            for (int c = 0; c < text.length(); ++c) {
                if (xPos < buffer.width()) {
                    Cell &cell = buffer.backCell(xPos, py);
                    cell.character = text.at(c);
                    cell.foreground = itemFg;
                    cell.background = itemBg;
                    cell.bold = isFocused;
                }
                xPos++;
            }

            // Trailing space
            if (xPos < buffer.width()) {
                Cell &cell = buffer.backCell(xPos, py);
                cell.character = QLatin1Char(' ');
                cell.foreground = itemFg;
                cell.background = itemBg;
            }
            xPos++;

            navigableIndex++;
        } else if (item.widget) {
            int w = item.widget->minimumSize().width();
            // Position the embedded widget at the current location
            item.widget->setGeometry(QRect(xPos, py, w, 1));
            xPos += w;
            navigableIndex++;
        }
    }
}

void QTuiToolBar::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();

    // Count navigable items (non-separator)
    int navigableCount = 0;
    for (const auto &item : m_items) {
        if (!item.isSeparator)
            navigableCount++;
    }

    if (navigableCount == 0)
        return;

    if (key == Qt::Key_Left) {
        m_focusedItem--;
        if (m_focusedItem < 0)
            m_focusedItem = navigableCount - 1;
    } else if (key == Qt::Key_Right) {
        m_focusedItem++;
        if (m_focusedItem >= navigableCount)
            m_focusedItem = 0;
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        // Trigger the focused action
        int navIdx = 0;
        for (const auto &item : m_items) {
            if (item.isSeparator)
                continue;
            if (navIdx == m_focusedItem) {
                if (item.action)
                    item.action->trigger();
                break;
            }
            navIdx++;
        }
    }
}

void QTuiToolBar::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    int clickX = event->x() - geo.x();
    if (clickX < 0)
        return;

    // Walk through items accumulating width to find which was clicked
    int xPos = 0;
    int navigableIndex = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        const auto &item = m_items.at(i);
        int itemWidth = 0;

        if (item.isSeparator) {
            itemWidth = 3; // " | "
        } else if (item.action) {
            itemWidth = item.action->text().length() + 2; // padding
        } else if (item.widget) {
            itemWidth = item.widget->minimumSize().width();
        }

        if (clickX >= xPos && clickX < xPos + itemWidth) {
            if (!item.isSeparator) {
                m_focusedItem = navigableIndex;
                if (item.action)
                    item.action->trigger();
            }
            return;
        }

        xPos += itemWidth;
        if (!item.isSeparator)
            navigableIndex++;
    }
}

} // namespace QtTUI
