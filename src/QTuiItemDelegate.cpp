#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

namespace QtTUI {

QTuiItemDelegate::QTuiItemDelegate(QObject *parent)
    : QObject(parent)
{
}

void QTuiItemDelegate::paint(ScreenBuffer &buffer, const QRect &rect,
                             const QModelIndex &index) const
{
    if (!index.isValid() || rect.isEmpty())
        return;

    QString text = index.data(Qt::DisplayRole).toString();

    int y = rect.y();
    int x = rect.x();
    int maxWidth = rect.width();

    for (int i = 0; i < text.length() && i < maxWidth; ++i) {
        if (x + i >= 0 && x + i < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(x + i, y);
            cell.setChar(text.at(i).unicode());
        }
    }
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
