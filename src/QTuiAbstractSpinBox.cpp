#include <QtTUI/QTuiAbstractSpinBox.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiAbstractSpinBox::QTuiAbstractSpinBox(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

QString QTuiAbstractSpinBox::prefix() const
{
    return m_prefix;
}

void QTuiAbstractSpinBox::setPrefix(const QString &prefix)
{
    if (m_prefix == prefix)
        return;
    m_prefix = prefix;
    markDirty();
}

QString QTuiAbstractSpinBox::suffix() const
{
    return m_suffix;
}

void QTuiAbstractSpinBox::setSuffix(const QString &suffix)
{
    if (m_suffix == suffix)
        return;
    m_suffix = suffix;
    markDirty();
}

QString QTuiAbstractSpinBox::content() const
{
    return QStringLiteral("▲ ") + m_prefix + valueText() + m_suffix + QStringLiteral(" ▼");
}

QSize QTuiAbstractSpinBox::minimumSizeFor(const QString &minimumText, const QString &maximumText) const
{
    // Content: "▲ " + prefix + value + suffix + " ▼"
    int valueWidth = qMax(minimumText.length(), maximumText.length());
    int contentWidth = 2 + m_prefix.length() + valueWidth + m_suffix.length() + 2;

    // Add 2 for left/right border, 1 padding each side
    return QSize(contentWidth + 4, 3); // 3 rows: top border, content, bottom border
}

void QTuiAbstractSpinBox::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    const auto theme = themeProperties();

    // Determine colors based on focus state
    Color fg = theme.foreground;
    Color bg = theme.background;

    if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    fillRect(buffer, geo, cellStyle(theme, fg, bg));
    drawBorder(buffer, geo, theme.borderStyle, cellStyle(theme, fg, bg));

    // Render content centered in the middle row
    if (geo.height() >= 3 && geo.width() >= 3) {
        const QString text = content();
        int innerWidth = geo.width() - 2; // Exclude left/right borders
        int textLen = qMin(QtTuiUnicode::stringWidth(text), innerWidth);
        int startCol = 1 + (innerWidth - textLen) / 2; // Center within inner area

        Cell textStyle = cellStyle(theme, fg, bg);
        textStyle.bold = theme.bold || hasFocus();
        renderText(buffer, geo.x() + startCol, geo.y() + geo.height() / 2, text, textLen, textStyle);
    }
}

void QTuiAbstractSpinBox::keyPressEvent(TuiKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Up) {
        stepBy(1);
    } else if (key == Qt::Key_Down) {
        stepBy(-1);
    }
}

void QTuiAbstractSpinBox::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() == TuiMouseEvent::Wheel) {
        if (event->wheelDelta() > 0)
            stepBy(1);
        else if (event->wheelDelta() < 0)
            stepBy(-1);
        return;
    }

    if (event->mouseType() != TuiMouseEvent::Press)
        return;

    const QRect geo = geometry();
    // Mirror paintEvent: the content is centered on the middle row, only
    // when the widget is at least 3x3.
    if (geo.height() < 3 || geo.width() < 3)
        return;

    const QString text = content();
    const int contentRow = geo.y() + geo.height() / 2;
    const int innerWidth = geo.width() - 2; // exclude left/right borders
    const int textWidth = QtTuiUnicode::stringWidth(text);
    const int textLen = qMin(textWidth, innerWidth);
    const int startX = geo.x() + 1 + (innerWidth - textLen) / 2;

    if (event->y() != contentRow)
        return;
    const int col = event->x() - startX;

    // Step only when the click lands exactly on a painted arrow glyph: ▲ is
    // the first column, ▼ is the last column if the text is not cut.
    if (col == 0 && textLen > 0)
        stepBy(1);
    else if (col == textWidth - 1 && textLen == textWidth)
        stepBy(-1);
}

} // namespace QtTUI
