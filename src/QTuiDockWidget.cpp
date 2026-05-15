#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiDockWidget::QTuiDockWidget(const QString &title, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_windowTitle(title)
{
    setFocusPolicy(Qt::StrongFocus);
}

void QTuiDockWidget::setWidget(QTuiWidget *widget)
{
    m_widget = widget;
    if (m_widget)
        m_widget->setParent(this);
}

QTuiWidget *QTuiDockWidget::widget() const
{
    return m_widget;
}

QString QTuiDockWidget::windowTitle() const
{
    return m_windowTitle;
}

void QTuiDockWidget::setWindowTitle(const QString &title)
{
    if (m_windowTitle == title)
        return;
    m_windowTitle = title;
    markDirty();
}

Qt::DockWidgetAreas QTuiDockWidget::allowedAreas() const
{
    return m_allowedAreas;
}

void QTuiDockWidget::setAllowedAreas(Qt::DockWidgetAreas areas)
{
    m_allowedAreas = areas;
}

bool QTuiDockWidget::isFloatable() const
{
    return m_floatable;
}

void QTuiDockWidget::setFloatable(bool floatable)
{
    m_floatable = floatable;
}

QSize QTuiDockWidget::minimumSize() const
{
    // Title bar row + at least 1 content row + bottom border = 3 rows
    // Width: border + title + close indicator "[X]" + border
    int titleWidth = m_windowTitle.length() + 6; // " title [X]" with borders
    int contentWidth = 4; // minimum 2 interior cols + 2 border cols
    if (m_widget) {
        QSize ws = m_widget->minimumSize();
        contentWidth = ws.width() + 2; // add border cols
    }
    return QSize(qMax(titleWidth, contentWidth), 3);
}

QRect QTuiDockWidget::contentArea() const
{
    const QRect geo = geometry();
    if (geo.width() < 3 || geo.height() < 3)
        return QRect();
    // Chrome: 1 row title bar (top border), 1 row bottom border, 1 col left border, 1 col right border
    return QRect(geo.x() + 1, geo.y() + 1, geo.width() - 2, geo.height() - 2);
}

void QTuiDockWidget::paintEvent(ScreenBuffer &buffer)
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

    // --- Title bar (row 0) ---
    // Top border with title and close indicator
    for (int col = 0; col < w; ++col) {
        int px = x0 + col;
        if (px >= buffer.width() || y0 >= buffer.height())
            continue;
        Cell &cell = buffer.backCell(px, y0);
        if (col == 0)
            cell.setChar(border.topLeft);
        else if (col == w - 1)
            cell.setChar(border.topRight);
        else
            cell.setChar(border.horizontal);
        cell.foreground = fg;
        cell.background = bg;
        cell.bold = theme.bold;
    }

    // Write title text starting at col 2
    if (!m_windowTitle.isEmpty() && w > 4) {
        // Reserve space for close indicator "[X]" at the right (3 chars + 1 space before border)
        int closeIndicatorWidth = 4; // " [X]"
        int maxTitleLen = w - 4 - closeIndicatorWidth; // borders + close
        if (maxTitleLen < 0)
            maxTitleLen = 0;
        QString titleStr = m_windowTitle.left(maxTitleLen);

        int titleCol = 2;
        for (int i = 0; i < titleStr.length(); ++i) {
            int px = x0 + titleCol + i;
            if (px < buffer.width() && y0 < buffer.height()) {
                Cell &cell = buffer.backCell(px, y0);
                cell.setChar(titleStr.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = true;
            }
        }
    }

    // Write close indicator "[X]" near right edge of title bar
    if (w >= 6) {
        const QString closeStr = QStringLiteral("[X]");
        int closeStart = x0 + w - 1 - closeStr.length(); // before right border
        for (int i = 0; i < closeStr.length(); ++i) {
            int px = closeStart + i;
            if (px >= x0 + 1 && px < buffer.width() && y0 < buffer.height()) {
                Cell &cell = buffer.backCell(px, y0);
                cell.setChar(closeStr.at(i).unicode());
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
            }
        }
    }

    // --- Bottom border ---
    int by = y0 + h - 1;
    if (by < buffer.height()) {
        for (int col = 0; col < w; ++col) {
            int px = x0 + col;
            if (px >= buffer.width())
                continue;
            Cell &cell = buffer.backCell(px, by);
            if (col == 0)
                cell.setChar(border.bottomLeft);
            else if (col == w - 1)
                cell.setChar(border.bottomRight);
            else
                cell.setChar(border.horizontal);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }

    // --- Left and right borders (interior rows) ---
    for (int row = 1; row < h - 1; ++row) {
        int py = y0 + row;
        if (py >= buffer.height())
            continue;
        if (x0 < buffer.width()) {
            Cell &cell = buffer.backCell(x0, py);
            cell.setChar(border.vertical);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
        int rx = x0 + w - 1;
        if (rx < buffer.width()) {
            Cell &cell = buffer.backCell(rx, py);
            cell.setChar(border.vertical);
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }

    // --- Clear interior ---
    for (int row = 1; row < h - 1; ++row) {
        for (int col = 1; col < w - 1; ++col) {
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

    // --- Paint content widget within content area ---
    if (m_widget && m_widget->isVisible()) {
        const QRect content = contentArea();
        if (!content.isEmpty()) {
            m_widget->setGeometry(content);
            m_widget->paintEvent(buffer);
        }
    }
}

void QTuiDockWidget::keyPressEvent(TuiKeyEvent *event)
{
    // 'q' or Escape activates the close indicator: hides dock and emits visibilityChanged
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Q) {
        setVisible(false);
        emit visibilityChanged(false);
        return;
    }
    QTuiWidget::keyPressEvent(event);
}

} // namespace QtTUI
