#ifndef QTTUI_QTUIABSTRACTBUTTON_H
#define QTTUI_QTUIABSTRACTBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>

class QTimer;

namespace QtTUI {

// Base class of the button widgets, like QAbstractButton: text, the
// checkable and checked state, the pressed look, and click handling.
//
// Enter, Space and a mouse press call animateClick(). A subclass paints
// the button and can change what a click does to the checked state with
// nextCheckState().
class QTuiAbstractButton : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
    explicit QTuiAbstractButton(QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool isChecked() const;
    void setChecked(bool checked);

    // Pressed look. Terminals do not report key releases, so a click shows
    // the pressed look for a short time (see animateClick()).
    bool isDown() const;
    void setDown(bool down);

public slots:
    // Shows the pressed look, does the click, then releases the button
    // after a short time
    void animateClick();
    // Does the click: changes the checked state (nextCheckState()) of a
    // checkable button, then emits clicked()
    void click();
    // Inverts the checked state of a checkable button
    void toggle();

signals:
    void clicked();
    void toggled(bool checked);
    void textChanged(const QString &text);

protected:
    // Called by click() for a checkable button. The default inverts the
    // checked state.
    virtual void nextCheckState();
    // Called after setChecked() changes the checked state, so a subclass
    // with more states (QTuiCheckBox) can follow
    virtual void checkStateSet();

    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    QString m_text;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_down = false;
    QTimer *m_releaseTimer;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTBUTTON_H
