#include <QtTUI/QTuiLabel.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiLabel::QTuiLabel(const QString &text, QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_text(text)
{
}

QString QTuiLabel::text() const
{
    return m_text;
}

void QTuiLabel::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged(m_text);
    }
}

QSize QTuiLabel::minimumSize() const
{
    return QSize(m_text.length(), 1);
}

void QTuiLabel::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const int startX = geo.x();
    const int y = geo.y();
    const int maxWidth = geo.width();

    for (int i = 0; i < maxWidth && i < m_text.length(); ++i) {
        if (startX + i < buffer.width() && y < buffer.height()) {
            Cell &cell = buffer.backCell(startX + i, y);
            cell.character = m_text.at(i);
        }
    }
}

} // namespace QtTUI
