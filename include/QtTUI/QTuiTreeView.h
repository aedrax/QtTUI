#ifndef QTTUI_QTUITREEVIEW_H
#define QTTUI_QTUITREEVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QSet>

namespace QtTUI {

class QTuiItemDelegate;

class QTuiTreeView : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiTreeView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);

    void expand(const QModelIndex &index);
    void collapse(const QModelIndex &index);
    bool isExpanded(const QModelIndex &index) const;

    void setItemDelegate(QTuiItemDelegate *delegate);
    QTuiItemDelegate *itemDelegate() const;

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void activated(const QModelIndex &index);
    void expanded(const QModelIndex &index);
    void collapsed(const QModelIndex &index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onModelReset();

private:
    struct FlattenedItem {
        QModelIndex index;
        int depth;
        bool expanded;
    };

    void rebuildFlatList();
    void buildFlatListRecursive(const QModelIndex &parent, int depth);
    void disconnectModel();
    void connectModel();
    void scrollToVisible();
    int flatRowForIndex(const QModelIndex &index) const;
    void markDirtyFromRowToBottom(int flatRow);

    QAbstractItemModel *m_model = nullptr;
    QTuiItemDelegate *m_delegate = nullptr;
    QList<FlattenedItem> m_flatList;
    QSet<QPersistentModelIndex> m_expandedItems;
    int m_currentRow = 0;
    int m_scrollOffset = 0;
    static constexpr int m_indentWidth = 2;
};

} // namespace QtTUI

#endif // QTTUI_QTUITREEVIEW_H
