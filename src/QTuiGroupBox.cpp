#include <QtTUI/QTuiGroupBox.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiGroupBox::QTuiGroupBox(const QString &title, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_title(title)
{
}

QString QTuiGroupBox::title() const
{
    return m_title;
}

void QTuiGroupBox::setTitle(const QString &title)
{
    m_title = title;
}

bool QTuiGroupBox::isCheckable() const
{
    return m_checkable;
}

void QTuiGroupBox::setCheckable(bool checkable)
{
    m_checkable = checkable;
}

bool QTuiGroupBox::isChecked() const
{
    return m_checked;
}

void QTuiGroupBox::setChecked(bool checked)
{
    if (m_checked == checked)
        return;
    m_checked = checked;
    updateChildrenEnabled();
    emit toggled(m_checked);
}

void QTuiGroupBox::setLayout(QTuiLayout *layout)
{
    m_layout = layout;
}

QTuiLayout *QTuiGroupBox::layout() const
{
    return m_layout;
}

QSize QTuiGroupBox::minimumSize() const
{
    // Border takes 2 cols and 2 rows minimum
    int titleWidth = m_title.isEmpty() ? 0 : m_title.length() + 2; // space padding
    if (m_checkable)
        titleWidth += 4; // "[x] " prefix
    return QSize(qMax(4, titleWidth + 2), 3);
}

void QTuiGroupBox::paintEvent(ScreenBuffer &buffer)
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

    // Draw border
    for (int col = 0; col < w; ++col) {
        int px = x0 + col;
        // Top border
        if (y0 < buffer.height() && px < buffer.width()) {
            Cell &cell = buffer.backCell(px, y0);
            if (col == 0)
                cell.character = border.topLeft;
            else if (col == w - 1)
                cell.character = border.topRight;
            else
                cell.character = border.horizontal;
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
        // Bottom border
        int by = y0 + h - 1;
        if (by < buffer.height() && px < buffer.width()) {
            Cell &cell = buffer.backCell(px, by);
            if (col == 0)
                cell.character = border.bottomLeft;
            else if (col == w - 1)
                cell.character = border.bottomRight;
            else
                cell.character = border.horizontal;
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }

    // Left and right borders (interior rows)
    for (int row = 1; row < h - 1; ++row) {
        int py = y0 + row;
        if (py >= buffer.height())
            continue;
        // Left
        if (x0 < buffer.width()) {
            Cell &cell = buffer.backCell(x0, py);
            cell.character = border.vertical;
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
        // Right
        int rx = x0 + w - 1;
        if (rx < buffer.width()) {
            Cell &cell = buffer.backCell(rx, py);
            cell.character = border.vertical;
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    }

    // Clear interior
    for (int row = 1; row < h - 1; ++row) {
        for (int col = 1; col < w - 1; ++col) {
            int px = x0 + col;
            int py = y0 + row;
            if (px < buffer.width() && py < buffer.height()) {
                Cell &cell = buffer.backCell(px, py);
                cell.character = QLatin1Char(' ');
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = false;
            }
        }
    }

    // Draw title in top border (if present)
    if (!m_title.isEmpty() && w > 4) {
        int titleCol = 2; // start after border + space
        QString titleStr;
        if (m_checkable) {
            titleStr = (m_checked ? QStringLiteral("[x] ") : QStringLiteral("[ ] "));
        }
        titleStr += m_title;

        int maxLen = w - 4; // leave room for border chars and spacing
        if (titleStr.length() > maxLen)
            titleStr = titleStr.left(maxLen);

        for (int i = 0; i < titleStr.length(); ++i) {
            int px = x0 + titleCol + i;
            if (px < buffer.width() && y0 < buffer.height()) {
                Cell &cell = buffer.backCell(px, y0);
                cell.character = titleStr.at(i);
                cell.foreground = fg;
                cell.background = bg;
                cell.bold = theme.bold;
            }
        }
    }

    // Apply layout to interior area if set
    if (m_layout) {
        QRect interior(x0 + 1, y0 + 1, w - 2, h - 2);
        m_layout->apply(interior);
    }

    // Paint child widgets
    const auto children = childWidgets();
    for (auto *child : children) {
        if (child->isVisible())
            child->paintEvent(buffer);
    }
}

void QTuiGroupBox::keyPressEvent(TuiKeyEvent *event)
{
    if (m_checkable && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return)) {
        setChecked(!m_checked);
        return;
    }
    QTuiWidget::keyPressEvent(event);
}

void QTuiGroupBox::updateChildrenEnabled()
{
    if (!m_checkable)
        return;

    const auto children = childWidgets();
    for (auto *child : children) {
        child->setVisible(m_checked);
    }
}

} // namespace QtTUI
