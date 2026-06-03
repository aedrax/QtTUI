#ifndef QTTUI_QTUICLIPBOARD_H
#define QTTUI_QTUICLIPBOARD_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QString>

namespace QtTUI {

// Clipboard for copy and paste, like QClipboard.
//
// setText() keeps the text for paste inside the application. It also sends
// the text to the system clipboard with an OSC 52 escape sequence, if a
// QTuiApplication with a terminal exists. Terminals that do not support
// OSC 52 ignore the sequence.
//
// text() returns the last text set in this application. It cannot read the
// system clipboard: most terminals block OSC 52 reads. To paste from the
// system clipboard, the user pastes into the terminal (a TuiPasteEvent).
class QTuiClipboard : public QObject {
    Q_OBJECT
public:
    explicit QTuiClipboard(QObject *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    void clear();

    // The OSC 52 sequence that sets the system clipboard to text
    static QByteArray osc52Sequence(const QString &text);

signals:
    void dataChanged();

private:
    QString m_text;
};

} // namespace QtTUI

#endif // QTTUI_QTUICLIPBOARD_H
