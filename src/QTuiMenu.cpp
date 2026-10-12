#include <QtTUI/QTuiMenu.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/QTuiMainWindow.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/RenderUtils.h>
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
            w += QtTuiUnicode::stringWidth(display);
        }
        if (item.action && !item.action->shortcut().isEmpty())
            w += 2 + QtTuiUnicode::stringWidth(item.action->shortcut());
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

    fillRect(buffer, geo, cellStyle(fg, bg));
    drawBorder(buffer, geo, theme.borderStyle, cellStyle(fg, bg));

    // Menu items
    int innerWidth = gw - 2;
    for (int i = 0; i < m_menuItems.size(); ++i) {
        int rowY = gy + 1 + i;
        if (rowY >= buffer.height() || rowY >= gy + gh - 1)
            break;

        const auto &mi = m_menuItems.at(i);

        if (mi.isSeparator) {
            for (int col = 0; col < innerWidth && gx + 1 + col < buffer.width(); ++col)
                buffer.putChar(gx + 1 + col, rowY, border.horizontal, cellStyle(fg, bg));
            continue;
        }

        bool highlighted = (i == m_highlightedIndex);
        Cell itemStyle = highlighted ? cellStyle(bg, fg) : cellStyle(fg, bg);
        itemStyle.bold = highlighted;

        QString text = mi.action ? mi.action->text() : QString();
        text.remove(QLatin1Char('&')); // mnemonic markers are not painted
        QString rightStr;
        if (mi.action && !mi.action->shortcut().isEmpty())
            rightStr = mi.action->shortcut();
        if (mi.submenu)
            rightStr += QStringLiteral(" >");

        // The shortcut and the submenu marker are right-aligned; if they are
        // wider than the row, the row shows their end
        const int rightLen = qMin(QtTuiUnicode::stringWidth(rightStr), innerWidth);
        fillRect(buffer, QRect(gx + 1, rowY, innerWidth, 1), itemStyle);
        renderText(buffer, gx + 1, rowY, text, innerWidth - rightLen, itemStyle);
        renderText(buffer, gx + 1 + innerWidth - rightLen, rowY, rightStr.right(rightLen), rightLen, itemStyle);
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

QRect QTuiMenu::hoverRect() const
{
    // Only the item under the mouse, inside the border; not a separator
    const QRect geo = geometry();
    const int itemIdx = hoverPos().y() - geo.y() - 1;
    if (itemIdx < 0 || itemIdx >= m_menuItems.size() || m_menuItems.at(itemIdx).isSeparator)
        return QRect();
    return QRect(geo.x() + 1, hoverPos().y(), geo.width() - 2, 1);
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
