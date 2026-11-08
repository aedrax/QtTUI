#include <QtTUI/QTuiSyntaxHighlighter.h>
#include <QtTUI/QTuiTextEdit.h>

namespace QtTUI {

QTuiSyntaxHighlighter::QTuiSyntaxHighlighter(QTuiTextEdit *parent)
    : QObject(parent)
    , m_edit(parent)
{
    if (m_edit)
        m_edit->setHighlighter(this);
}

QTuiSyntaxHighlighter::~QTuiSyntaxHighlighter()
{
    // The text shows without colors again. m_edit is null if the text edit
    // deletes the highlighter as its child.
    if (m_edit)
        m_edit->markDirty();
}

QTuiTextEdit *QTuiSyntaxHighlighter::textEdit() const
{
    return m_edit;
}

void QTuiSyntaxHighlighter::rehighlight()
{
    if (m_edit && m_edit->m_highlighter == this)
        m_edit->setHighlighter(this);
}

void QTuiSyntaxHighlighter::setFormat(int start, int count, const Color &color)
{
    if (!m_formats)
        return;
    const int end = int(qMin(qint64(m_formats->size()), qint64(start) + count));
    for (int i = qMax(0, start); i < end; ++i)
        (*m_formats)[i].foreground = color;
}

void QTuiSyntaxHighlighter::setFormat(int start, int count, const Cell &format)
{
    if (!m_formats)
        return;
    const int end = int(qMin(qint64(m_formats->size()), qint64(start) + count));
    for (int i = qMax(0, start); i < end; ++i) {
        Cell &cell = (*m_formats)[i];
        cell.foreground = format.foreground;
        cell.bold = format.bold;
        cell.italic = format.italic;
        cell.underline = format.underline;
        cell.strikethrough = format.strikethrough;
    }
}

int QTuiSyntaxHighlighter::previousBlockState() const
{
    return m_previousState;
}

int QTuiSyntaxHighlighter::currentBlockState() const
{
    return m_currentState;
}

void QTuiSyntaxHighlighter::setCurrentBlockState(int newState)
{
    m_currentState = newState;
}

int QTuiSyntaxHighlighter::highlightLine(const QString &text, int previousState,
                                         QList<Cell> *formats)
{
    m_formats = formats;
    m_previousState = previousState;
    m_currentState = -1;
    highlightBlock(text);
    m_formats = nullptr;
    return m_currentState;
}

} // namespace QtTUI
