#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiMenu::QTuiMenu(const QString &title, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_title(title)
{
    setFocusPolicy(Qt::StrongFocus);
    // A menu is always a popup: it renders in the topmost pass, clipped only
    // to the screen, so it can escape its owner's rect (e.g. a 1-row menu bar)
    setPopup(true);
    setVisible(false);
}

QString QTuiMenu::title() const
{
    return m_title;
}

void QTuiMenu::setTitle(const QString &title)
{
    m_title = title;
}

QTuiAction *QTuiMenu::addAction(const QString &text)
{
    auto *action = new QTuiAction(text, this);
    MenuItem item;
    item.action = action;
    m_menuItems.append(item);
    return action;
}

QTuiAction *QTuiMenu::addAction(const QString &text, const QString &shortcut)
{
    auto *action = new QTuiAction(text, shortcut, this);
    MenuItem item;
    item.action = action;
    m_menuItems.append(item);
    return action;
}

void QTuiMenu::addAction(QTuiAction *action)
{
    if (!action)
        return;
    MenuItem item;
    item.action = action;
    m_menuItems.append(item);
    // Drop the item if the action goes away first
    connect(action, &QObject::destroyed, this, [this, action]() {
        for (int i = 0; i < m_menuItems.size(); ++i) {
            if (m_menuItems.at(i).action == action) {
                m_menuItems.removeAt(i);
                break;
            }
        }
        m_highlightedIndex = qBound(0, m_highlightedIndex, qMax(0, int(m_menuItems.size()) - 1));
        markDirty();
    });
}

void QTuiMenu::addSeparator()
{
    MenuItem item;
    item.isSeparator = true;
    m_menuItems.append(item);
}

QTuiMenu *QTuiMenu::addMenu(const QString &title)
{
    auto *submenu = new QTuiMenu(title, this);
    MenuItem item;
    item.action = new QTuiAction(title, this);
    item.submenu = submenu;
    m_menuItems.append(item);
    return submenu;
}

void QTuiMenu::addMenu(QTuiMenu *menu)
{
    if (!menu)
        return;
    MenuItem item;
    item.action = new QTuiAction(menu->title(), this);
    item.submenu = menu;
    m_menuItems.append(item);
}

void QTuiMenu::popup(const QPoint &pos)
{
    m_highlightedIndex = 0;
    // Skip initial separators
    while (m_highlightedIndex < m_menuItems.size() && m_menuItems.at(m_highlightedIndex).isSeparator)
        m_highlightedIndex++;

    QRect geo(pos, minimumSize());
    if (auto *app = QTuiApplication::instance(); app && app->mainWindow()) {
        const QRect screen = app->mainWindow()->geometry();
        geo.moveLeft(qBound(screen.left(), geo.left(), screen.right() + 1 - geo.width()));
        geo.moveTop(qBound(screen.top(), geo.top(), screen.bottom() + 1 - geo.height()));
    }
    setGeometry(geo);
    setVisible(true);
    setFocus();
    emit aboutToShow();
}

void QTuiMenu::close()
{
    if (isVisible()) {
        // Close any open submenu first so its screen region is restored too
        for (const auto &item : m_menuItems) {
            if (item.submenu && item.submenu->isVisible())
                item.submenu->close();
        }
        emit aboutToHide();
        setVisible(false);
    }
}

void QTuiMenu::popupOutsideClick()
{
    close();
}

QList<QTuiAction *> QTuiMenu::actions() const
{
    QList<QTuiAction *> result;
    for (const auto &item : m_menuItems) {
        if (item.action)
            result.append(item.action);
    }
    return result;
}

void QTuiMenu::navigateToNextItem()
{
    if (m_menuItems.isEmpty())
        return;
    int start = m_highlightedIndex;
    do {
        m_highlightedIndex = (m_highlightedIndex + 1) % m_menuItems.size();
    } while (m_menuItems.at(m_highlightedIndex).isSeparator && m_highlightedIndex != start);
    if (m_highlightedIndex != start)
        markDirty();
}

void QTuiMenu::navigateToPrevItem()
{
    if (m_menuItems.isEmpty())
        return;
    int start = m_highlightedIndex;
    do {
        m_highlightedIndex = (m_highlightedIndex - 1 + m_menuItems.size()) % m_menuItems.size();
    } while (m_menuItems.at(m_highlightedIndex).isSeparator && m_highlightedIndex != start);
    if (m_highlightedIndex != start)
        markDirty();
}

QSize QTuiMenu::minimumSize() const
{
    int maxWidth = 4; // minimum
    for (const auto &item : m_menuItems) {
        if (item.isSeparator)
            continue;
        int w = 0;
        if (item.action) {
            QString display = item.action->text();
            display.remove(QLatin1Char('&'));
            w += display.length();
        }
        if (item.action && !item.action->shortcut().isEmpty())
            w += 2 + item.action->shortcut().length();
        if (item.submenu)
            w += 2; // " >"
        maxWidth = qMax(maxWidth, w);
    }
    // borders (2) + padding (2)
    int width = maxWidth + 4;
    int height = 2 + m_menuItems.size(); // top/bottom borders + items
    return QSize(width, height);
}

void QTuiMenu::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);

    const int gx = geo.x();
    const int gy = geo.y();
    const int gw = geo.width();
    const int gh = geo.height();

    Color fg = theme.foreground;
    Color bg = theme.background;

    // Clear area
    for (int row = 0; row < gh && (gy + row) < buffer.height(); ++row) {
        for (int col = 0; col < gw && (gx + col) < buffer.width(); ++col) {
            Cell &cell = buffer.backCell(gx + col, gy + row);
            cell.setChar(U' ');
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;
        }
    }

    // Top border
    if (gy < buffer.height()) {
        for (int col = 0; col < gw && (gx + col) < buffer.width(); ++col) {
            Cell &cell = buffer.backCell(gx + col, gy);
            if (col == 0)
                cell.setChar(border.topLeft);
            else if (col == gw - 1)
                cell.setChar(border.topRight);
            else
                cell.setChar(border.horizontal);
            cell.foreground = fg;
            cell.background = bg;
        }
    }

    // Bottom border
    int bottomY = gy + gh - 1;
    if (bottomY < buffer.height()) {
        for (int col = 0; col < gw && (gx + col) < buffer.width(); ++col) {
            Cell &cell = buffer.backCell(gx + col, bottomY);
            if (col == 0)
                cell.setChar(border.bottomLeft);
            else if (col == gw - 1)
                cell.setChar(border.bottomRight);
            else
                cell.setChar(border.horizontal);
            cell.foreground = fg;
            cell.background = bg;
        }
    }

    // Menu items
    int innerWidth = gw - 2;
    for (int i = 0; i < m_menuItems.size(); ++i) {
        int rowY = gy + 1 + i;
        if (rowY >= buffer.height() || rowY >= gy + gh - 1)
            break;

        // Left/right borders
        if (gx < buffer.width()) {
            buffer.backCell(gx, rowY).setChar(border.vertical);
            buffer.backCell(gx, rowY).foreground = fg;
            buffer.backCell(gx, rowY).background = bg;
        }
        int rx = gx + gw - 1;
        if (rx < buffer.width()) {
            buffer.backCell(rx, rowY).setChar(border.vertical);
            buffer.backCell(rx, rowY).foreground = fg;
            buffer.backCell(rx, rowY).background = bg;
        }

        const auto &mi = m_menuItems.at(i);

        if (mi.isSeparator) {
            for (int col = 0; col < innerWidth; ++col) {
                int px = gx + 1 + col;
                if (px < buffer.width()) {
                    buffer.backCell(px, rowY).setChar(border.horizontal);
                    buffer.backCell(px, rowY).foreground = fg;
                    buffer.backCell(px, rowY).background = bg;
                }
            }
            continue;
        }

        bool highlighted = (i == m_highlightedIndex);
        Color itemFg = highlighted ? bg : fg;
        Color itemBg = highlighted ? fg : bg;

        QString text = mi.action ? mi.action->text() : QString();
        text.remove(QLatin1Char('&')); // mnemonic markers are not painted
        QString shortcutStr;
        if (mi.action && !mi.action->shortcut().isEmpty())
            shortcutStr = mi.action->shortcut();
        QString suffix;
        if (mi.submenu)
            suffix = QStringLiteral(" >");

        int rightLen = shortcutStr.length() + suffix.length();
        int textSpace = innerWidth - rightLen;
        if (textSpace < 0) textSpace = 0;
        QString truncText = text.left(textSpace);

        for (int col = 0; col < innerWidth; ++col) {
            int px = gx + 1 + col;
            if (px >= buffer.width())
                break;
            Cell &cell = buffer.backCell(px, rowY);
            cell.foreground = itemFg;
            cell.background = itemBg;
            cell.bold = highlighted;

            int rightStart = innerWidth - rightLen;
            if (col < truncText.length()) {
                cell.setChar(truncText.at(col).unicode());
            } else if (rightLen > 0 && col >= rightStart) {
                int ri = col - rightStart;
                QString rightStr = shortcutStr + suffix;
                if (ri < rightStr.length())
                    cell.setChar(rightStr.at(ri).unicode());
                else
                    cell.setChar(U' ');
            } else {
                cell.setChar(U' ');
            }
        }
    }
}

void QTuiMenu::keyPressEvent(TuiKeyEvent *event)
{
    // A closed menu can still receive events through direct dispatch; ignore them
    if (!isVisible())
        return;

    const int key = event->key();

    if (key == Qt::Key_Up) {
        navigateToPrevItem();
    } else if (key == Qt::Key_Down) {
        navigateToNextItem();
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (m_highlightedIndex >= 0 && m_highlightedIndex < m_menuItems.size()) {
            const auto &mi = m_menuItems.at(m_highlightedIndex);
            if (mi.submenu) {
                QRect geo = geometry();
                QPoint subPos(geo.x() + geo.width(), geo.y() + 1 + m_highlightedIndex);
                mi.submenu->popup(subPos);
            } else if (mi.action && !mi.isSeparator) {
                mi.action->trigger();
                close();
            }
        }
    } else if (key == Qt::Key_Escape) {
        close();
    } else if (key == Qt::Key_Right) {
        if (m_highlightedIndex >= 0 && m_highlightedIndex < m_menuItems.size()) {
            const auto &mi = m_menuItems.at(m_highlightedIndex);
            if (mi.submenu) {
                QRect geo = geometry();
                QPoint subPos(geo.x() + geo.width(), geo.y() + 1 + m_highlightedIndex);
                mi.submenu->popup(subPos);
            }
        }
    }
}

void QTuiMenu::mousePressEvent(TuiMouseEvent *event)
{
    if (!isVisible() || event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    // Menu items are rendered one per row starting at geo.y()+1 (first row is border)
    int itemIdx = event->y() - geo.y() - 1;
    if (itemIdx >= 0 && itemIdx < m_menuItems.size()) {
        const auto &mi = m_menuItems.at(itemIdx);
        if (!mi.isSeparator) {
            if (mi.submenu) {
                QPoint subPos(geo.x() + geo.width(), geo.y() + 1 + itemIdx);
                mi.submenu->popup(subPos);
            } else if (mi.action) {
                mi.action->trigger();
                close();
            }
        }
    }
}

} // namespace QtTUI
