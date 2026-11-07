#ifndef QTTUI_QTUISYNTAXHIGHLIGHTER_H
#define QTTUI_QTUISYNTAXHIGHLIGHTER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/ScreenBuffer.h>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

namespace QtTUI {

class QTuiTextEdit;

// Colors the text of a QTuiTextEdit, like QSyntaxHighlighter (which is in
// Qt GUI). Make a subclass and implement highlightBlock(). A block is one
// line of the text.
//
// The text edit calls highlightBlock() for the lines that it paints, each
// time it paints them. It keeps no formats. It keeps only the block state
// of each line, so that a line can continue the state of the line before
// it (for example a comment over many lines).
class QTuiSyntaxHighlighter : public QObject {
    Q_OBJECT
public:
    // Attaches the highlighter to the text edit. The text edit is the
    // parent and uses one highlighter: the last one made for it.
    explicit QTuiSyntaxHighlighter(QTuiTextEdit *parent);
    ~QTuiSyntaxHighlighter() override;

    QTuiTextEdit *textEdit() const;

public slots:
    // Call this after a change of the rules. The text edit calculates all
    // block states again and repaints.
    void rehighlight();

protected:
    // The text edit calls this function with the text of one line. Call
    // setFormat() for the parts that get a color, and
    // setCurrentBlockState() if the next line must know how this line ends.
    virtual void highlightBlock(const QString &text) = 0;

    // Sets the text color of count UTF-16 code units from start
    void setFormat(int start, int count, const Color &color);
    // Sets the foreground, bold, italic, underline and strikethrough of
    // format. The background stays, so the cursor and the selection show.
    void setFormat(int start, int count, const Cell &format);

    // The state of the line before this line; -1 for the first line and
    // for a line that set no state
    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

private:
    friend class QTuiTextEdit;
    // Runs highlightBlock() for one line and returns its block state.
    // formats has one cell style per code unit of text; nullptr if only
    // the state is necessary.
    int highlightLine(const QString &text, int previousState, QList<Cell> *formats);

    QPointer<QTuiTextEdit> m_edit;
    QList<Cell> *m_formats = nullptr; // set only during highlightLine()
    int m_previousState = -1;
    int m_currentState = -1;
};

} // namespace QtTUI

#endif // QTTUI_QTUISYNTAXHIGHLIGHTER_H
