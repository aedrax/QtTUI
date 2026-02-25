#ifndef QTTUI_QTUILISTVIEW_H
#define QTTUI_QTUILISTVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QAbstractItemModel>
#include <QModelIndex>

namespace QtTUI {

class QTuiListView : public QTuiWidget {
    Q_OBJECT
public:
    explicit QTuiListView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex &index);

    int scrollOffset() const;

    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void activated(const QModelIndex &index);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onModelReset();

private:
    void disconnectModel();
    void connectModel();
    void ensureCurrentVisible();
    void clampCurrentIndex();

    QAbstractItemModel *m_model = nullptr;
    int m_currentRow = 0;
    int m_scrollOffset = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUILISTVIEW_H
