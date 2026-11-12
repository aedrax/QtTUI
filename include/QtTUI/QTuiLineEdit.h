#ifndef QTTUI_QTUILINEEDIT_H
#define QTTUI_QTUILINEEDIT_H

#include <QtTUI/QTuiWidget.h>
#include <QPointer>
#include <QString>

namespace QtTUI {

class QTuiCompleter;
class QTuiValidator;

class QTuiLineEdit : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask)
public:
    // How the text shows, as in QLineEdit. Password shows one "•" per
    // character; NoEcho shows nothing and keeps the cursor at the start.
    enum EchoMode { Normal, NoEcho, Password };
    Q_ENUM(EchoMode)

    explicit QTuiLineEdit(QTuiWidget *parent = nullptr);

    EchoMode echoMode() const;
    void setEchoMode(EchoMode mode);
    // The text as it shows on the screen
    QString displayText() const;

    // Muted hint text that shows while the line edit is empty
    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    // A read-only line edit moves the cursor and emits returnPressed(), but
    // the user cannot change the text. setText() still works.
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    // Maximum text length in UTF-16 code units (default 32767). Longer
    // typed or pasted text is cut; setText() also cuts.
    int maxLength() const;
    void setMaxLength(int length);

    // Input mask, as in QLineEdit: a template with one position for each
    // character. The mask characters are A a N n X x 9 0 D d # H h B b; an
    // uppercase one (and 9) needs a character, a lowercase one (and 0 and
    // #) can stay blank. > and < change the case of the letters that
    // follow, ! stops that, \ makes the next character a separator, and
    // ";c" at the end sets the blank character to c (default: a space).
    // All other characters are separators. With a mask, displayText() is
    // the full template and text() is the same without the blanks. An
    // empty mask (the default) removes the mask.
    QString inputMask() const;
    void setInputMask(const QString &inputMask);

    // Rejects edits that make the text Invalid. The line edit does not take
    // ownership of the validator.
    const QTuiValidator *validator() const;
    void setValidator(const QTuiValidator *validator);
    // True if each position of the mask that needs a character has one,
    // and there is no validator or it says Acceptable
    bool hasAcceptableInput() const;

    // Selection, as in QLineEdit. Keys: Shift+Left/Right/Home/End select,
    // Ctrl+A selects all; Ctrl+C copies, Ctrl+X cuts, Ctrl+V pastes. In
    // Password and NoEcho mode, copy and cut do nothing.
    bool hasSelectedText() const;
    QString selectedText() const;
    int selectionStart() const; // -1 if nothing is selected
    void setSelection(int start, int length);
    void selectAll();
    void deselect();
    void copy() const;
    void cut();
    void paste();

    // Undo history, as in QLineEdit: Ctrl+Z undoes, Ctrl+Y and
    // Ctrl+Shift+Z redo. Typed characters in a row undo as one step.
    // setText() clears the history.
    void undo();
    void redo();
    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    QString text() const;
    void setText(const QString &text);

    int cursorPosition() const;
    void setCursorPosition(int pos);

    // The line edit does not take ownership of the completer
    QTuiCompleter *completer() const;
    void setCompleter(QTuiCompleter *completer);

    void paintEvent(ScreenBuffer &buffer) override;

    QSize minimumSize() const override;
    // Bare printable-key shortcuts must not fire while this widget has focus
    bool acceptsTextInput() const override { return true; }
    QPoint terminalCursorPos() const override;

signals:
    void textChanged(const QString &text);
    void returnPressed();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;
    void pasteEvent(TuiPasteEvent *event) override;

private:
    void adjustScrollToCursor();
    // Maps a cursor index between m_text and displayText()
    int displayIndex(int textIndex) const;
    int textIndex(int displayIndex) const;
    void insertPastedText(const QString &text);
    // Every user edit goes through here: replaces [start, end) with text,
    // puts the cursor after it, and emits textChanged(). Returns false and
    // changes nothing if the edit is not allowed (read-only).
    bool replaceRange(int start, int end, const QString &text);
    // Replaces the selection (or inserts at the cursor) with text
    bool replaceSelection(const QString &text);

    // One position of the input mask
    struct MaskCell {
        QChar ch;       // the mask character, or the separator itself
        bool separator;
        quint8 caseMode; // 0: as typed, 1: uppercase, 2: lowercase
    };
    // True if the mask position of this type takes ch
    bool maskAccepts(QChar type, QChar ch) const;
    // Puts the characters of text into buffer from position pos on, each in
    // the next position that takes it. Returns the position after the last
    // one, or -1 if the mask takes nothing of text.
    int maskInsert(QString &buffer, int pos, const QString &text) const;
    // The buffer without the blanks
    QString stripMask(const QString &buffer) const;
    // The first position from pos that takes a character
    int skipSeparators(int pos) const;

    QString m_text;          // with a mask: one character per mask position
    int m_cursorPos = 0;     // code-unit index into m_text (cluster boundary)
    int m_scrollOffset = 0;  // display columns scrolled off the left edge
    EchoMode m_echoMode = Normal;
    QString m_placeholderText;
    bool m_readOnly = false;
    int m_maxLength = 32767;
    int m_selAnchor = -1; // the fixed end of the selection; -1 for none
    QString m_inputMask;
    QList<MaskCell> m_mask; // empty: no mask
    QChar m_blank = QLatin1Char(' ');

    struct UndoState {
        QString text;
        int cursor;
    };
    QList<UndoState> m_undoStack;
    QList<UndoState> m_redoStack;
    int m_typingEnd = -1; // cursor after the last typed character, else -1
    QPointer<const QTuiValidator> m_validator;
    QPointer<QTuiCompleter> m_completer;
};

} // namespace QtTUI

#endif // QTTUI_QTUILINEEDIT_H
