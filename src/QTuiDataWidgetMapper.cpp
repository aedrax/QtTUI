#include <QtTUI/QTuiDataWidgetMapper.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/QTuiDoubleSpinBox.h>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/QTuiCheckBox.h>
#include <QAbstractItemModel>
#include <QMetaProperty>

namespace QtTUI {

// Property used to move data between the model and a mapped widget.
// Mirrors the widget-type dispatch in connectWidgetSignals(): the names
// must match each widget's Q_PROPERTY declarations exactly.
static QByteArray mappedPropertyName(const QTuiWidget *widget)
{
    if (qobject_cast<const QTuiSpinBox *>(widget))
        return QByteArrayLiteral("value");
    if (qobject_cast<const QTuiDoubleSpinBox *>(widget))
        return QByteArrayLiteral("value");
    if (qobject_cast<const QTuiComboBox *>(widget))
        return QByteArrayLiteral("currentIndex");
    if (qobject_cast<const QTuiCheckBox *>(widget))
        return QByteArrayLiteral("checkState");
    return QByteArrayLiteral("text");
}

QTuiDataWidgetMapper::QTuiDataWidgetMapper(QObject *parent)
    : QObject(parent)
{
}

void QTuiDataWidgetMapper::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);

    m_model = model;
    m_currentIndex = -1;

    if (m_model) {
        // Guard against the model being deleted while still set on the mapper
        connect(m_model, &QObject::destroyed, this, [this]() {
            m_model = nullptr;
            m_currentIndex = -1;
        });
        // Keep mapped widgets in sync when the model changes underneath us
        // (mirrors QDataWidgetMapper behavior)
        connect(m_model, &QAbstractItemModel::dataChanged,
                this, &QTuiDataWidgetMapper::onModelDataChanged);
    }
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

    // Drop the mapping if the widget is destroyed while still mapped
    connect(widget, &QObject::destroyed, this, [this, widget]() {
        for (int i = 0; i < m_mappings.size(); ++i) {
            if (m_mappings[i].widget == widget) {
                m_mappings.removeAt(i);
                break;
            }
        }
    });

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
            disconnect(widget, &QObject::destroyed, this, nullptr);
            m_mappings.removeAt(i);
            return;
        }
    }
}

void QTuiDataWidgetMapper::clearMapping()
{
    for (const auto &m : m_mappings) {
        disconnectWidgetSignals(m.widget);
        disconnect(m.widget, &QObject::destroyed, this, nullptr);
    }
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
        QVariant value = m.widget->property(mappedPropertyName(m.widget).constData());
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

    for (const auto &m : m_mappings)
        populateWidget(m.widget, m.section);
}

void QTuiDataWidgetMapper::populateWidget(QTuiWidget *widget, int section)
{
    if (!m_model || m_currentIndex < 0)
        return;

    QModelIndex idx = m_model->index(m_currentIndex, section);
    if (!idx.isValid())
        return;

    QVariant value = m_model->data(idx, Qt::EditRole);
    if (!value.isValid())
        value = m_model->data(idx, Qt::DisplayRole);

    const QByteArray prop = mappedPropertyName(widget);
    if (prop == QByteArrayLiteral("text"))
        widget->setProperty(prop.constData(), value.toString());
    else
        widget->setProperty(prop.constData(), value);
}

void QTuiDataWidgetMapper::onModelDataChanged(const QModelIndex &topLeft,
                                              const QModelIndex &bottomRight)
{
    if (!m_model || m_currentIndex < 0)
        return;
    if (topLeft.parent().isValid())
        return; // the mapper only maps top-level rows
    if (m_currentIndex < topLeft.row() || m_currentIndex > bottomRight.row())
        return;

    // Repopulate only the widgets mapped to changed sections
    for (const auto &m : m_mappings) {
        if (m.section >= topLeft.column() && m.section <= bottomRight.column())
            populateWidget(m.widget, m.section);
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
            QVariant value = m.widget->property(mappedPropertyName(m.widget).constData());
            QModelIndex idx = m_model->index(m_currentIndex, m.section);
            if (idx.isValid())
                m_model->setData(idx, value, Qt::EditRole);
            break;
        }
    }
}

void QTuiDataWidgetMapper::connectWidgetSignals(QTuiWidget *widget)
{
    // Connect the first change signal the widget has: line edits, spin
    // boxes, double spin boxes, combo boxes, check boxes
    static const char *const signatures[] = {
        "textChanged(QString)", "valueChanged(int)", "valueChanged(double)",
        "currentIndexChanged(int)", "stateChanged(int)",
    };
    const QMetaObject *mo = widget->metaObject();
    for (const char *signature : signatures) {
        const int signalIdx = mo->indexOfSignal(signature);
        if (signalIdx >= 0) {
            connect(widget, mo->method(signalIdx), this,
                    metaObject()->method(metaObject()->indexOfSlot("onWidgetTextChanged()")));
            return;
        }
    }
}

void QTuiDataWidgetMapper::disconnectWidgetSignals(QTuiWidget *widget)
{
    disconnect(widget, nullptr, this, SLOT(onWidgetTextChanged()));
}

} // namespace QtTUI
