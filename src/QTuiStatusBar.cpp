#include <QtTUI/QTuiStatusBar.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiStatusBar::QTuiStatusBar(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &QTuiStatusBar::onTimeout);
}

void QTuiStatusBar::showMessage(const QString &text, int timeout)
{
    m_temporaryMessage = text;
    markDirty();
    emit messageChanged(currentMessage());

    if (timeout > 0)
        m_timer->start(timeout);
    else
        m_timer->stop();
}

void QTuiStatusBar::clearMessage()
{
    m_temporaryMessage.clear();
    m_timer->stop();
    markDirty();
    emit messageChanged(currentMessage());
}

QString QTuiStatusBar::currentMessage() const
{
    if (!m_temporaryMessage.isEmpty())
        return m_temporaryMessage;
    return m_permanentMessage;
}

void QTuiStatusBar::addPermanentWidget(QTuiWidget *widget, int stretch)
{
    Q_UNUSED(stretch);
    if (!widget || m_permanentWidgets.contains(widget))
        return;
    widget->setParent(this);
    widget->setVisible(true);
    m_permanentWidgets.append(widget);
    markDirty();
}

void QTuiStatusBar::removeWidget(QTuiWidget *widget)
{
    // As in Qt, the widget is hidden, not deleted
    if (m_permanentWidgets.removeAll(widget) == 0)
        return;
    widget->setVisible(false);
    markDirty();
}

QSize QTuiStatusBar::minimumSize() const
{
    return QSize(1, 1);
}

void QTuiStatusBar::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();
    Color fg = theme.foreground;
    Color bg = theme.background;

    int py = geo.y();
    if (py >= buffer.height())
        return;

    // Clear the entire status bar row
    fillRect(buffer, QRect(geo.x(), py, geo.width(), 1), cellStyle(fg, bg));

    // Calculate space used by permanent widgets on the right
    int permanentWidth = 0;
    for (auto *w : m_permanentWidgets) {
        if (w && w->isVisible())
            permanentWidth += w->minimumSize().width();
    }

    // Render message text on the left
    renderText(buffer, geo.x(), py, currentMessage(), geo.width() - permanentWidth, cellStyle(fg, bg));

    // Position permanent widgets on the right side
    int rightX = geo.x() + geo.width();
    for (int i = m_permanentWidgets.size() - 1; i >= 0; --i) {
        auto *w = m_permanentWidgets.at(i);
        if (!w || !w->isVisible())
            continue;
        int wWidth = w->minimumSize().width();
        rightX -= wWidth;
        w->setGeometry(QRect(rightX, py, wWidth, 1));
    }
}

void QTuiStatusBar::onTimeout()
{
    m_temporaryMessage.clear();
    markDirty();
    emit messageChanged(currentMessage());
}

} // namespace QtTUI
