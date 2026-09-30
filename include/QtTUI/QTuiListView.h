#ifndef QTTUI_QTUILISTVIEW_H
#define QTTUI_QTUILISTVIEW_H

#include <QtTUI/QTuiAbstractItemView.h>
#include <QAbstractItemModel>
#include <QModelIndex>

namespace QtTUI {

// A list of model rows (column 0). Each click emits activated();
// a double click also emits doubleClicked().
class QTuiListView : public QTuiAbstractItemView {
    Q_OBJECT
public:
    explicit QTuiListView(QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    QModelIndex currentIndex() const override;
    void setCurrentIndex(const QModelIndex &index) override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QRect visualRect(const QModelIndex &index) const override;
    QRect hoverRect() const override;


    void paintEvent(ScreenBuffer &buffer) override;

signals:

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onLayoutChanged();
    void onModelReset();

private:
    void disconnectModel();
    void connectModel();
    void ensureCurrentVisible();
    void clampCurrentIndex();

    int m_currentRow = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUILISTVIEW_H
