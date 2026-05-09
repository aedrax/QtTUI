#include <QtTUI/QTuiMenuBar.h>
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
        xOffset += displayTitle.length() + 2; // title + padding
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
        width += displayTitle.length() + 2; // padding around each title
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
    for (int col = 0; col < geo.width() && (geo.x() + col) < buffer.width(); ++col) {
        int px = geo.x() + col;
        int py = geo.y();
        if (py < buffer.height()) {
            Cell &cell = buffer.backCell(px, py);
            cell.setChar(U' ');
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = false;
            cell.italic = false;
            cell.underline = false;
        }
    }

    // Render menu titles
    int xPos = geo.x();
    int py = geo.y();
    if (py >= buffer.height())
        return;

    for (int i = 0; i < m_menus.size(); ++i) {
        QString displayTitle = m_menus.at(i)->title();
        displayTitle.remove(QLatin1Char('&'));

        bool isActive = (i == m_activeMenuIndex);
        Color titleFg = isActive ? bg : fg;
        Color titleBg = isActive ? fg : bg;

        // Leading space
        if (xPos < buffer.width()) {
            Cell &cell = buffer.backCell(xPos, py);
            cell.setChar(U' ');
            cell.foreground = titleFg;
            cell.background = titleBg;
        }
        xPos++;

        // Title text
        for (int c = 0; c < displayTitle.length(); ++c) {
            if (xPos < buffer.width()) {
                Cell &cell = buffer.backCell(xPos, py);
                cell.setChar(displayTitle.at(c).unicode());
                cell.foreground = titleFg;
                cell.background = titleBg;
                cell.bold = isActive;
            }
            xPos++;
        }

        // Trailing space
        if (xPos < buffer.width()) {
            Cell &cell = buffer.backCell(xPos, py);
            cell.setChar(U' ');
            cell.foreground = titleFg;
            cell.background = titleBg;
        }
        xPos++;
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

void QTuiMenuBar::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    if (m_menus.isEmpty())
        return;

    const QRect geo = geometry();
    int clickX = event->x() - geo.x();
    if (clickX < 0)
        return;

    // Walk through menus accumulating widths to find which was clicked
    int xPos = 0;
    for (int i = 0; i < m_menus.size(); ++i) {
        QString displayTitle = m_menus.at(i)->title();
        displayTitle.remove(QLatin1Char('&'));
        int itemWidth = displayTitle.length() + 2; // leading + trailing space

        if (clickX >= xPos && clickX < xPos + itemWidth) {
            m_activeMenuIndex = i;
            openMenu(i);
            return;
        }
        xPos += itemWidth;
    }
}

} // namespace QtTUI
