#include <QtTUI/QTuiAbstractButton.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/RenderUtils.h>
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
    // Reduced motion: no pressed-look flash
    if (!QTuiApplication::reducedMotion()) {
        setDown(true);
        m_releaseTimer->start();
    }
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
    // Only the left button clicks, as in Qt; the right button asks for a
    // context menu
    if (!isEffectivelyEnabled() || event->mouseType() != TuiMouseEvent::Press
        || event->button() != Qt::LeftButton)
        return;
    animateClick();
}

void QTuiAbstractButton::paintIndicator(ScreenBuffer &buffer, const QString &indicator)
{
    const QRect geo = geometry();
    if (!isVisible() || geo.isEmpty())
        return;

    // Determine colors based on focus state
    const auto theme = themeProperties();
    Color fg = hasFocus() ? theme.focusForeground : theme.foreground;
    Color bg = hasFocus() ? theme.focusBackground : theme.background;

    const Cell style = cellStyle(theme, fg, bg);
    fillRect(buffer, geo, style);
    const int indicatorWidth = renderText(buffer, geo.x(), geo.y(), indicator, geo.width(),
                                          (isDown() && isEffectivelyEnabled()) ? cellStyle(theme, bg, fg) : style);
    renderText(buffer, geo.x() + indicatorWidth, geo.y(), QLatin1Char(' ') + text(),
               geo.width() - indicatorWidth, style);
}

Cell QTuiAbstractButton::buttonStyle(const ThemeProperties &theme) const
{
    // A disabled ancestor also gives this button the disabled colors
    const bool enabled = isEffectivelyEnabled();
    Color fg = theme.foreground;
    Color bg = theme.background;

    if (!enabled) {
        fg = theme.disabledForeground;
        bg = theme.disabledBackground;
    } else if (hasFocus()) {
        fg = theme.focusForeground;
        bg = theme.focusBackground;
    }

    // If checked, use inverse colors as visual indicator
    if (isCheckable() && isChecked() && enabled) {
        std::swap(fg, bg);
    }

    // Pressed: invert the colors again
    const bool pressed = isDown() && enabled;

    // Under the mouse (and not focused or pressed): the accent color shows
    // that a click goes here
    if (underMouse() && enabled && !hasFocus() && !pressed && !(isCheckable() && isChecked()))
        fg = theme.focusBackground;
    if (pressed)
        std::swap(fg, bg);

    return cellStyle(theme, fg, bg);
}

} // namespace QtTUI
