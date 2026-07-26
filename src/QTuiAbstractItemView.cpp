#include <QtTUI/QTuiAbstractItemView.h>

namespace QtTUI {

QTuiAbstractItemView::QTuiAbstractItemView(QTuiWidget *parent)
    : QTuiAbstractScrollArea(parent)
{
}

QAbstractItemModel *QTuiAbstractItemView::model() const
{
    return m_model;
}

} // namespace QtTUI
