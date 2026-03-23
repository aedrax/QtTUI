#ifndef QTTUI_QTUISTYLEDITEMDELEGATE_H
#define QTTUI_QTUISTYLEDITEMDELEGATE_H

#include <QtTUI/QTuiItemDelegate.h>

namespace QtTUI {

class QTuiStyledItemDelegate : public QTuiItemDelegate {
    Q_OBJECT
public:
    explicit QTuiStyledItemDelegate(QObject *parent = nullptr);

    void paint(ScreenBuffer &buffer, const QRect &rect,
               const QModelIndex &index) const override;
    QTuiWidget *createEditor(QTuiWidget *parent,
                             const QModelIndex &index) const override;
    void setEditorData(QTuiWidget *editor,
                       const QModelIndex &index) const override;
    void setModelData(QTuiWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

} // namespace QtTUI

#endif // QTTUI_QTUISTYLEDITEMDELEGATE_H
