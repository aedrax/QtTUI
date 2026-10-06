#include <QtTUI/QTuiMenuBar.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/QTuiMenu.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiMenuBar::QTuiMenuBar(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

QTuiMenu *QTuiMenuBar::addMenu(const QString &title)
{
    auto *menu = new QTuiMenu(title, this);
    registerMenu(menu);
    return menu;
}

void QTuiMenuBar::addMenu(QTuiMenu *menu)
{
    if (!menu)
        return;
    registerMenu(menu);
}

void QTuiMenuBar::registerMenu(QTuiMenu *menu)
{
    m_menus.append(menu);
    if (m_activeMenuIndex < 0 && !m_menus.isEmpty())
        m_activeMenuIndex = 0;

    // Keep m_menuOpen in sync when the menu closes itself (Escape inside the
    // menu, action trigger, outside click) instead of through closeCurrentMenu()
    connect(menu, &QTuiWidget::visibilityChanged, this, [this, menu](bool visible) {
        if (!visible && m_menuOpen && m_activeMenuIndex >= 0
            && m_activeMenuIndex < m_menus.size()
            && m_menus.at(m_activeMenuIndex) == menu) {
            m_menuOpen = false;
            markDirty();
        }
    });

    markDirty();
}

QList<QTuiMenu *> QTuiMenuBar::menus() const
{
    return m_menus;
}

void QTuiMenuBar::openMenu(int index)
{
    if (index < 0 || index >= m_menus.size())
        return;

    closeCurrentMenu();
    m_activeMenuIndex = index;
    m_menuOpen = true;

    // Calculate position below the menu title. Widths must match paintEvent(),
    // which strips '&' mnemonic markers from the painted titles.
    QRect geo = geometry();
    int xOffset = 0;
    for (int i = 0; i < index; ++i) {
        QString displayTitle = m_menus.at(i)->title();
        displayTitle.remove(QLatin1Char('&'));
        xOffset += QtTuiUnicode::stringWidth(displayTitle) + 2; // title + padding
    }

    QPoint pos(geo.x() + xOffset, geo.y() + 1);
    m_menus.at(index)->popup(pos);
    markDirty();
}

void QTuiMenuBar::closeCurrentMenu()
{
    if (m_menuOpen && m_activeMenuIndex >= 0 && m_activeMenuIndex < m_menus.size()) {
        m_menus.at(m_activeMenuIndex)->close();
        markDirty();
    }
    m_menuOpen = false;
}

bool QTuiMenuBar::triggerMnemonic(QChar ch)
{
    const int idx = findMnemonicIndex(ch);
    if (idx < 0)
        return false;
    m_activeMenuIndex = idx;
    openMenu(idx);
    return true;
}

bool QTuiMenuBar::openFirstMenu()
{
    if (m_menus.isEmpty())
        return false;
    m_activeMenuIndex = 0;
    openMenu(0);
    return true;
}

int QTuiMenuBar::findMnemonicIndex(QChar ch) const
{
    QChar upper = ch.toUpper();
    for (int i = 0; i < m_menus.size(); ++i) {
        const QString &title = m_menus.at(i)->title();
        // Look for &X pattern (mnemonic)
        int ampIdx = title.indexOf(QLatin1Char('&'));
        if (ampIdx >= 0 && ampIdx + 1 < title.length()) {
            if (title.at(ampIdx + 1).toUpper() == upper)
                return i;
        }
        // Fallback: match first character
        if (!title.isEmpty() && title.at(0).toUpper() == upper)
            return i;
    }
    return -1;
}

QSize QTuiMenuBar::minimumSize() const
{
    int width = 0;
    for (const auto *menu : m_menus) {
        QString displayTitle = menu->title();
        displayTitle.remove(QLatin1Char('&'));
        width += QtTuiUnicode::stringWidth(displayTitle) + 2; // padding around each title
    }
    return QSize(qMax(width, 1), 1);
}

void QTuiMenuBar::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    Color fg = theme.foreground;
    Color bg = theme.background;

    // Clear the bar row
    fillRect(buffer, QRect(geo.x(), geo.y(), geo.width(), 1), cellStyle(fg, bg));

    // Render menu titles
    int xPos = geo.x();
    int py = geo.y();
    if (py >= buffer.height())
        return;

    for (int i = 0; i < m_menus.size(); ++i) {
        QString displayTitle = m_menus.at(i)->title();
        displayTitle.remove(QLatin1Char('&'));

        // The title has a space on each side; only the text of the active title is bold
        const bool isActive = (i == m_activeMenuIndex);
        const Cell spaceStyle = isActive ? cellStyle(bg, fg) : cellStyle(fg, bg);
        Cell textStyle = spaceStyle;
        textStyle.bold = isActive;
        const int textWidth = QtTuiUnicode::stringWidth(displayTitle);
        fillRect(buffer, QRect(xPos, py, textWidth + 2, 1), spaceStyle);
        renderText(buffer, xPos + 1, py, displayTitle, textWidth, textStyle);
        xPos += textWidth + 2;
    }
}

void QTuiMenuBar::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    const auto mods = event->modifiers();

    if (key == Qt::Key_Left) {
        if (m_menus.isEmpty())
            return;
        int newIndex = m_activeMenuIndex - 1;
        if (newIndex < 0)
            newIndex = m_menus.size() - 1;
        if (m_menuOpen) {
            closeCurrentMenu();
            m_activeMenuIndex = newIndex;
            openMenu(newIndex);
        } else {
            m_activeMenuIndex = newIndex;
            markDirty();
        }
    } else if (key == Qt::Key_Right) {
        if (m_menus.isEmpty())
            return;
        int newIndex = m_activeMenuIndex + 1;
        if (newIndex >= m_menus.size())
            newIndex = 0;
        if (m_menuOpen) {
            closeCurrentMenu();
            m_activeMenuIndex = newIndex;
            openMenu(newIndex);
        } else {
            m_activeMenuIndex = newIndex;
            markDirty();
        }
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Down) {
        if (!m_menuOpen && m_activeMenuIndex >= 0) {
            openMenu(m_activeMenuIndex);
        }
    } else if (key == Qt::Key_Escape) {
        if (m_menuOpen) {
            closeCurrentMenu();
        }
    } else if (mods & Qt::AltModifier) {
        // Alt+letter mnemonic activation
        QString text = event->text();
        if (!text.isEmpty())
            triggerMnemonic(text.at(0));
    }
}

int QTuiMenuBar::menuAt(int x, int *start, int *width) const
{
    // Walk through menus accumulating widths, as paintEvent() does
    const int col = x - geometry().x();
    int xPos = 0;
    for (int i = 0; i < m_menus.size(); ++i) {
        QString displayTitle = m_menus.at(i)->title();
        displayTitle.remove(QLatin1Char('&'));
        const int itemWidth = QtTuiUnicode::stringWidth(displayTitle) + 2; // leading + trailing space
        if (col >= xPos && col < xPos + itemWidth) {
            if (start)
                *start = geometry().x() + xPos;
            if (width)
                *width = itemWidth;
            return i;
        }
        xPos += itemWidth;
    }
    return -1;
}

QRect QTuiMenuBar::hoverRect() const
{
    // Only the menu title under the mouse
    int start = 0;
    int width = 0;
    if (menuAt(hoverPos().x(), &start, &width) < 0)
        return QRect();
    return QRect(start, geometry().y(), width, 1);
}

void QTuiMenuBar::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    const int i = menuAt(event->x());
    if (i >= 0) {
        m_activeMenuIndex = i;
        openMenu(i);
    }
}

} // namespace QtTUI
