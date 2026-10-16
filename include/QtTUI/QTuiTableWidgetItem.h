#ifndef QTTUI_QTUITABLEWIDGETITEM_H
#define QTTUI_QTUITABLEWIDGETITEM_H

#include <QHash>
#include <QString>
#include <QVariant>
#include <Qt>

namespace QtTUI {

class QTuiTableWidget;

class QTuiTableWidgetItem {
public:
    explicit QTuiTableWidgetItem(const QString &text = QString());

    QString text() const;
    void setText(const QString &text);

    Qt::ItemFlags flags() const;
    void setFlags(Qt::ItemFlags flags);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    QTuiTableWidget *tableWidget() const;
    int row() const;
    int column() const;

private:
    friend class QTuiTableWidget;
    friend class QTuiTableWidgetModel;
    QHash<int, QVariant> m_data;
    // Editable by default, as in Qt
    Qt::ItemFlags m_flags = Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    QTuiTableWidget *m_tableWidget = nullptr;
    int m_row = -1;
    int m_column = -1;
};

struct QTuiTableWidgetSelectionRange {
    int topRow() const { return m_topRow; }
    int leftColumn() const { return m_leftColumn; }
    int bottomRow() const { return m_bottomRow; }
    int rightColumn() const { return m_rightColumn; }
    int rowCount() const { return m_bottomRow - m_topRow + 1; }
    int columnCount() const { return m_rightColumn - m_leftColumn + 1; }

    int m_topRow = 0, m_leftColumn = 0, m_bottomRow = 0, m_rightColumn = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUITABLEWIDGETITEM_H
