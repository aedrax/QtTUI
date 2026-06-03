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

private:
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
};

} // namespace QtTUI

#endif // QTTUI_QTUITEXTEDIT_H
