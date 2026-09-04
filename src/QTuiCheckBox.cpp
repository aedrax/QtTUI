#include <QtTUI/QTuiCheckBox.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiCheckBox::QTuiCheckBox(const QString &text, QTuiWidget *parent)
    : QTuiAbstractButton(parent)
{
    setText(text);
    setCheckable(true);
}

int QTuiCheckBox::checkState() const
{
    return m_checkState;
}

void QTuiCheckBox::setCheckState(int state)
{
    // Clamp to valid range
    state = qBound(0, state, 2);
    if (m_checkState == state)
        return;
    m_checkState = state;
    emit stateChanged(m_checkState);
    markDirty();
    // Keep the base checked state (Checked only) in sync
    QTuiAbstractButton::setChecked(m_checkState == Checked);
}

void QTuiCheckBox::nextCheckState()
{
    if (m_tristate)
        setCheckState((m_checkState + 1) % 3); // Unchecked, Partially, Checked
    else
        setCheckState(m_checkState == Checked ? Unchecked : Checked);
}

void QTuiCheckBox::checkStateSet()
{
    // setChecked() through the base class: follow it, but keep a partial
    // state that already matches "not checked"
    if ((m_checkState == Checked) != isChecked())
        setCheckState(isChecked() ? Checked : Unchecked);
}

bool QTuiCheckBox::isTristate() const
{
    return m_tristate;
}

void QTuiCheckBox::setTristate(bool tristate)
{
    m_tristate = tristate;
}

QSize QTuiCheckBox::minimumSize() const
{
    // "[x] " = 4 chars + text length
    return QSize(4 + text().length(), 1);
}

void QTuiCheckBox::paintEvent(ScreenBuffer &buffer)
{
    switch (m_checkState) {
    case Checked:          paintIndicator(buffer, QStringLiteral("[x]")); break;
    case PartiallyChecked: paintIndicator(buffer, QStringLiteral("[-]")); break;
    default:               paintIndicator(buffer, QStringLiteral("[ ]")); break;
    }
}

} // namespace QtTUI
