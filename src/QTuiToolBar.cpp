#include <QtTUI/QTuiToolBar.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/QTuiAction.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiToolBar::QTuiToolBar(const QString &title, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_title(title)
{
    setFocusPolicy(Qt::StrongFocus);
}

void QTuiToolBar::addAction(QTuiAction *action)
{
    if (!action)
        return;
    ToolBarItem item;
    item.action = action;
    m_items.append(item);
    markDirty();
}

void QTuiToolBar::addSeparator()
{
    ToolBarItem item;
    item.isSeparator = true;
    m_items.append(item);
    markDirty();
}

void QTuiToolBar::addWidget(QTuiWidget *widget)
{
    if (!widget)
        return;
    ToolBarItem item;
    item.widget = widget;
    m_items.append(item);
    markDirty();
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
            width += QtTuiUnicode::stringWidth(item.action->text()) + 2; // padding
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
    fillRect(buffer, QRect(geo.x(), geo.y(), geo.width(), 1), cellStyle(fg, bg));

    int xPos = geo.x();
    int py = geo.y();
    if (py >= buffer.height())
        return;

    int navigableIndex = 0; // tracks which navigable item we're on

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &item = m_items.at(i);

        if (item.isSeparator) {
            // Render separator as " | "
            xPos += renderText(buffer, xPos, py, QStringLiteral(" | "), 3, cellStyle(fg, bg));
        } else if (item.action) {
            // The action has a space on each side; only the text of the focused action is bold
            const bool isFocused = hasFocus() && (navigableIndex == m_focusedItem);
            const Cell spaceStyle = isFocused ? cellStyle(bg, fg) : cellStyle(fg, bg);
            Cell textStyle = spaceStyle;
            textStyle.bold = isFocused;
            const QString text = item.action->text();
            const int textWidth = QtTuiUnicode::stringWidth(text);
            fillRect(buffer, QRect(xPos, py, textWidth + 2, 1), spaceStyle);
            renderText(buffer, xPos + 1, py, text, textWidth, textStyle);
            xPos += textWidth + 2;

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
        markDirty();
    } else if (key == Qt::Key_Right) {
        m_focusedItem++;
        if (m_focusedItem >= navigableCount)
            m_focusedItem = 0;
        markDirty();
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
            itemWidth = QtTuiUnicode::stringWidth(item.action->text()) + 2; // padding
        } else if (item.widget) {
            itemWidth = item.widget->minimumSize().width();
        }

        if (clickX >= xPos && clickX < xPos + itemWidth) {
            if (!item.isSeparator) {
                m_focusedItem = navigableIndex;
                markDirty();
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
