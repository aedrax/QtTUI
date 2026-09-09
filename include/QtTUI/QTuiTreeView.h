#ifndef QTTUI_QTUITREEVIEW_H
#define QTTUI_QTUITREEVIEW_H

#include <QtTUI/QTuiAbstractItemView.h>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QSet>

namespace QtTUI {

class QTuiItemDelegate;

class QTuiTreeView : public QTuiAbstractItemView {
    Q_OBJECT
public:
    explicit QTuiTreeView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    QModelIndex currentIndex() const override;
    void setCurrentIndex(const QModelIndex &index) override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QRect visualRect(const QModelIndex &index) const override;

    void expand(const QModelIndex &index);
    void collapse(const QModelIndex &index);
    bool isExpanded(const QModelIndex &index) const;

    // Selects all visible items in the three multiple modes
    void selectAll() override;


    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void expanded(const QModelIndex &index);
    void collapsed(const QModelIndex &index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;
    QItemSelection selectionRange(const QModelIndex &from, const QModelIndex &to) const override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onLayoutChanged();
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

    QList<FlattenedItem> m_flatList;
    QSet<QPersistentModelIndex> m_expandedItems;
    int m_currentRow = 0;
    // Flat row affected by an in-flight removal, captured in
    // onRowsAboutToBeRemoved while the indexes are still valid.
    int m_pendingRemovalFlatRow = -1;
    static constexpr int m_indentWidth = 2;
};

} // namespace QtTUI

#endif // QTTUI_QTUITREEVIEW_H
