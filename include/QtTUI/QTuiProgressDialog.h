#ifndef QTTUI_QTUIPROGRESSDIALOG_H
#define QTTUI_QTUIPROGRESSDIALOG_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiDialog.h>
#include <QElapsedTimer>

namespace QtTUI {

class QTuiLabel;
class QTuiProgressBar;
class QTuiPushButton;

// Progress of a long operation, like QProgressDialog: a label, a progress
// bar and a Cancel button.
//
// Call setValue() as the work goes on, for example from a timer. The
// dialog shows only when the work takes longer than minimumDuration
// (default 4000 ms) after the first setValue(); a short operation never
// shows it. At maximum() it closes by itself (autoClose). Cancel or
// Escape sets wasCanceled() and emits canceled().
class QTuiProgressDialog : public QTuiDialog {
    Q_OBJECT
public:
    explicit QTuiProgressDialog(QTuiWidget *parent = nullptr);
    QTuiProgressDialog(const QString &labelText, const QString &cancelButtonText,
                       int minimum, int maximum, QTuiWidget *parent = nullptr);

    QString labelText() const;
    void setLabelText(const QString &text);

    int minimum() const;
    int maximum() const;
    void setRange(int minimum, int maximum);

    int value() const;
    void setValue(int value);

    int minimumDuration() const;
    void setMinimumDuration(int ms);

    bool autoClose() const;
    void setAutoClose(bool close);

    bool wasCanceled() const;

public slots:
    void cancel();
    // Hides the dialog if autoClose() is true, sets the value to the
    // minimum and clears wasCanceled(), so that a new operation can start
    void reset();

signals:
    void canceled();

private:
    QTuiLabel *m_label;
    QTuiProgressBar *m_bar;
    QTuiPushButton *m_cancelButton;
    QElapsedTimer m_timer;
    int m_minimumDuration = 4000;
    bool m_autoClose = true;
    bool m_canceled = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUIPROGRESSDIALOG_H
