#ifndef QTTUI_QTUIITEMDELEGATE_H
#define QTTUI_QTUIITEMDELEGATE_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QModelIndex>
#include <QtTUI/ScreenBuffer.h>

class QAbstractItemModel;

namespace QtTUI {

class QTuiWidget;

// What a view gives to QTuiItemDelegate::paint(), like QStyleOptionViewItem
struct QTuiStyleOptionViewItem {
    // The area of the item text, one line high
    QRect rect;
    // The colors and attributes of the item, from the itemStyle() of the view
    Cell style;
    Qt::TextElideMode textElideMode = Qt::ElideRight;
};

// Paints the items of a view and makes their editors, like
// QAbstractItemDelegate. The list, table and tree views call paint() for
// each item that shows.
class QTuiItemDelegate : public QObject {
    Q_OBJECT
public:
    explicit QTuiItemDelegate(QObject *parent = nullptr);

    // Writes the item in option.rect with option.style: the check box
    // ("[ ]", "[x]" or "[-]") if the item has a Qt::CheckStateRole value,
    // the Qt::DecorationRole value if it is a string (a glyph), and the
    // Qt::DisplayRole text, with "…" as option.textElideMode tells if the
    // text is too long. The view fills the area with the colors of the
    // item before it calls this function, so an override can write only
    // the text.
    virtual void paint(ScreenBuffer &buffer, const QTuiStyleOptionViewItem &option,
                       const QModelIndex &index) const;

    // Editor lifecycle
    virtual QTuiWidget *createEditor(QTuiWidget *parent,
                                     const QModelIndex &index) const;
    virtual void setEditorData(QTuiWidget *editor,
                               const QModelIndex &index) const;
    virtual void setModelData(QTuiWidget *editor, QAbstractItemModel *model,
                              const QModelIndex &index) const;

    // The width that paint() needs for the item, and a height of 1. The
    // table view calls it to measure a column.
    virtual QSize sizeHint(const QModelIndex &index) const;

    // The text that paint() writes before the item text: the check box
    // and the decoration, each with a space after it. Empty if the item
    // has none.
    static QString decorationText(const QModelIndex &index);

signals:
    void commitData(QTuiWidget *editor);
    void closeEditor(QTuiWidget *editor);
};

} // namespace QtTUI

#endif // QTTUI_QTUIITEMDELEGATE_H
