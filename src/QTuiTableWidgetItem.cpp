#include <QtTUI/QTuiTableWidgetItem.h>
#include <QtTUI/QTuiTableWidget.h>

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
    if (m_tableWidget)
        m_tableWidget->syncItemToModel(m_row, m_column);
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
    if (m_tableWidget)
        m_tableWidget->syncItemToModel(m_row, m_column);
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
