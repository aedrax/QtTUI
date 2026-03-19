#include <QtTUI/QTuiFocusFrame.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiFocusFrame::QTuiFocusFrame(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setVisible(false);
}

void QTuiFocusFrame::setWidget(QTuiWidget *widget)
{
    if (m_target == widget)
        return;

    // Disconnect from previous target
    if (m_target) {
        disconnect(m_target, &QTuiWidget::focusGained, this, &QTuiFocusFrame::onTargetFocusGained);
        disconnect(m_target, &QTuiWidget::focusLost, this, &QTuiFocusFrame::onTargetFocusLost);
        disconnect(m_target, &QTuiWidget::geometryChanged, this, &QTuiFocusFrame::onTargetGeometryChanged);
    }

    m_target = widget;

    if (m_target) {
        connect(m_target, &QTuiWidget::focusGained, this, &QTuiFocusFrame::onTargetFocusGained);
        connect(m_target, &QTuiWidget::focusLost, this, &QTuiFocusFrame::onTargetFocusLost);
        connect(m_target, &QTuiWidget::geometryChanged, this, &QTuiFocusFrame::onTargetGeometryChanged);

        updateFrameGeometry();
        setVisible(m_target->hasFocus());
    } else {
        setVisible(false);
    }
}

QTuiWidget *QTuiFocusFrame::widget() const
{
    return m_target;
}

void QTuiFocusFrame::onTargetFocusGained()
{
    updateFrameGeometry();
    setVisible(true);
}

void QTuiFocusFrame::onTargetFocusLost()
{
    setVisible(false);
}

void QTuiFocusFrame::onTargetGeometryChanged(const QRect &rect)
{
    Q_UNUSED(rect);
    updateFrameGeometry();
}

void QTuiFocusFrame::updateFrameGeometry()
{
    if (!m_target)
        return;

    // Position the focus frame 1 cell outside the target's geometry on all sides
    const QRect targetGeo = m_target->geometry();
    setGeometry(QRect(targetGeo.x() - 1,
                      targetGeo.y() - 1,
                      targetGeo.width() + 2,
                      targetGeo.height() + 2));
}

void QTuiFocusFrame::paintEvent(ScreenBuffer &buffer)
{
    if (!isVisible() || !m_target)
        return;

    const QRect geo = geometry();
    if (geo.isEmpty())
        return;

    const auto theme = themeProperties();
    const Color fg = theme.focusForeground;
    const Color bg = theme.background;

    const auto border = Theme_Engine::borderChars(theme.borderStyle);

    const int x0 = geo.x();
    const int y0 = geo.y();
    const int w = geo.width();
    const int h = geo.height();
    const int x1 = x0 + w - 1;
    const int y1 = y0 + h - 1;

    // Draw the four corners
    auto setCell = [&](int x, int y, QChar ch) {
        if (x >= 0 && x < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(x, y);
            cell.character = ch;
            cell.foreground = fg;
            cell.background = bg;
            cell.bold = theme.bold;
        }
    };

    setCell(x0, y0, border.topLeft);
    setCell(x1, y0, border.topRight);
    setCell(x0, y1, border.bottomLeft);
    setCell(x1, y1, border.bottomRight);

    // Draw top and bottom horizontal edges
    for (int col = x0 + 1; col < x1; ++col) {
        setCell(col, y0, border.horizontal);
        setCell(col, y1, border.horizontal);
    }

    // Draw left and right vertical edges
    for (int row = y0 + 1; row < y1; ++row) {
        setCell(x0, row, border.vertical);
        setCell(x1, row, border.vertical);
    }
}

} // namespace QtTUI
