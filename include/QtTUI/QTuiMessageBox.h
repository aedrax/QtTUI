#ifndef QTTUI_QTUIMESSAGEBOX_H
#define QTTUI_QTUIMESSAGEBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiDialog.h>
#include <QHash>

namespace QtTUI {

class QTuiLabel;
class QTuiPushButton;

// Message dialog with standard buttons, like QMessageBox.
//
// exec() and the static functions return the button that the user chose.
// Escape returns Cancel, else No, else the only button.
class QTuiMessageBox : public QTuiDialog {
    Q_OBJECT
public:
    enum Icon { NoIcon, Information, Warning, Critical, Question };

    // Same values as QMessageBox::StandardButton
    enum StandardButton {
        NoButton = 0x00000000,
        Ok = 0x00000400,
        Yes = 0x00004000,
        No = 0x00010000,
        Cancel = 0x00400000,
    };
    Q_DECLARE_FLAGS(StandardButtons, StandardButton)

    explicit QTuiMessageBox(QTuiWidget *parent = nullptr);
    QTuiMessageBox(Icon icon, const QString &title, const QString &text,
                   StandardButtons buttons = Ok, QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    Icon icon() const;
    void setIcon(Icon icon);

    StandardButtons standardButtons() const;
    void setStandardButtons(StandardButtons buttons);

    // The button that has focus when the box opens
    void setDefaultButton(StandardButton button);

    // The button that the user chose, or NoButton while the box is open
    StandardButton clickedButton() const;

    // Shows the box and returns the chosen StandardButton
    int exec();

    static StandardButton information(QTuiWidget *parent, const QString &title,
                                      const QString &text, StandardButtons buttons = Ok,
                                      StandardButton defaultButton = NoButton);
    static StandardButton warning(QTuiWidget *parent, const QString &title,
                                  const QString &text, StandardButtons buttons = Ok,
                                  StandardButton defaultButton = NoButton);
    static StandardButton critical(QTuiWidget *parent, const QString &title,
                                   const QString &text, StandardButtons buttons = Ok,
                                   StandardButton defaultButton = NoButton);
    static StandardButton question(QTuiWidget *parent, const QString &title,
                                   const QString &text,
                                   StandardButtons buttons = StandardButtons(Yes | No),
                                   StandardButton defaultButton = NoButton);

protected:
    QTuiWidget *initialFocusWidget() const override;

private:
    void rebuild();
    StandardButton escapeButton() const;

    QString m_text;
    Icon m_icon = NoIcon;
    StandardButtons m_buttons = Ok;
    StandardButton m_defaultButton = NoButton;
    StandardButton m_clicked = NoButton;
    QTuiLabel *m_label = nullptr;
    QTuiWidget *m_buttonRow = nullptr; // owns the buttons
    QHash<StandardButton, QTuiPushButton *> m_buttonWidgets;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTuiMessageBox::StandardButtons)

} // namespace QtTUI

#endif // QTTUI_QTUIMESSAGEBOX_H
