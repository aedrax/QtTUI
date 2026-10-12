#ifndef QTTUI_QTUIABSTRACTBUTTON_H
#define QTTUI_QTUIABSTRACTBUTTON_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QPointer>
#include <QString>

class QTimer;

namespace QtTUI {

struct Cell;
class QTuiButtonGroup;

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

    // The group that the button is in, or nullptr
    QTuiButtonGroup *group() const;

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
    // Paints "<indicator> <text>" on the first row, for a check box or a
    // radio button. The indicator has 3 columns; it inverts while the
    // button is down.
    void paintIndicator(ScreenBuffer &buffer, const QString &indicator);
    // The colors of a push button or a tool button for its state: disabled,
    // focused, checked, under the mouse or pressed
    Cell buttonStyle(const ThemeProperties &theme) const;

private:
    QString m_text;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_down = false;
    QTimer *m_releaseTimer;
    QPointer<QTuiButtonGroup> m_group;
    friend class QTuiButtonGroup;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTBUTTON_H
