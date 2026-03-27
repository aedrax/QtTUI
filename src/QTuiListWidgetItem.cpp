#include <QtTUI/QTuiListWidgetItem.h>
#include <QtTUI/QTuiListWidget.h>

namespace QtTUI {

QTuiListWidgetItem::QTuiListWidgetItem(const QString &text, QTuiListWidget *parent)
    : m_listWidget(nullptr)
{
    if (!text.isEmpty())
        m_data[Qt::DisplayRole] = text;

    if (parent)
        parent->addItem(this);
}

QString QTuiListWidgetItem::text() const
{
    return m_data.value(Qt::DisplayRole).toString();
}

void QTuiListWidgetItem::setText(const QString &text)
{
    m_data[Qt::DisplayRole] = text;
    if (m_listWidget)
        m_listWidget->syncItemToModel(this);
}

QString QTuiListWidgetItem::icon() const
{
    return m_data.value(Qt::DecorationRole).toString();
}

void QTuiListWidgetItem::setIcon(const QString &icon)
{
    m_data[Qt::DecorationRole] = icon;
    if (m_listWidget)
        m_listWidget->syncItemToModel(this);
}

Qt::ItemFlags QTuiListWidgetItem::flags() const
{
    return m_flags;
}

void QTuiListWidgetItem::setFlags(Qt::ItemFlags flags)
{
    m_flags = flags;
}

int QTuiListWidgetItem::checkState() const
{
    return m_checkState;
}

void QTuiListWidgetItem::setCheckState(int state)
{
    m_checkState = state;
    if (m_listWidget)
        m_listWidget->syncItemToModel(this);
}

QVariant QTuiListWidgetItem::data(int role) const
{
    return m_data.value(role);
}

void QTuiListWidgetItem::setData(int role, const QVariant &value)
{
    m_data[role] = value;
    if (m_listWidget)
        m_listWidget->syncItemToModel(this);
}

QTuiListWidget *QTuiListWidgetItem::listWidget() const
{
    return m_listWidget;
}

} // namespace QtTUI
