#ifndef QTTUI_QTUILINEEDIT_H
#define QTTUI_QTUILINEEDIT_H

#include <QtTUI/QTuiWidget.h>
#include <QString>

namespace QtTUI {

class QTuiLineEdit : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    explicit QTuiLineEdit(QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    int cursorPosition() const;
    void setCursorPosition(int pos);

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void textChanged(const QString &text);
    void returnPressed();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    QString m_text;
    int m_cursorPos = 0;
    int m_scrollOffset = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUILINEEDIT_H
