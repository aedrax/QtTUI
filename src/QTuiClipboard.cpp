#include <QtTUI/QTuiClipboard.h>
#include <QtTUI/QTuiApplication.h>
#include <QtTUI/TerminalBackend.h>

namespace QtTUI {

QTuiClipboard::QTuiClipboard(QObject *parent)
    : QObject(parent)
{
}

QString QTuiClipboard::text() const
{
    return m_text;
}

void QTuiClipboard::setText(const QString &text)
{
    m_text = text;

    auto *app = QTuiApplication::instance();
    if (app && app->terminalInitialized() && app->backend())
        app->backend()->write(osc52Sequence(text));

    emit dataChanged();
}

void QTuiClipboard::clear()
{
    setText(QString());
}

QByteArray QTuiClipboard::osc52Sequence(const QString &text)
{
    // OSC 52 ; c (clipboard selection) ; base64 data BEL
    return QByteArrayLiteral("\x1b]52;c;") + text.toUtf8().toBase64() + '\a';
}

} // namespace QtTUI
