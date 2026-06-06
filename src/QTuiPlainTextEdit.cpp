#include <QtTUI/QTuiPlainTextEdit.h>

namespace QtTUI {

QTuiPlainTextEdit::QTuiPlainTextEdit(QTuiWidget *parent)
    : QTuiTextEdit(parent)
{
}

QTuiPlainTextEdit::QTuiPlainTextEdit(const QString &text, QTuiWidget *parent)
    : QTuiTextEdit(parent)
{
    setPlainText(text);
}

void QTuiPlainTextEdit::appendPlainText(const QString &text)
{
    // A null string would only trim; append an empty line instead
    appendLines(text.isNull() ? QStringLiteral("") : text, m_maximumBlockCount);
}

int QTuiPlainTextEdit::maximumBlockCount() const
{
    return m_maximumBlockCount;
}

void QTuiPlainTextEdit::setMaximumBlockCount(int count)
{
    m_maximumBlockCount = qMax(0, count);
    if (m_maximumBlockCount > 0 && blockCount() > m_maximumBlockCount)
        appendLines(QString(), m_maximumBlockCount); // trim only
}

int QTuiPlainTextEdit::blockCount() const
{
    return lineCount();
}

} // namespace QtTUI
