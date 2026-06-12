#ifndef QTTUI_QTUIINPUTDIALOG_H
#define QTTUI_QTUIINPUTDIALOG_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiDialog.h>
#include <QStringList>

namespace QtTUI {

class QTuiComboBox;
class QTuiLabel;
class QTuiLineEdit;
class QTuiSpinBox;

// Dialog that asks the user for one value, like QInputDialog: a text
// (line edit), an integer (spin box), or an item from a list (combo box).
// The static functions are the usual way to use it. OK accepts, Cancel or
// Escape rejects. In text mode, Enter in the line edit also accepts.
class QTuiInputDialog : public QTuiDialog {
    Q_OBJECT
public:
    enum InputMode { TextInput, IntInput, ItemInput };

    explicit QTuiInputDialog(QTuiWidget *parent = nullptr);

    InputMode inputMode() const;
    void setInputMode(InputMode mode);

    QString labelText() const;
    void setLabelText(const QString &text);

    QString textValue() const;
    void setTextValue(const QString &text);

    int intValue() const;
    void setIntValue(int value);
    void setIntRange(int minimum, int maximum);
    void setIntStep(int step);

    // Sets the list for ItemInput mode
    void setComboBoxItems(const QStringList &items);

    // Each returns the value; *ok (if given) is true if the user chose OK
    static QString getText(QTuiWidget *parent, const QString &title, const QString &label,
                           const QString &text = QString(), bool *ok = nullptr);
    static int getInt(QTuiWidget *parent, const QString &title, const QString &label,
                      int value = 0, int minimum = -2147483647, int maximum = 2147483647,
                      int step = 1, bool *ok = nullptr);
    static QString getItem(QTuiWidget *parent, const QString &title, const QString &label,
                           const QStringList &items, int current = 0, bool *ok = nullptr);

private:
    void rebuild();

    InputMode m_mode = TextInput;
    QString m_labelText;
    QString m_text;
    int m_intValue = 0;
    int m_intMin = -2147483647;
    int m_intMax = 2147483647;
    int m_intStep = 1;
    QStringList m_items;
    QTuiLabel *m_label = nullptr;
    QTuiLineEdit *m_lineEdit = nullptr;
    QTuiSpinBox *m_spinBox = nullptr;
    QTuiComboBox *m_comboBox = nullptr;
    QTuiWidget *m_buttonRow = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUIINPUTDIALOG_H
