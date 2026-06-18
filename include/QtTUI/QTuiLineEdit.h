#ifndef QTTUI_QTUILINEEDIT_H
#define QTTUI_QTUILINEEDIT_H

#include <QtTUI/QTuiWidget.h>
#include <QPointer>
#include <QString>

namespace QtTUI {

class QTuiCompleter;

class QTuiLineEdit : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
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

    QString m_text;
    int m_cursorPos = 0;     // code-unit index into m_text (cluster boundary)
    int m_scrollOffset = 0;  // display columns scrolled off the left edge
    EchoMode m_echoMode = Normal;
    QString m_placeholderText;
    bool m_readOnly = false;
    QPointer<QTuiCompleter> m_completer;
};

} // namespace QtTUI

#endif // QTTUI_QTUILINEEDIT_H
