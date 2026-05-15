#include <QtTUI/QTuiTabWidget.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiTabWidget::QTuiTabWidget(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_tabBar(new QTuiTabBar(this))
{
    connect(m_tabBar, &QTuiTabBar::currentChanged, this, &QTuiTabWidget::onTabChanged);
}

int QTuiTabWidget::addTab(QTuiWidget *page, const QString &label)
{
    if (!page)
        return -1;

    m_pages.append(page);
    page->setParent(this);
    int idx = m_tabBar->addTab(label);

    // Hide all non-current pages
    if (m_pages.size() == 1) {
        // First page added, it becomes current via tabBar's setCurrentIndex(0)
        // onTabChanged will be called by the signal
        // Ensure the first page is dirty so it gets painted on initial render
        page->markDirty();
    } else {
        page->setVisible(false);
    }

    updatePageGeometries();
    return idx;
}

void QTuiTabWidget::removeTab(int index)
{
    if (index < 0 || index >= m_pages.size())
        return;

    QTuiWidget *page = m_pages.takeAt(index);
    page->setVisible(false);
    m_tabBar->removeTab(index);

    // Update current index tracking
    if (m_pages.isEmpty()) {
        m_currentIndex = -1;
    } else if (m_currentIndex >= m_pages.size()) {
        m_currentIndex = m_pages.size() - 1;
    }

    updatePageGeometries();
}

int QTuiTabWidget::count() const
{
    return m_pages.size();
}

QTuiWidget *QTuiTabWidget::widget(int index) const
{
    if (index < 0 || index >= m_pages.size())
        return nullptr;
    return m_pages.at(index);
}

int QTuiTabWidget::indexOf(QTuiWidget *widget) const
{
    return m_pages.indexOf(widget);
}

int QTuiTabWidget::currentIndex() const
{
    return m_currentIndex;
}

void QTuiTabWidget::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_pages.size())
        return;
    m_tabBar->setCurrentIndex(index);
    // onTabChanged will handle the rest via signal
}

QTuiWidget *QTuiTabWidget::currentWidget() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_pages.size())
        return nullptr;
    return m_pages.at(m_currentIndex);
}

QTuiTabBar *QTuiTabWidget::tabBar() const
{
    return m_tabBar;
}

void QTuiTabWidget::onTabChanged(int index)
{
    if (index == m_currentIndex)
        return;

    // Hide old page and mark it dirty
    if (m_currentIndex >= 0 && m_currentIndex < m_pages.size()) {
        QTuiWidget *oldPage = m_pages.at(m_currentIndex);
        oldPage->setVisible(false);
        oldPage->markDirty();
    }

    m_currentIndex = index;

    // Show new page and mark it dirty
    if (m_currentIndex >= 0 && m_currentIndex < m_pages.size()) {
        QTuiWidget *newPage = m_pages.at(m_currentIndex);
        newPage->setVisible(true);
        newPage->markDirty();
    }

    // Mark the TabWidget itself dirty so its paintEvent runs and clears the content area
    markDirty();

    emit currentChanged(m_currentIndex);
}

void QTuiTabWidget::updatePageGeometries()
{
    const QRect geo = geometry();
    if (geo.isEmpty())
        return;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // Tab bar occupies the first row (North position)
    m_tabBar->setGeometry(QRect(x0, y0, w, 1));

    // Content area is below the tab bar and separator line (2 rows of chrome)
    const QRect content = contentArea();

    for (auto *page : m_pages) {
        page->setGeometry(content);
    }
}

QSize QTuiTabWidget::minimumSize() const
{
    // Chrome is 2 rows (tab bar + separator). The content area needs at
    // least 1 row, else contentArea() is empty and no page paints.
    int width = m_tabBar->minimumSize().width();
    int pageHeight = 1;
    for (auto *page : m_pages) {
        const QSize ms = page->minimumSize();
        width = qMax(width, ms.width());
        pageHeight = qMax(pageHeight, ms.height());
    }
    return QSize(width, 2 + pageHeight);
}

QRect QTuiTabWidget::contentArea() const
{
    const QRect geo = geometry();
    if (geo.isEmpty() || geo.height() < 3)
        return QRect();
    // Chrome: 1 row for tab bar + 1 row for separator = 2 rows
    return QRect(geo.x(), geo.y() + 2, geo.width(), qMax(0, geo.height() - 2));
}

void QTuiTabWidget::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const auto border = Theme_Engine::borderChars(theme.borderStyle);
    const Color fg = theme.foreground;
    const Color bg = theme.background;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();

    // Update geometries before painting
    updatePageGeometries();

    // --- Render chrome first ---

    // Paint the tab bar
    m_tabBar->paintEvent(buffer);

    // Clear the content area below the tab bar
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

    // Draw a separator line between tab bar and content
    if (h > 1) {
        int sepY = y0 + 1;
        if (sepY < buffer.height()) {
            for (int col = 0; col < w; ++col) {
                int px = x0 + col;
                if (px < buffer.width()) {
                    Cell &cell = buffer.backCell(px, sepY);
                    cell.setChar(border.horizontal);
                    cell.foreground = fg;
                    cell.background = bg;
                    cell.bold = theme.bold;
                }
            }
        }
    }

    // --- Paint only the current (active) page within content area ---
    if (m_currentIndex >= 0 && m_currentIndex < m_pages.size()) {
        QTuiWidget *page = m_pages.at(m_currentIndex);
        if (page->isVisible()) {
            const QRect content = contentArea();
            // Ensure page geometry is within content area
            page->setGeometry(content);
            page->paintEvent(buffer);
        }
    }
}

void QTuiTabWidget::resizeEvent(TuiResizeEvent *event)
{
    Q_UNUSED(event);
    updatePageGeometries();
}

} // namespace QtTUI
