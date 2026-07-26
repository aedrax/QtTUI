#ifndef QTTUI_QTUIABSTRACTITEMVIEW_H
#define QTTUI_QTUIABSTRACTITEMVIEW_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiAbstractScrollArea.h>
#include <QAbstractItemModel>

namespace QtTUI {

// Base class of the model/view widgets, like QAbstractItemView: the
// model, the current index, and the activated() and doubleClicked()
// signals. Each view connects to the model signals that it needs in
// setModel(), and reads the model each time; it does not copy the data.
class QTuiAbstractItemView : public QTuiAbstractScrollArea {
    Q_OBJECT
public:
    explicit QTuiAbstractItemView(QTuiWidget *parent = nullptr);

    virtual void setModel(QAbstractItemModel *model) = 0;
    QAbstractItemModel *model() const;

    virtual QModelIndex currentIndex() const = 0;
    virtual void setCurrentIndex(const QModelIndex &index) = 0;

signals:
    // Enter on an item, or a click or double click (it depends on the view)
    void activated(const QModelIndex &index);
    // A double click on an item
    void doubleClicked(const QModelIndex &index);

protected:
    QAbstractItemModel *m_model = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTITEMVIEW_H
