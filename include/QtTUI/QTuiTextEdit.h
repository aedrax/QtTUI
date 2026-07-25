#ifndef QTTUI_QTUITEXTEDIT_H
#define QTTUI_QTUITEXTEDIT_H

#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <QStringList>

namespace QtTUI {

class QTuiTextEdit : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
public:
    explicit QTuiTextEdit(QTuiWidget *parent = nullptr);

    QString toPlainText() const;
    void setPlainText(const QString &text);

    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    int cursorRow() const;
    int cursorColumn() const;

    // Selection
    bool hasSelection() const;
    QString selectedText() const;
    // Selects the whole text (Ctrl+A); the cursor goes to the end
    void selectAll();

    // Undo history: Ctrl+Z undoes, Ctrl+Y and Ctrl+Shift+Z redo. Typed
    // characters in a row undo as one step. setPlainText() clears it.
    void undo();
    void redo();
    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;
    // Bare printable-key shortcuts must not fire while this widget has focus
    bool acceptsTextInput() const override { return true; }
    QPoint terminalCursorPos() const override;

signals:
    void textChanged();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;
    void pasteEvent(TuiPasteEvent *event) override;

    // Adds text as new lines at the end, then removes lines from the top
    // while there are more than maxLines (0: no limit). If the cursor is on
    // the last line, it moves to the new end, so the view follows new text;
    // otherwise the view stays where the user scrolled.
    void appendLines(const QString &text, int maxLines);
    int lineCount() const;

private:
    void editKey(TuiKeyEvent *event);
    // Records an undo step if the text differs from before
    void recordEdit(const QStringList &before, int row, int col, bool typing);
    void restoreState(const QStringList &lines, int row, int col);

    void insertText(const QString &text);
    void insertPastedText(const QString &text);
    void insertNewLine();
    void deleteBackward();
    void deleteForward();
    void deleteSelection();
    void moveCursor(int dRow, int dCol);
    void scrollToVisible();
    void clearSelection();
    void startOrExtendSelection();
    void normalizeSelection(int &startRow, int &startCol, int &endRow, int &endCol) const;

    QStringList m_lines;
    int m_cursorRow = 0;
    int m_cursorCol = 0;  // code-unit index into the line (cluster boundary)
    int m_scrollRow = 0;
    int m_scrollCol = 0;  // display columns scrolled off the left edge
    int m_desiredCol = 0; // sticky DISPLAY column for vertical cursor movement
    bool m_readOnly = false;

    // Selection state
    int m_selStartRow = -1, m_selStartCol = -1;
    int m_selEndRow = -1, m_selEndCol = -1;
    bool m_selecting = false;

    struct UndoState {
        QStringList lines;
        int row;
        int col;
    };
    QList<UndoState> m_undoStack;
    QList<UndoState> m_redoStack;
    // Cursor after the last typed character; -1 if the last edit was not typing
    int m_typingRow = -1;
    int m_typingCol = -1;
};

} // namespace QtTUI

#endif // QTTUI_QTUITEXTEDIT_H
