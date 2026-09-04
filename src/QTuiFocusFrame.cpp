#include <QtTUI/QTuiFocusFrame.h>
#include <QtTUI/RenderUtils.h>
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

    Cell style = cellStyle(fg, bg);
    style.bold = theme.bold;
    drawBorder(buffer, geo, theme.borderStyle, style);
}

} // namespace QtTUI
