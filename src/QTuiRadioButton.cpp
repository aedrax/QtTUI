#include <QtTUI/QTuiRadioButton.h>
#include <QtTUI/QtTuiUnicode.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiRadioButton::QTuiRadioButton(const QString &text, QTuiWidget *parent)
    : QTuiAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
}

void QTuiRadioButton::nextCheckState()
{
    setChecked(true);
}

QSize QTuiRadioButton::minimumSize() const
{
    // "(*) " = 4 chars + text length
    return QSize(4 + QtTuiUnicode::stringWidth(text()), 1);
}

void QTuiRadioButton::paintEvent(ScreenBuffer &buffer)
{
    paintIndicator(buffer, isChecked() ? QStringLiteral("(*)") : QStringLiteral("( )"));
}

} // namespace QtTUI
