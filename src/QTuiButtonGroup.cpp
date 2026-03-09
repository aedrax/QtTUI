#include <QtTUI/QTuiButtonGroup.h>
#include <QtTUI/QTuiWidget.h>
#include <QMetaObject>
#include <QMetaMethod>

namespace QtTUI {

QTuiButtonGroup::QTuiButtonGroup(QObject *parent)
    : QObject(parent)
{
}

void QTuiButtonGroup::addButton(QTuiWidget *button, int id)
{
    if (!button)
        return;

    // Check if already added
    for (const auto &entry : m_buttons) {
        if (entry.button == button)
            return;
    }

    if (id == -1)
        id = m_nextId--;

    m_buttons.append({button, id});

    // Dynamically connect to available signals using the meta-object system.
    // QTuiPushButton and QTuiRadioButton have toggled(bool).
    // QTuiCheckBox has stateChanged(int).
    // QTuiPushButton has clicked().
    const QMetaObject *meta = button->metaObject();

    int toggledIdx = meta->indexOfSignal(QMetaObject::normalizedSignature("toggled(bool)").constData());
    if (toggledIdx >= 0) {
        connect(button, SIGNAL(toggled(bool)), this, SLOT(onButtonToggled(bool)));
    }

    int stateChangedIdx = meta->indexOfSignal(QMetaObject::normalizedSignature("stateChanged(int)").constData());
    if (stateChangedIdx >= 0) {
        connect(button, SIGNAL(stateChanged(int)), this, SLOT(onCheckBoxStateChanged(int)));
    }

    int clickedIdx = meta->indexOfSignal(QMetaObject::normalizedSignature("clicked()").constData());
    if (clickedIdx >= 0) {
        connect(button, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    }
}

void QTuiButtonGroup::removeButton(QTuiWidget *button)
{
    if (!button)
        return;

    for (int i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons.at(i).button == button) {
            m_buttons.removeAt(i);
            disconnect(button, nullptr, this, nullptr);
            return;
        }
    }
}

QList<QTuiWidget *> QTuiButtonGroup::buttons() const
{
    QList<QTuiWidget *> result;
    result.reserve(m_buttons.size());
    for (const auto &entry : m_buttons)
        result.append(entry.button);
    return result;
}

int QTuiButtonGroup::id(QTuiWidget *button) const
{
    for (const auto &entry : m_buttons) {
        if (entry.button == button)
            return entry.id;
    }
    return -1;
}

QTuiWidget *QTuiButtonGroup::button(int id) const
{
    for (const auto &entry : m_buttons) {
        if (entry.id == id)
            return entry.button;
    }
    return nullptr;
}

QTuiWidget *QTuiButtonGroup::checkedButton() const
{
    for (const auto &entry : m_buttons) {
        // Try "checked" property (QTuiPushButton, QTuiRadioButton)
        QVariant checked = entry.button->property("checked");
        if (checked.isValid() && checked.toBool())
            return entry.button;

        // Try "checkState" property (QTuiCheckBox)
        QVariant checkState = entry.button->property("checkState");
        if (checkState.isValid() && checkState.toInt() == 2) // Checked == 2
            return entry.button;
    }
    return nullptr;
}

int QTuiButtonGroup::checkedId() const
{
    QTuiWidget *btn = checkedButton();
    if (btn)
        return id(btn);
    return -1;
}

bool QTuiButtonGroup::exclusive() const
{
    return m_exclusive;
}

void QTuiButtonGroup::setExclusive(bool exclusive)
{
    m_exclusive = exclusive;
}

int QTuiButtonGroup::findId(QTuiWidget *button) const
{
    for (const auto &entry : m_buttons) {
        if (entry.button == button)
            return entry.id;
    }
    return -1;
}

void QTuiButtonGroup::handleToggle(QTuiWidget *button, bool checked)
{
    int buttonId = findId(button);
    if (buttonId == -1) // button not found in group
        return;

    emit buttonToggled(buttonId, checked);

    if (m_exclusive && checked) {
        // Uncheck all other buttons
        for (const auto &entry : m_buttons) {
            if (entry.button != button) {
                // Use property system to uncheck generically.
                // "checked" works for QTuiPushButton and QTuiRadioButton.
                // "checkState" works for QTuiCheckBox.
                entry.button->setProperty("checked", false);
                entry.button->setProperty("checkState", 0);
            }
        }
    }
}

void QTuiButtonGroup::onButtonToggled(bool checked)
{
    auto *button = qobject_cast<QTuiWidget *>(sender());
    if (!button)
        return;

    handleToggle(button, checked);
}

void QTuiButtonGroup::onButtonClicked()
{
    auto *button = qobject_cast<QTuiWidget *>(sender());
    if (!button)
        return;

    int buttonId = findId(button);
    if (buttonId == -1) // not found
        return;

    emit buttonClicked(buttonId);
}

void QTuiButtonGroup::onCheckBoxStateChanged(int state)
{
    auto *button = qobject_cast<QTuiWidget *>(sender());
    if (!button)
        return;

    // state == 2 means Checked, anything else is not checked
    handleToggle(button, state == 2);
}

} // namespace QtTUI
