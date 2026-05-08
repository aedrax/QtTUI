#include <QtTUI/QTuiTabBar.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiTabBar::QTuiTabBar(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

int QTuiTabBar::addTab(const QString &label)
{
    m_tabs.append(label);
    int idx = m_tabs.size() - 1;
    if (m_currentIndex < 0)
        setCurrentIndex(0);
    return idx;
}

void QTuiTabBar::removeTab(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;

    m_tabs.removeAt(index);

    if (m_tabs.isEmpty()) {
        if (m_currentIndex != -1) {
            m_currentIndex = -1;
            emit currentChanged(m_currentIndex);
        }
    } else if (m_currentIndex >= m_tabs.size()) {
        m_currentIndex = m_tabs.size() - 1;
        emit currentChanged(m_currentIndex);
    } else if (index < m_currentIndex) {
        --m_currentIndex;
        emit currentChanged(m_currentIndex);
    } else if (index == m_currentIndex) {
        // Re-emit for same index since the tab content changed
        emit currentChanged(m_currentIndex);
    }
}

int QTuiTabBar::count() const
{
    return m_tabs.size();
}

QString QTuiTabBar::tabText(int index) const
{
    if (index < 0 || index >= m_tabs.size())
        return QString();
    return m_tabs.at(index);
}

void QTuiTabBar::setTabText(int index, const QString &text)
{
    if (index < 0 || index >= m_tabs.size())
        return;
    m_tabs[index] = text;
}

int QTuiTabBar::currentIndex() const
{
    return m_currentIndex;
}

void QTuiTabBar::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_tabs.size())
        return;
    if (m_currentIndex == index)
        return;
    m_currentIndex = index;
    emit currentChanged(m_currentIndex);
}

QTuiTabBar::TabPosition QTuiTabBar::tabPosition() const
{
    return m_tabPosition;
}

void QTuiTabBar::setTabPosition(TabPosition position)
{
    m_tabPosition = position;
}

QSize QTuiTabBar::minimumSize() const
{
    if (m_tabs.isEmpty())
        return QSize(1, 1);

    // Calculate total width: each tab label + " | " separators
    int totalWidth = 0;
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (i > 0)
            totalWidth += 3; // " | "
        totalWidth += m_tabs.at(i).length();
    }
    return QSize(qMax(1, totalWidth), 1);
}

void QTuiTabBar::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.foreground;
    const Color bg = theme.background;
    const Color focusFg = theme.focusForeground;
    const Color focusBg = theme.focusBackground;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();

    // Build the tab label string and track positions of each tab
    // Format: "Tab1 | Tab2 | Tab3"
    QString line;
    QList<QPair<int, int>> tabRanges; // start, length for each tab

    for (int i = 0; i < m_tabs.size(); ++i) {
        if (i > 0)
            line += QStringLiteral(" | ");
        int start = line.length();
        line += m_tabs.at(i);
        tabRanges.append({start, m_tabs.at(i).length()});
    }

    // Render the row
    for (int col = 0; col < w; ++col) {
        int px = x0 + col;
        if (px >= buffer.width() || y0 >= buffer.height())
            continue;

        Cell &cell = buffer.backCell(px, y0);

        if (col < line.length())
            cell.character = line.at(col);
        else
            cell.character = QLatin1Char(' ');

        // Determine if this column falls within the current tab's label
        bool isCurrentTab = false;
        if (m_currentIndex >= 0 && m_currentIndex < tabRanges.size()) {
            int tabStart = tabRanges.at(m_currentIndex).first;
            int tabLen = tabRanges.at(m_currentIndex).second;
            if (col >= tabStart && col < tabStart + tabLen)
                isCurrentTab = true;
        }

        if (isCurrentTab) {
            cell.foreground = focusFg;
            cell.background = focusBg;
            cell.bold = true;
        } else {
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }
}

void QTuiTabBar::keyPressEvent(TuiKeyEvent *event)
{
    if (m_tabs.isEmpty()) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        if (m_currentIndex > 0)
            setCurrentIndex(m_currentIndex - 1);
        break;
    case Qt::Key_Right:
        if (m_currentIndex < m_tabs.size() - 1)
            setCurrentIndex(m_currentIndex + 1);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // Enter confirms/selects the current tab (re-emit signal)
        if (m_currentIndex >= 0)
            emit currentChanged(m_currentIndex);
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        break;
    }
}

void QTuiTabBar::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    if (m_tabs.isEmpty())
        return;

    const QRect geo = geometry();
    // Convert click position to column offset within the tab bar
    int col = event->x() - geo.x();

    // Walk through tabs using the same layout as paintEvent:
    // "Tab1 | Tab2 | Tab3" — each separator is " | " (3 chars)
    int offset = 0;
    for (int i = 0; i < m_tabs.size(); ++i) {
        int tabLen = m_tabs.at(i).length();
        if (col >= offset && col < offset + tabLen) {
            setCurrentIndex(i);
            return;
        }
        offset += tabLen + 3; // tab text + " | "
    }
}

} // namespace QtTUI
