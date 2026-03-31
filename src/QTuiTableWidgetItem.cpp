#include <QtTUI/QTuiTableWidgetItem.h>

namespace QtTUI {

QTuiTableWidgetItem::QTuiTableWidgetItem(const QString &text)
{
    if (!text.isEmpty())
        m_data[Qt::DisplayRole] = text;
}

QString QTuiTableWidgetItem::text() const
{
    return m_data.value(Qt::DisplayRole).toString();
}

void QTuiTableWidgetItem::setText(const QString &text)
{
    m_data[Qt::DisplayRole] = text;
}

Qt::ItemFlags QTuiTableWidgetItem::flags() const
{
    return m_flags;
}

void QTuiTableWidgetItem::setFlags(Qt::ItemFlags flags)
{
    m_flags = flags;
}

QVariant QTuiTableWidgetItem::data(int role) const
{
    return m_data.value(role);
}

void QTuiTableWidgetItem::setData(int role, const QVariant &value)
{
    m_data[role] = value;
}

QTuiTableWidget *QTuiTableWidgetItem::tableWidget() const
{
    return m_tableWidget;
}

int QTuiTableWidgetItem::row() const
{
    return m_row;
}

int QTuiTableWidgetItem::column() const
{
    return m_column;
}

} // namespace QtTUI
