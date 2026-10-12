#include <QtTUI/QTuiGroupBox.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/QTuiLayout.h>
#include <QtTUI/RenderUtils.h>
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
    if (m_title == title)
        return;
    m_title = title;
    markDirty();
}

bool QTuiGroupBox::isCheckable() const
{
    return m_checkable;
}

void QTuiGroupBox::setCheckable(bool checkable)
{
    if (m_checkable == checkable)
        return;
    m_checkable = checkable;
    // Only a checkable group box is interactive (Space/Enter toggles it);
    // a plain group box is a passive container that Tab must skip.
    setFocusPolicy(checkable ? Qt::StrongFocus : Qt::NoFocus);
    markDirty();
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
    markDirty();
}

void QTuiGroupBox::setLayout(QTuiLayout *layout)
{
    // The group box owns the layout, as QWidget::setLayout() does; the
    // layout also finds its layout direction through its parent
    m_layout = layout;
    if (m_layout)
        m_layout->setParent(this);
}

QTuiLayout *QTuiGroupBox::layout() const
{
    return m_layout;
}

QSize QTuiGroupBox::minimumSize() const
{
    // Border takes 2 cols and 2 rows minimum
    int titleWidth = m_title.isEmpty() ? 0 : QtTuiUnicode::stringWidth(m_title) + 2; // space padding
    if (m_checkable)
        titleWidth += 4; // "[x] " prefix
    QSize size(qMax(4, titleWidth + 2), 3);
    // The layout goes inside the border, so add 2 cells on each axis
    if (m_layout)
        size = size.expandedTo(m_layout->minimumSize() + QSize(2, 2));
    return size;
}

QRect QTuiGroupBox::contentArea() const
{
    const QRect geo = geometry();
    if (geo.width() < 3 || geo.height() < 3)
        return QRect();
    // Content area is inside the border: 1 cell inset on all sides
    return QRect(geo.x() + 1, geo.y() + 1, geo.width() - 2, geo.height() - 2);
}

void QTuiGroupBox::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.foreground;
    const Color bg = theme.background;

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();

    // --- Render chrome first: borders and title ---

    // The border and the title use the bold of the theme; the interior is plain
    Cell chromeStyle = cellStyle(fg, bg);
    chromeStyle.bold = theme.bold;
    drawBorder(buffer, geo, theme.borderStyle, chromeStyle);
    fillRect(buffer, geo.adjusted(1, 1, -1, -1), cellStyle(fg, bg));

    // Draw title in top border (if present)
    if (!m_title.isEmpty() && w > 4) {
        QString titleStr;
        if (m_checkable) {
            titleStr = (m_checked ? QStringLiteral("[x] ") : QStringLiteral("[ ] "));
        }
        titleStr += m_title;
        // Start after the border and a space; leave room for the border chars and spacing
        renderText(buffer, x0 + 2, y0, titleStr, w - 4, chromeStyle);
    }

    // --- Compute content area and set child geometries within it ---
    const QRect content = contentArea();

    // Apply layout to content area if set
    if (m_layout && !content.isEmpty()) {
        m_layout->apply(content);
    }

    // Ensure child geometries are contained within content area
    const auto children = childWidgets();
    for (auto *child : children) {
        if (!content.isEmpty()) {
            QRect childGeo = child->geometry();
            // Clamp child geometry to content area
            QRect clamped = childGeo.intersected(content);
            if (clamped != childGeo && !childGeo.isEmpty()) {
                child->setGeometry(clamped);
            }
        }
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

void QTuiGroupBox::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() != TuiMouseEvent::Press)
        return;
    if (!m_checkable)
        return;

    // Only clicks on the painted "[x] Title" span in the top border toggle
    // the checkbox. Mirror paintEvent: the title is drawn on row geo.y(),
    // starting at column geo.x() + 2, truncated to width - 4, and only when
    // the title is non-empty and the box is wider than 4 cells.
    const QRect geo = geometry();
    if (m_title.isEmpty() || geo.width() <= 4)
        return;
    if (event->y() != geo.y())
        return;

    const int titleLen = 4 + QtTuiUnicode::stringWidth(m_title); // "[x] " prefix + title
    const int span = qMin(titleLen, geo.width() - 4);
    const int startX = geo.x() + 2;
    if (event->x() < startX || event->x() >= startX + span)
        return;

    setChecked(!m_checked);
}

void QTuiGroupBox::updateChildrenEnabled()
{
    if (!m_checkable)
        return;

    // Unchecking disables the children instead of hiding them (matching
    // QGroupBox): they stay visible but inert, rendered with the disabled
    // palette, and drop out of the Tab focus chain.
    const auto children = childWidgets();
    for (auto *child : children) {
        child->setEnabled(m_checked);
    }
}

} // namespace QtTUI
