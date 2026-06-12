#include <QtTUI/QTuiInputDialog.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiSpinBox.h>

namespace QtTUI {

// Minimum width of the input field, in cells
static constexpr int kFieldWidth = 40;

QTuiInputDialog::QTuiInputDialog(QTuiWidget *parent)
    : QTuiDialog(parent)
{
    rebuild();
}

QTuiInputDialog::InputMode QTuiInputDialog::inputMode() const
{
    return m_mode;
}

void QTuiInputDialog::setInputMode(InputMode mode)
{
    m_mode = mode;
    rebuild();
}

QString QTuiInputDialog::labelText() const
{
    return m_labelText;
}

void QTuiInputDialog::setLabelText(const QString &text)
{
    m_labelText = text;
    rebuild();
}

QString QTuiInputDialog::textValue() const
{
    if (m_lineEdit)
        return m_lineEdit->text();
    if (m_comboBox)
        return m_comboBox->currentText();
    return m_text;
}

void QTuiInputDialog::setTextValue(const QString &text)
{
    m_text = text;
    if (m_lineEdit)
        m_lineEdit->setText(text);
    if (m_comboBox) {
        const int index = m_items.indexOf(text);
        if (index >= 0)
            m_comboBox->setCurrentIndex(index);
    }
}

int QTuiInputDialog::intValue() const
{
    return m_spinBox ? m_spinBox->value() : m_intValue;
}

void QTuiInputDialog::setIntValue(int value)
{
    m_intValue = value;
    if (m_spinBox)
        m_spinBox->setValue(value);
}

void QTuiInputDialog::setIntRange(int minimum, int maximum)
{
    m_intMin = minimum;
    m_intMax = maximum;
    if (m_spinBox)
        m_spinBox->setRange(minimum, maximum);
}

void QTuiInputDialog::setIntStep(int step)
{
    m_intStep = step;
    if (m_spinBox)
        m_spinBox->setSingleStep(step);
}

void QTuiInputDialog::setComboBoxItems(const QStringList &items)
{
    m_items = items;
    m_mode = ItemInput;
    rebuild();
}

void QTuiInputDialog::rebuild()
{
    // Keep the current value across a rebuild
    if (m_lineEdit)
        m_text = m_lineEdit->text();
    if (m_spinBox)
        m_intValue = m_spinBox->value();

    delete layout();
    delete m_label;
    delete m_lineEdit;
    delete m_spinBox;
    delete m_comboBox;
    delete m_buttonRow;
    m_lineEdit = nullptr;
    m_spinBox = nullptr;
    m_comboBox = nullptr;

    auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    m_label = new QTuiLabel(m_labelText, this);
    column->addWidget(m_label);

    // The field is at least kFieldWidth wide: a row with a fixed spacing
    // sets the minimum width of the column
    QTuiWidget *field = nullptr;
    switch (m_mode) {
    case TextInput:
        m_lineEdit = new QTuiLineEdit(this);
        m_lineEdit->setText(m_text);
        connect(m_lineEdit, &QTuiLineEdit::returnPressed, this, &QTuiDialog::accept);
        field = m_lineEdit;
        break;
    case IntInput:
        m_spinBox = new QTuiSpinBox(this);
        m_spinBox->setRange(m_intMin, m_intMax);
        m_spinBox->setSingleStep(m_intStep);
        m_spinBox->setValue(m_intValue);
        field = m_spinBox;
        break;
    case ItemInput:
        m_comboBox = new QTuiComboBox(this);
        m_comboBox->addItems(m_items);
        if (const int index = m_items.indexOf(m_text); index >= 0)
            m_comboBox->setCurrentIndex(index);
        field = m_comboBox;
        break;
    }
    column->addWidget(field);
    column->addSpacing(1);

    auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    row->setSpacing(1);
    row->addSpacing(kFieldWidth - 18); // with the two buttons: kFieldWidth
    row->addStretch();
    m_buttonRow = new QTuiWidget(this);
    auto *ok = new QTuiPushButton(QStringLiteral("OK"), m_buttonRow);
    auto *cancel = new QTuiPushButton(QStringLiteral("Cancel"), m_buttonRow);
    connect(ok, &QTuiPushButton::clicked, this, &QTuiDialog::accept);
    connect(cancel, &QTuiPushButton::clicked, this, &QTuiDialog::reject);
    row->addWidget(ok);
    row->addWidget(cancel);
    m_buttonRow->setLayout(row);
    column->addWidget(m_buttonRow);
    setLayout(column);
}

QString QTuiInputDialog::getText(QTuiWidget *parent, const QString &title, const QString &label,
                                 const QString &text, bool *ok)
{
    QTuiInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setTextValue(text);
    const bool accepted = dialog.exec() == Accepted;
    if (ok)
        *ok = accepted;
    return accepted ? dialog.textValue() : QString();
}

int QTuiInputDialog::getInt(QTuiWidget *parent, const QString &title, const QString &label,
                            int value, int minimum, int maximum, int step, bool *ok)
{
    QTuiInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setInputMode(IntInput);
    dialog.setLabelText(label);
    dialog.setIntRange(minimum, maximum);
    dialog.setIntStep(step);
    dialog.setIntValue(value);
    const bool accepted = dialog.exec() == Accepted;
    if (ok)
        *ok = accepted;
    return accepted ? dialog.intValue() : value;
}

QString QTuiInputDialog::getItem(QTuiWidget *parent, const QString &title, const QString &label,
                                 const QStringList &items, int current, bool *ok)
{
    QTuiInputDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setComboBoxItems(items);
    dialog.setTextValue(items.value(current));
    const bool accepted = dialog.exec() == Accepted;
    if (ok)
        *ok = accepted;
    return accepted ? dialog.textValue() : QString();
}

} // namespace QtTUI
