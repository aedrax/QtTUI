#ifndef QTTUI_QTUILISTWIDGETITEM_H
#define QTTUI_QTUILISTWIDGETITEM_H

#include <QHash>
#include <QString>
#include <QVariant>
#include <Qt>

namespace QtTUI {

class QTuiListWidget;

class QTuiListWidgetItem {
public:
    explicit QTuiListWidgetItem(const QString &text = QString(),
                                QTuiListWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    QString icon() const;
    void setIcon(const QString &icon);

    Qt::ItemFlags flags() const;
    void setFlags(Qt::ItemFlags flags);

    int checkState() const;
    void setCheckState(int state);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    QTuiListWidget *listWidget() const;

private:
    friend class QTuiListWidget;
    QHash<int, QVariant> m_data;
    Qt::ItemFlags m_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    int m_checkState = 0;
    QTuiListWidget *m_listWidget = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUILISTWIDGETITEM_H
