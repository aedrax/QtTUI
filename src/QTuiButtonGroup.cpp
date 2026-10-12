#include <QtTUI/QTuiButtonGroup.h>
#include <QtTUI/QTuiAbstractButton.h>
#include <QDebug>
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

    auto *abstractButton = qobject_cast<QTuiAbstractButton *>(button);
    if (!abstractButton) {
        qWarning() << "QTuiButtonGroup::addButton: not a QTuiAbstractButton, ignored";
        return;
    }

    m_buttons.append({button, id});
    abstractButton->m_group = this;
    connect(abstractButton, &QTuiAbstractButton::toggled, this, &QTuiButtonGroup::onButtonToggled);
    connect(abstractButton, &QTuiAbstractButton::clicked, this, &QTuiButtonGroup::onButtonClicked);
}

void QTuiButtonGroup::removeButton(QTuiWidget *button)
{
    if (!button)
        return;

    for (int i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons.at(i).button == button) {
            m_buttons.removeAt(i);
            disconnect(button, nullptr, this, nullptr);
            if (auto *abstractButton = qobject_cast<QTuiAbstractButton *>(button))
                abstractButton->m_group = nullptr;
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
        auto *button = qobject_cast<QTuiAbstractButton *>(entry.button);
        if (button && button->isChecked())
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
                if (auto *other = qobject_cast<QTuiAbstractButton *>(entry.button))
                    other->setChecked(false);
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


} // namespace QtTUI
