#include <QtTUI/QTuiStyledItemDelegate.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/ScreenBuffer.h>

#include <QAbstractItemModel>

namespace QtTUI {

QTuiStyledItemDelegate::QTuiStyledItemDelegate(QObject *parent)
    : QTuiItemDelegate(parent)
{
}

void QTuiStyledItemDelegate::paint(ScreenBuffer &buffer, const QRect &rect,
                                   const QModelIndex &index) const
{
    if (!index.isValid() || rect.isEmpty())
        return;

    // Query Theme_Engine for styled colors and text attributes
    ThemeProperties props = Theme_Engine::instance()->properties(
        QStringLiteral("QTuiStyledItemDelegate"));

    QString text = index.data(Qt::DisplayRole).toString();

    int y = rect.y();
    int x = rect.x();
    int maxWidth = rect.width();

    // Fill the entire rect with themed background first
    for (int col = 0; col < maxWidth; ++col) {
        int cx = x + col;
        if (cx >= 0 && cx < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(cx, y);
            cell.setChar(U' ');
            cell.foreground = props.foreground;
            cell.background = props.background;
            cell.bold = props.bold;
            cell.italic = props.italic;
            cell.underline = props.underline;
        }
    }

    // Render text characters with themed attributes
    for (int i = 0; i < text.length() && i < maxWidth; ++i) {
        int cx = x + i;
        if (cx >= 0 && cx < buffer.width() && y >= 0 && y < buffer.height()) {
            Cell &cell = buffer.backCell(cx, y);
            cell.setChar(text.at(i).unicode());
        }
    }
}

QTuiWidget *QTuiStyledItemDelegate::createEditor(QTuiWidget *parent,
                                                  const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    QVariant data = index.data(Qt::EditRole);

    // Check for enum-like data: QStringList stored in UserRole
    QVariant userRoleData = index.data(Qt::UserRole);
    if (userRoleData.typeId() == QMetaType::QStringList) {
        auto *combo = new QTuiComboBox(parent);
        combo->addItems(userRoleData.toStringList());
        return combo;
    }

    // Integer data → QTuiSpinBox
    if (data.typeId() == QMetaType::Int || data.typeId() == QMetaType::LongLong) {
        auto *spinBox = new QTuiSpinBox(parent);
        spinBox->setRange(-999999, 999999);
        return spinBox;
    }

    // Default: string data → QTuiLineEdit
    auto *lineEdit = new QTuiLineEdit(parent);
    return lineEdit;
}

void QTuiStyledItemDelegate::setEditorData(QTuiWidget *editor,
                                           const QModelIndex &index) const
{
    if (!editor || !index.isValid())
        return;

    QVariant data = index.data(Qt::EditRole);

    if (auto *combo = qobject_cast<QTuiComboBox *>(editor)) {
        QString text = data.toString();
        int idx = -1;
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemText(i) == text) {
                idx = i;
                break;
            }
        }
        if (idx >= 0)
            combo->setCurrentIndex(idx);
    } else if (auto *spinBox = qobject_cast<QTuiSpinBox *>(editor)) {
        spinBox->setValue(data.toInt());
    } else if (auto *lineEdit = qobject_cast<QTuiLineEdit *>(editor)) {
        lineEdit->setText(data.toString());
    }
}

void QTuiStyledItemDelegate::setModelData(QTuiWidget *editor,
                                          QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    if (!editor || !model || !index.isValid())
        return;

    if (auto *combo = qobject_cast<QTuiComboBox *>(editor)) {
        model->setData(index, combo->currentText(), Qt::EditRole);
    } else if (auto *spinBox = qobject_cast<QTuiSpinBox *>(editor)) {
        model->setData(index, spinBox->value(), Qt::EditRole);
    } else if (auto *lineEdit = qobject_cast<QTuiLineEdit *>(editor)) {
        model->setData(index, lineEdit->text(), Qt::EditRole);
    }
}

} // namespace QtTUI
