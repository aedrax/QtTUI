#ifndef QTTUI_QTUIDIALOG_H
#define QTTUI_QTUIDIALOG_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QPointer>
#include <QString>

namespace QtTUI {

// Modal dialog, like QDialog. It is a bordered box with a title, centered
// in the main window, above the other widgets.
//
// While a dialog is open, it is modal: keys and clicks reach only the
// dialog, Tab moves focus only between its widgets, other shortcuts and
// menu mnemonics do not work, and a click outside it does nothing. Escape
// rejects the dialog (unless a popup inside it, such as a completer, is
// open). When the dialog closes, focus goes back to where it was.
//
// Put the content in a layout with setLayout(); the dialog applies it
// inside the border and sizes itself from the layout's minimum size.
class QTuiDialog : public QTuiWidget {
    Q_OBJECT
public:
    enum DialogCode { Rejected, Accepted };

    // The dialog is always a child of the main window, so it can show
    // above everything; parent is used only if no main window exists yet.
    explicit QTuiDialog(QTuiWidget *parent = nullptr);

    QString windowTitle() const;
    void setWindowTitle(const QString &title);

    // Shows the dialog and returns at once; finished() reports the result
    void open();
    // Shows the dialog and waits in a local event loop until it closes.
    // Returns the result, or Rejected if there is no main window or the
    // application quits first.
    int exec();

    int result() const;

    void setLayout(QTuiLayout *layout) override;
    QTuiLayout *layout() const override;

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;
    // Modal: a click outside the dialog does not close it
    void popupOutsideClick() override {}

public slots:
    void accept();
    void reject();
    void done(int result);

signals:
    void finished(int result);
    void accepted();
    void rejected();

protected:
    // The widget that gets focus when the dialog opens. The default is the
    // first widget that takes Tab focus.
    virtual QTuiWidget *initialFocusWidget() const;

private:
    void placeInMainWindow();
    void applyLayout();

    QString m_title;
    QPointer<QTuiLayout> m_layout;
    int m_result = Rejected;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDIALOG_H
