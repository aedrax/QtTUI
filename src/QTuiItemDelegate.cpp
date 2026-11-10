#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>

namespace QtTUI {

QTuiItemDelegate::QTuiItemDelegate(QObject *parent)
    : QObject(parent)
{
}

void QTuiItemDelegate::paint(ScreenBuffer &buffer, const QTuiStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (!index.isValid() || option.rect.isEmpty())
        return;
    renderTextTruncated(buffer, option.rect.x(), option.rect.y(),
                        index.data(Qt::DisplayRole).toString(), option.rect.width(),
                        option.style, option.textElideMode);
}

QTuiWidget *QTuiItemDelegate::createEditor(QTuiWidget * /*parent*/,
                                           const QModelIndex & /*index*/) const
{
    return nullptr;
}

void QTuiItemDelegate::setEditorData(QTuiWidget * /*editor*/,
                                     const QModelIndex & /*index*/) const
{
    // Default implementation does nothing
}

void QTuiItemDelegate::setModelData(QTuiWidget * /*editor*/,
                                    QAbstractItemModel * /*model*/,
                                    const QModelIndex & /*index*/) const
{
    // Default implementation does nothing
}

QSize QTuiItemDelegate::sizeHint(const QModelIndex &index) const
{
    if (!index.isValid())
        return QSize(0, 1);

    QString text = index.data(Qt::DisplayRole).toString();
    return QSize(text.length(), 1);
}

} // namespace QtTUI
