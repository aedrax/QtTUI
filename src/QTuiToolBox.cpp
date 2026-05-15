#include <QtTUI/QTuiToolBox.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiToolBox::QTuiToolBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

int QTuiToolBox::addItem(QTuiWidget *widget, const QString &label)
{
    if (!widget)
        return -1;

    widget->setParent(this);
    m_items.append({label, widget});
    int index = m_items.size() - 1;

    if (m_items.size() == 1) {
        // First item becomes current
        m_currentIndex = 0;
        m_focusedHeader = 0;
        widget->setVisible(true);
    } else {
        widget->setVisible(false);
    }

    return index;
}

void QTuiToolBox::removeItem(int index)
{
    if (index < 0 || index >= m_items.size())
        return;

    QTuiWidget *w = m_items.at(index).widget;
    w->setVisible(false);
    m_items.removeAt(index);

    if (m_items.isEmpty()) {
        m_currentIndex = -1;
        m_focusedHeader = 0;
    } else {
        if (m_currentIndex >= m_items.size())
            m_currentIndex = m_items.size() - 1;
        if (m_focusedHeader >= m_items.size())
            m_focusedHeader = m_items.size() - 1;
        // Ensure current item's widget is visible
        if (m_currentIndex >= 0)
            m_items.at(m_currentIndex).widget->setVisible(true);
    }
}

int QTuiToolBox::count() const
{
    return m_items.size();
}

QTuiWidget *QTuiToolBox::widget(int index) const
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    return m_items.at(index).widget;
}

QString QTuiToolBox::itemText(int index) const
{
    if (index < 0 || index >= m_items.size())
        return QString();
    return m_items.at(index).label;
}

int QTuiToolBox::currentIndex() const
{
    return m_currentIndex;
}

void QTuiToolBox::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_items.size())
        return;
    if (index == m_currentIndex)
        return;

    // Collapse previous
    if (m_currentIndex >= 0 && m_currentIndex < m_items.size())
        m_items.at(m_currentIndex).widget->setVisible(false);

    m_currentIndex = index;
    m_focusedHeader = index;

    // Expand new
    m_items.at(m_currentIndex).widget->setVisible(true);

    emit currentChanged(m_currentIndex);
    markDirty();
}

QSize QTuiToolBox::minimumSize() const
{
    // All headers (1 row each) + at least 1 row for content
    int headerCount = m_items.size();
    int minWidth = 4;
    for (const auto &item : m_items) {
        int labelWidth = 2 + item.label.length(); // indicator + space + label
        if (labelWidth > minWidth)
            minWidth = labelWidth;
    }
    return QSize(minWidth, qMax(1, headerCount + 1));
}

void QTuiToolBox::paintEvent(ScreenBuffer &buffer)
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
    const int h = geo.height();

    // Clear entire area
    for (int row = 0; row < h; ++row) {
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

    // Render headers and content vertically
    int currentRow = 0;
    for (int i = 0; i < m_items.size() && currentRow < h; ++i) {
        const auto &item = m_items.at(i);
        bool isCurrent = (i == m_currentIndex);
        bool isFocusedHeader = (i == m_focusedHeader) && hasFocus();

        // Draw header row
        QString indicator = isCurrent ? QStringLiteral("\u25BC ") : QStringLiteral("\u25B6 ");
        QString headerLine = indicator + item.label;
        if (headerLine.length() > w)
            headerLine = headerLine.left(w);

        Color headerFg = isFocusedHeader ? focusFg : fg;
        Color headerBg = isFocusedHeader ? focusBg : bg;

        int py = y0 + currentRow;
        if (py < buffer.height()) {
            for (int col = 0; col < w; ++col) {
                int px = x0 + col;
                if (px >= buffer.width())
                    continue;
                Cell &cell = buffer.backCell(px, py);
                if (col < headerLine.length())
                    cell.setChar(headerLine.at(col).unicode());
                else
                    cell.setChar(U' ');
                cell.foreground = headerFg;
                cell.background = headerBg;
                cell.bold = theme.bold || isFocusedHeader;
            }
        }
        ++currentRow;

        // If this is the current (expanded) item, render its content widget
        if (isCurrent && item.widget) {
            // Calculate remaining rows for content before next header
            int remainingHeaders = m_items.size() - i - 1;
            int availableRows = h - currentRow - remainingHeaders;
            if (availableRows < 1)
                availableRows = 1;

            QRect contentRect(x0, y0 + currentRow, w, availableRows);
            item.widget->setGeometry(contentRect);
            item.widget->setVisible(true);
            item.widget->paintEvent(buffer);

            currentRow += availableRows;
        }
    }
}

void QTuiToolBox::keyPressEvent(TuiKeyEvent *event)
{
    if (m_items.isEmpty()) {
        QTuiWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Up:
        if (m_focusedHeader > 0) {
            --m_focusedHeader;
            markDirty();
        }
        break;
    case Qt::Key_Down:
        if (m_focusedHeader < m_items.size() - 1) {
            ++m_focusedHeader;
            markDirty();
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Space:
        setCurrentIndex(m_focusedHeader);
        break;
    default:
        QTuiWidget::keyPressEvent(event);
        return;
    }
}

void QTuiToolBox::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    if (m_items.isEmpty())
        return;

    const QRect geo = geometry();
    int clickY = event->y() - geo.y();
    if (clickY < 0)
        return;

    // Walk through items to find which header row was clicked
    int currentRow = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        if (clickY == currentRow) {
            // Clicked on this header
            setCurrentIndex(i);
            return;
        }
        ++currentRow;

        // If this is the current (expanded) item, skip its content rows
        if (i == m_currentIndex) {
            int remainingHeaders = m_items.size() - i - 1;
            int availableRows = geo.height() - currentRow - remainingHeaders;
            if (availableRows < 1)
                availableRows = 1;
            currentRow += availableRows;
        }
    }
}

} // namespace QtTUI
