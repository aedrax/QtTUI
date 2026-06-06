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
public:
    explicit QTuiLineEdit(QTuiWidget *parent = nullptr);

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
    void insertPastedText(const QString &text);

    QString m_text;
    int m_cursorPos = 0;     // code-unit index into m_text (cluster boundary)
    int m_scrollOffset = 0;  // display columns scrolled off the left edge
    QPointer<QTuiCompleter> m_completer;
};

} // namespace QtTUI

#endif // QTTUI_QTUILINEEDIT_H
