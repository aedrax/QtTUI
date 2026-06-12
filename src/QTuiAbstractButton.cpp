#include <QtTUI/QTuiAbstractButton.h>
#include <QtTUI/TuiEvents.h>
#include <QTimer>

namespace QtTUI {

// Time that the pressed look stays visible after a click
static constexpr int kPressedLookMs = 150;

QTuiAbstractButton::QTuiAbstractButton(QTuiWidget *parent)
    : QTuiWidget(parent)
    , m_releaseTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    m_releaseTimer->setSingleShot(true);
    m_releaseTimer->setInterval(kPressedLookMs);
    connect(m_releaseTimer, &QTimer::timeout, this, [this]() { setDown(false); });
}

QString QTuiAbstractButton::text() const
{
    return m_text;
}

void QTuiAbstractButton::setText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    emit textChanged(m_text);
    markDirty();
}

bool QTuiAbstractButton::isCheckable() const
{
    return m_checkable;
}

void QTuiAbstractButton::setCheckable(bool checkable)
{
    if (m_checkable == checkable)
        return;
    m_checkable = checkable;
    markDirty();
}

bool QTuiAbstractButton::isChecked() const
{
    return m_checked;
}

void QTuiAbstractButton::setChecked(bool checked)
{
    if (m_checked == checked)
        return;
    m_checked = checked;
    checkStateSet();
    emit toggled(m_checked);
    markDirty();
}

bool QTuiAbstractButton::isDown() const
{
    return m_down;
}

void QTuiAbstractButton::setDown(bool down)
{
    if (m_down == down)
        return;
    m_down = down;
    markDirty();
}

void QTuiAbstractButton::animateClick()
{
    setDown(true);
    m_releaseTimer->start();
    click();
}

void QTuiAbstractButton::click()
{
    if (!isEffectivelyEnabled())
        return;
    emit clicked();
    if (m_checkable)
        nextCheckState();
}

void QTuiAbstractButton::toggle()
{
    if (m_checkable)
        setChecked(!m_checked);
}

void QTuiAbstractButton::nextCheckState()
{
    setChecked(!m_checked);
}

void QTuiAbstractButton::checkStateSet()
{
}

void QTuiAbstractButton::keyPressEvent(TuiKeyEvent *event)
{
    // The base event() already drops events for disabled widgets; this guard
    // covers direct handler invocation (subclasses, tests).
    if (!isEffectivelyEnabled())
        return;

    const int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space)
        animateClick();
}

void QTuiAbstractButton::mousePressEvent(TuiMouseEvent *event)
{
    if (!isEffectivelyEnabled() || event->mouseType() != TuiMouseEvent::Press)
        return;
    animateClick();
}

} // namespace QtTUI
