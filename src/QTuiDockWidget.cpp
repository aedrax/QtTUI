#include <QtTUI/QTuiDockWidget.h>
#include <QtTUI/RenderUtils.h>
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

    drawWindowFrame(buffer, geo, themeProperties(), m_windowTitle, QStringLiteral("[X]"));

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

void QTuiDockWidget::mousePressEvent(TuiMouseEvent *event)
{
    // "[X]" is in the 3 columns before the top-right corner
    const QRect geo = geometry();
    if (event->mouseType() == TuiMouseEvent::Press && event->button() == Qt::LeftButton
        && event->y() == geo.y() && event->x() >= geo.right() - 3 && event->x() < geo.right()) {
        setVisible(false);
        emit visibilityChanged(false);
    }
}

} // namespace QtTUI
