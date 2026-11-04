#ifndef QTTUI_QTUIKEYSEQUENCEEDIT_H
#define QTTUI_QTUIKEYSEQUENCEEDIT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiKeySequence.h>
#include <QtTUI/QTuiWidget.h>
#include <QPointer>

namespace QtTUI {

// One-line field that records a key combination, like QKeySequenceEdit.
// It holds one combination (QTuiKeySequence has no multi-chord form).
//
// The field does not record while it only has focus, so shortcuts, Escape
// and the menu keys still work when the user moves through it with Tab.
// Enter, Space or a click starts the record. During the record the field
// gets every key before the main window does: the next combination becomes
// the value, also if it is a shortcut of an action or a navigation key.
// These keys keep their function during the record:
// - Tab and Shift+Tab stop the record and move the focus
// - Escape stops the record and keeps the old value
// - Backspace and Delete stop the record and clear the value
// A key that QTuiKeySequence::fromString() cannot read (punctuation) is
// ignored.
class QTuiKeySequenceEdit : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiKeySequenceEdit(QTuiWidget *parent = nullptr);
    explicit QTuiKeySequenceEdit(const QTuiKeySequence &keySequence, QTuiWidget *parent = nullptr);

    QTuiKeySequence keySequence() const;

    // True while the field waits for a key combination
    bool isRecording() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

public slots:
    void setKeySequence(const QTuiKeySequence &keySequence);
    void clear();

signals:
    void keySequenceChanged(const QTuiKeySequence &keySequence);
    // The user recorded a combination or cleared the field
    void editingFinished();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;
    // During the record, takes the keys that go to the top-level widget
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void startRecording();
    void stopRecording();
    void recordKey(TuiKeyEvent *event);

    QTuiKeySequence m_sequence;
    bool m_recording = false;
    QPointer<QObject> m_filtered; // the widget that has the event filter
};

} // namespace QtTUI

#endif // QTTUI_QTUIKEYSEQUENCEEDIT_H
