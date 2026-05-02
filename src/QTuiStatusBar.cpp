#include <QtTUI/QTuiStatusBar.h>
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
    m_permanentWidgets.append(widget);
}

void QTuiStatusBar::removeWidget(QTuiWidget *widget)
{
    m_permanentWidgets.removeAll(widget);
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
    for (int col = 0; col < geo.width() && (geo.x() + col) < buffer.width(); ++col) {
        int px = geo.x() + col;
        Cell &cell = buffer.backCell(px, py);
        cell.character = QLatin1Char(' ');
        cell.foreground = fg;
        cell.background = bg;
        cell.bold = false;
        cell.italic = false;
        cell.underline = false;
    }

    // Calculate space used by permanent widgets on the right
    int permanentWidth = 0;
    for (auto *w : m_permanentWidgets) {
        if (w && w->isVisible())
            permanentWidth += w->minimumSize().width();
    }

    // Render message text on the left
    const QString msg = currentMessage();
    int availableForMessage = geo.width() - permanentWidth;
    int xPos = geo.x();

    for (int i = 0; i < msg.length() && i < availableForMessage; ++i) {
        if (xPos < buffer.width()) {
            Cell &cell = buffer.backCell(xPos, py);
            cell.character = msg.at(i);
            cell.foreground = fg;
            cell.background = bg;
        }
        xPos++;
    }

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
