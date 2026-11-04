#include <QtTUI/QTuiKeySequenceEdit.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/TuiEvents.h>

namespace QtTUI {

QTuiKeySequenceEdit::QTuiKeySequenceEdit(QTuiWidget *parent)
    : QTuiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    connect(this, &QTuiWidget::focusLost, this, &QTuiKeySequenceEdit::stopRecording);
}

QTuiKeySequenceEdit::QTuiKeySequenceEdit(const QTuiKeySequence &keySequence, QTuiWidget *parent)
    : QTuiKeySequenceEdit(parent)
{
    m_sequence = keySequence;
}

QTuiKeySequence QTuiKeySequenceEdit::keySequence() const
{
    return m_sequence;
}

bool QTuiKeySequenceEdit::isRecording() const
{
    return m_recording;
}

void QTuiKeySequenceEdit::setKeySequence(const QTuiKeySequence &keySequence)
{
    if (m_sequence == keySequence)
        return;
    m_sequence = keySequence;
    markDirty();
    emit keySequenceChanged(m_sequence);
}

void QTuiKeySequenceEdit::clear()
{
    setKeySequence(QTuiKeySequence());
}

QSize QTuiKeySequenceEdit::minimumSize() const
{
    // Space for "Ctrl+Alt+Shift+F12"
    return QSize(18, 1);
}

void QTuiKeySequenceEdit::startRecording()
{
    if (m_recording)
        return;
    m_recording = true;

    // The main window uses many keys before the focus widget gets them
    // (shortcuts, F10, Escape in a dialog). A filter on the top-level
    // widget gives these keys to the record first.
    QTuiWidget *top = this;
    while (top->parentWidget())
        top = top->parentWidget();
    if (top != this) {
        m_filtered = top;
        top->installEventFilter(this);
    }
    markDirty();
}

void QTuiKeySequenceEdit::stopRecording()
{
    if (!m_recording)
        return;
    m_recording = false;
    if (m_filtered)
        m_filtered->removeEventFilter(this);
    m_filtered = nullptr;
    markDirty();
}

bool QTuiKeySequenceEdit::eventFilter(QObject *, QEvent *event)
{
    if (!m_recording || event->type() != TuiEventTypes::keyEventType())
        return false;

    auto *key = static_cast<TuiKeyEvent *>(event);
    // Tab always leaves the field: the top-level widget moves the focus
    if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) {
        stopRecording();
        return false;
    }
    recordKey(key);
    return true;
}

void QTuiKeySequenceEdit::recordKey(TuiKeyEvent *event)
{
    const int key = event->key();
    const bool bare = event->modifiers() == Qt::NoModifier;

    if (bare && key == Qt::Key_Escape) {
        stopRecording();
        return;
    }
    if (bare && (key == Qt::Key_Backspace || key == Qt::Key_Delete)) {
        stopRecording();
        clear();
        emit editingFinished();
        return;
    }

    const QTuiKeySequence sequence(key == Qt::Key_Enter ? int(Qt::Key_Return) : key,
                                   event->modifiers());
    // A key without a name cannot be a shortcut text: wait for the next key
    if (sequence.toString().isEmpty())
        return;

    stopRecording();
    setKeySequence(sequence);
    emit editingFinished();
}

void QTuiKeySequenceEdit::keyPressEvent(TuiKeyEvent *event)
{
    // Without a top-level widget above the field, the keys of the record
    // come here
    if (m_recording) {
        recordKey(event);
        return;
    }

    const int key = event->key();
    if (event->modifiers() != Qt::NoModifier)
        return;
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        startRecording();
    } else if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
        clear();
        emit editingFinished();
    }
}

void QTuiKeySequenceEdit::mousePressEvent(TuiMouseEvent *event)
{
    if (event->mouseType() == TuiMouseEvent::Press && event->button() == Qt::LeftButton)
        startRecording();
}

void QTuiKeySequenceEdit::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.isEmpty())
        return;
    clearBackground(buffer, geo);

    const auto theme = themeProperties();
    const bool focused = hasFocus();
    Cell style = cellStyle(theme, focused ? theme.focusForeground : theme.foreground,
                           focused ? theme.focusBackground : theme.background);
    fillRect(buffer, QRect(geo.x(), geo.y(), geo.width(), 1), style);

    // The hints use the muted color, or italics on the focus colors (as
    // the placeholder of QTuiLineEdit does)
    QString text = m_sequence.toString();
    if (m_recording || text.isEmpty()) {
        text = m_recording ? tr("Press a key") : focused ? tr("Enter: record") : tr("None");
        if (focused)
            style.italic = true;
        else
            style.foreground = theme.disabledForeground;
    }
    renderTextTruncated(buffer, geo.x(), geo.y(), text, geo.width(), style);
}

} // namespace QtTUI
