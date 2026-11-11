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

    // The Qt::CheckStateRole data, as in Qt. An item shows a check box
    // after setCheckState() (or setData()) gives it a state. checkState()
    // of an item without a state is Qt::Unchecked.
    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    QTuiListWidget *listWidget() const;

private:
    friend class QTuiListWidget;
    QHash<int, QVariant> m_data;
    Qt::ItemFlags m_flags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    QTuiListWidget *m_listWidget = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUILISTWIDGETITEM_H
