#include <QtTUI/QTuiDataWidgetMapper.h>
#include <QtTUI/QTuiWidget.h>
#include <QAbstractItemModel>
#include <QMetaProperty>

namespace QtTUI {

QTuiDataWidgetMapper::QTuiDataWidgetMapper(QObject *parent)
    : QObject(parent)
{
}

void QTuiDataWidgetMapper::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;
    m_model = model;
    m_currentIndex = -1;
}

QAbstractItemModel *QTuiDataWidgetMapper::model() const
{
    return m_model;
}

void QTuiDataWidgetMapper::addMapping(QTuiWidget *widget, int section)
{
    if (!widget)
        return;

    // Remove any existing mapping for this widget
    removeMapping(widget);

    m_mappings.append({widget, section});

    if (m_submitPolicy == AutoSubmit)
        connectWidgetSignals(widget);

    // If we already have a current index, populate this widget immediately
    if (m_model && m_currentIndex >= 0)
        populate();
}

void QTuiDataWidgetMapper::removeMapping(QTuiWidget *widget)
{
    for (int i = 0; i < m_mappings.size(); ++i) {
        if (m_mappings[i].widget == widget) {
            disconnectWidgetSignals(widget);
            m_mappings.removeAt(i);
            return;
        }
    }
}

void QTuiDataWidgetMapper::clearMapping()
{
    for (const auto &m : m_mappings)
        disconnectWidgetSignals(m.widget);
    m_mappings.clear();
}

int QTuiDataWidgetMapper::currentIndex() const
{
    return m_currentIndex;
}

QTuiDataWidgetMapper::SubmitPolicy QTuiDataWidgetMapper::submitPolicy() const
{
    return m_submitPolicy;
}

void QTuiDataWidgetMapper::setSubmitPolicy(SubmitPolicy policy)
{
    if (m_submitPolicy == policy)
        return;

    // Disconnect/reconnect widget signals based on new policy
    if (policy == AutoSubmit) {
        for (const auto &m : m_mappings)
            connectWidgetSignals(m.widget);
    } else {
        for (const auto &m : m_mappings)
            disconnectWidgetSignals(m.widget);
    }

    m_submitPolicy = policy;
}

void QTuiDataWidgetMapper::toFirst()
{
    setCurrentIndex(0);
}

void QTuiDataWidgetMapper::toLast()
{
    if (!m_model)
        return;
    setCurrentIndex(m_model->rowCount() - 1);
}

void QTuiDataWidgetMapper::toNext()
{
    setCurrentIndex(m_currentIndex + 1);
}

void QTuiDataWidgetMapper::toPrevious()
{
    setCurrentIndex(m_currentIndex - 1);
}

void QTuiDataWidgetMapper::setCurrentIndex(int index)
{
    if (!m_model)
        return;
    if (index < 0 || index >= m_model->rowCount())
        return;
    if (index == m_currentIndex)
        return;

    m_currentIndex = index;
    populate();
    emit currentIndexChanged(m_currentIndex);
}

bool QTuiDataWidgetMapper::submit()
{
    if (!m_model || m_currentIndex < 0)
        return false;

    bool allOk = true;
    for (const auto &m : m_mappings) {
        QVariant value = m.widget->property("text");
        QModelIndex idx = m_model->index(m_currentIndex, m.section);
        if (idx.isValid()) {
            if (!m_model->setData(idx, value, Qt::EditRole))
                allOk = false;
        }
    }
    return allOk;
}

void QTuiDataWidgetMapper::revert()
{
    populate();
}

void QTuiDataWidgetMapper::populate()
{
    if (!m_model || m_currentIndex < 0)
        return;

    for (const auto &m : m_mappings) {
        QModelIndex idx = m_model->index(m_currentIndex, m.section);
        if (idx.isValid()) {
            QVariant value = m_model->data(idx, Qt::EditRole);
            if (!value.isValid())
                value = m_model->data(idx, Qt::DisplayRole);
            m.widget->setProperty("text", value.toString());
        }
    }
}

void QTuiDataWidgetMapper::onWidgetTextChanged()
{
    if (m_submitPolicy != AutoSubmit || !m_model || m_currentIndex < 0)
        return;

    auto *senderWidget = qobject_cast<QTuiWidget *>(sender());
    if (!senderWidget)
        return;

    for (const auto &m : m_mappings) {
        if (m.widget == senderWidget) {
            QVariant value = m.widget->property("text");
            QModelIndex idx = m_model->index(m_currentIndex, m.section);
            if (idx.isValid())
                m_model->setData(idx, value, Qt::EditRole);
            break;
        }
    }
}

void QTuiDataWidgetMapper::connectWidgetSignals(QTuiWidget *widget)
{
    // Look for a textChanged signal on the widget using meta-object
    const QMetaObject *mo = widget->metaObject();
    int signalIdx = mo->indexOfSignal("textChanged(QString)");
    if (signalIdx >= 0) {
        const QMetaMethod signal = mo->method(signalIdx);
        const QMetaMethod slot = metaObject()->method(
            metaObject()->indexOfSlot("onWidgetTextChanged()"));
        connect(widget, signal, this, slot);
        return;
    }
    // Also try valueChanged(int) for spin boxes etc.
    signalIdx = mo->indexOfSignal("valueChanged(int)");
    if (signalIdx >= 0) {
        const QMetaMethod signal = mo->method(signalIdx);
        const QMetaMethod slot = metaObject()->method(
            metaObject()->indexOfSlot("onWidgetTextChanged()"));
        connect(widget, signal, this, slot);
        return;
    }
    // Try valueChanged(double) for double spin boxes
    signalIdx = mo->indexOfSignal("valueChanged(double)");
    if (signalIdx >= 0) {
        const QMetaMethod signal = mo->method(signalIdx);
        const QMetaMethod slot = metaObject()->method(
            metaObject()->indexOfSlot("onWidgetTextChanged()"));
        connect(widget, signal, this, slot);
    }
}

void QTuiDataWidgetMapper::disconnectWidgetSignals(QTuiWidget *widget)
{
    disconnect(widget, nullptr, this, SLOT(onWidgetTextChanged()));
}

} // namespace QtTUI
