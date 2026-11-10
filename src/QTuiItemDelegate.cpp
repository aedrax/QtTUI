#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/QtTuiUnicode.h>

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
    // The check box and the decoration do not get an ellipsis
    const int used = renderText(buffer, option.rect.x(), option.rect.y(), decorationText(index),
                                option.rect.width(), option.style);
    renderTextTruncated(buffer, option.rect.x() + used, option.rect.y(),
                        index.data(Qt::DisplayRole).toString(), option.rect.width() - used,
                        option.style, option.textElideMode);
}

QString QTuiItemDelegate::decorationText(const QModelIndex &index)
{
    QString text;
    const QVariant check = index.data(Qt::CheckStateRole);
    if (check.isValid()) {
        switch (check.toInt()) {
        case Qt::Checked:          text = QStringLiteral("[x] "); break;
        case Qt::PartiallyChecked: text = QStringLiteral("[-] "); break;
        default:                   text = QStringLiteral("[ ] "); break;
        }
    }
    // QIcon is in Qt GUI: only a string (a glyph) shows
    const QVariant decoration = index.data(Qt::DecorationRole);
    if (decoration.userType() == QMetaType::QString && !decoration.toString().isEmpty())
        text += decoration.toString() + QLatin1Char(' ');
    return text;
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
    return QSize(QtTuiUnicode::stringWidth(decorationText(index)) + text.length(), 1);
}

} // namespace QtTUI
