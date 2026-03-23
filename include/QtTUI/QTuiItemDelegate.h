#ifndef QTTUI_QTUIITEMDELEGATE_H
#define QTTUI_QTUIITEMDELEGATE_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QModelIndex>

class QAbstractItemModel;

namespace QtTUI {

class QTuiWidget;
class ScreenBuffer;

class QTuiItemDelegate : public QObject {
    Q_OBJECT
public:
    explicit QTuiItemDelegate(QObject *parent = nullptr);

    // Painting
    virtual void paint(ScreenBuffer &buffer, const QRect &rect,
                       const QModelIndex &index) const;

    // Editor lifecycle
    virtual QTuiWidget *createEditor(QTuiWidget *parent,
                                     const QModelIndex &index) const;
    virtual void setEditorData(QTuiWidget *editor,
                               const QModelIndex &index) const;
    virtual void setModelData(QTuiWidget *editor, QAbstractItemModel *model,
                              const QModelIndex &index) const;

    // Size
    virtual QSize sizeHint(const QModelIndex &index) const;

signals:
    void commitData(QTuiWidget *editor);
    void closeEditor(QTuiWidget *editor);
};

} // namespace QtTUI

#endif // QTTUI_QTUIITEMDELEGATE_H
