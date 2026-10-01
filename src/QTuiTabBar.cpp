#include <QtTUI/QTuiTabBar.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiTabBar::QTuiTabBar(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

int QTuiTabBar::addTab(const QString &label)
{
    m_tabs.append(label);
    int idx = m_tabs.size() - 1;
    if (m_currentIndex < 0)
        setCurrentIndex(0);
    markDirty();
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
    markDirty();
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
    if (m_tabs.at(index) == text)
        return;
    m_tabs[index] = text;
    markDirty();
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
    markDirty();
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

QString QTuiTabBar::tabLine(QList<QPair<int, int>> *ranges) const
{
    QString line;
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (i > 0)
            line += QStringLiteral(" | ");
        ranges->append({int(line.length()), int(m_tabs.at(i).length())});
        line += m_tabs.at(i);
    }
    return line;
}

int QTuiTabBar::scrollOffset(const QList<QPair<int, int>> &ranges, int lineLength, int width) const
{
    if (lineLength <= width || m_currentIndex < 0 || m_currentIndex >= ranges.size())
        return 0;

    // Keep 1 column at each edge free for the scroll indicators
    const int tabStart = ranges.at(m_currentIndex).first;
    const int tabEnd = tabStart + ranges.at(m_currentIndex).second;
    int offset = 0;
    if (tabEnd > width - 1)
        offset = tabEnd - (width - 1);
    if (offset > 0 && tabStart < offset + 1)
        offset = qMax(0, tabStart - 1);
    return qMin(offset, qMax(0, lineLength - width));
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

    QList<QPair<int, int>> tabRanges;
    const QString line = tabLine(&tabRanges);
    const int offset = scrollOffset(tabRanges, line.length(), w);
    const bool moreLeft = offset > 0;
    const bool moreRight = line.length() - offset > w;

    // Render the row
    for (int col = 0; col < w; ++col) {
        int px = x0 + col;
        if (px >= buffer.width() || y0 >= buffer.height())
            continue;

        Cell &cell = buffer.backCell(px, y0);
        const int lineCol = col + offset;

        if (col == 0 && moreLeft)
            cell.setChar(U'‹'); // single left angle quotation mark
        else if (col == w - 1 && moreRight)
            cell.setChar(U'›'); // single right angle quotation mark
        else if (lineCol < line.length())
            cell.setChar(line.at(lineCol).unicode());
        else
            cell.setChar(U' ');

        // Determine if this column falls within the current tab's label
        bool isCurrentTab = false;
        if (m_currentIndex >= 0 && m_currentIndex < tabRanges.size()) {
            int tabStart = tabRanges.at(m_currentIndex).first;
            int tabLen = tabRanges.at(m_currentIndex).second;
            if (lineCol >= tabStart && lineCol < tabStart + tabLen)
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

int QTuiTabBar::tabAt(int x, QRect *rect) const
{
    if (m_tabs.isEmpty())
        return -1;

    const QRect geo = geometry();
    QList<QPair<int, int>> tabRanges;
    const QString line = tabLine(&tabRanges);
    const int offset = scrollOffset(tabRanges, line.length(), geo.width());
    const int col = x - geo.x();

    if (offset > 0 && col == 0) {
        if (rect)
            *rect = QRect(geo.x(), geo.y(), 1, 1);
        return PreviousIndicator;
    }
    if (line.length() - offset > geo.width() && col == geo.width() - 1) {
        if (rect)
            *rect = QRect(geo.x() + col, geo.y(), 1, 1);
        return NextIndicator;
    }

    // Convert the column to a column in the full tab row
    const int lineCol = col + offset;
    for (int i = 0; i < tabRanges.size(); ++i) {
        if (lineCol >= tabRanges.at(i).first
            && lineCol < tabRanges.at(i).first + tabRanges.at(i).second) {
            if (rect)
                *rect = QRect(geo.x() + tabRanges.at(i).first - offset, geo.y(),
                              tabRanges.at(i).second, 1).intersected(geo);
            return i;
        }
    }
    return -1;
}

QRect QTuiTabBar::hoverRect() const
{
    // Only the tab or the scroll indicator under the mouse
    QRect rect;
    return tabAt(hoverPos().x(), &rect) == -1 ? QRect() : rect;
}

void QTuiTabBar::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    // A click on a scroll indicator moves to the previous or next tab
    const int i = tabAt(event->x());
    if (i == PreviousIndicator)
        setCurrentIndex(qMax(0, m_currentIndex - 1));
    else if (i == NextIndicator)
        setCurrentIndex(qMin(int(m_tabs.size()) - 1, m_currentIndex + 1));
    else if (i >= 0)
        setCurrentIndex(i);
}

} // namespace QtTUI
