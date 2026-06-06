#ifndef QTTUI_QTUIPLAINTEXTEDIT_H
#define QTTUI_QTUIPLAINTEXTEDIT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiTextEdit.h>

namespace QtTUI {

// Plain text editor and log viewer, like QPlainTextEdit. It is a
// QTuiTextEdit (all editing works the same) with appendPlainText() and a
// limit on the number of lines (blocks).
//
// For a log, set readOnly and a maximumBlockCount, then call
// appendPlainText() for each message. The view follows new lines while the
// cursor is on the last line; if the user moves up to read, it stays there.
class QTuiPlainTextEdit : public QTuiTextEdit {
    Q_OBJECT
    Q_PROPERTY(int maximumBlockCount READ maximumBlockCount WRITE setMaximumBlockCount)
public:
    explicit QTuiPlainTextEdit(QTuiWidget *parent = nullptr);
    explicit QTuiPlainTextEdit(const QString &text, QTuiWidget *parent = nullptr);

    // Adds text as a new line (or lines) at the end
    void appendPlainText(const QString &text);

    // Maximum number of lines; older lines are removed from the top.
    // 0 (the default) means no limit.
    int maximumBlockCount() const;
    void setMaximumBlockCount(int count);

    int blockCount() const;

private:
    int m_maximumBlockCount = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIPLAINTEXTEDIT_H
