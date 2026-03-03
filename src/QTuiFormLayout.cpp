#include <QtTUI/QTuiFormLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiWidget.h>

#include <algorithm>

namespace QtTUI {

QTuiFormLayout::QTuiFormLayout(QObject *parent)
    : QTuiLayout(parent)
{
}

QTuiFormLayout::~QTuiFormLayout()
{
    for (const auto &row : m_rows) {
        if (row.ownsLabel)
            delete row.label;
    }
}

void QTuiFormLayout::addRow(const QString &labelText, QTuiWidget *field)
{
    auto *label = new QTuiLabel(labelText);
    m_rows.append({label, field, true});
}

void QTuiFormLayout::addRow(QTuiWidget *label, QTuiWidget *field)
{
    m_rows.append({label, field, false});
}

int QTuiFormLayout::rowCount() const
{
    return m_rows.size();
}

void QTuiFormLayout::apply(const QRect &availableArea)
{
    if (m_rows.isEmpty())
        return;

    const QRect rect = contentsRect(availableArea);
    if (rect.isEmpty())
        return;

    // Pass 1: compute max label width
    int maxLabelWidth = 0;
    for (const auto &row : m_rows) {
        if (row.label) {
            const int w = row.label->minimumSize().width();
            maxLabelWidth = std::max(maxLabelWidth, w);
        }
    }

    // Ensure at least 1 column for fields
    const int totalWidth = rect.width();
    if (maxLabelWidth >= totalWidth)
        maxLabelWidth = totalWidth - 1;
    if (maxLabelWidth < 0)
        maxLabelWidth = 0;

    const int fieldWidth = totalWidth - maxLabelWidth;
    const int x = rect.x();
    int currentY = rect.y();

    // Pass 2: lay out each row
    for (const auto &row : m_rows) {
        const int labelH = row.label ? row.label->minimumSize().height() : 0;
        const int fieldH = row.field ? row.field->minimumSize().height() : 0;
        const int rowHeight = std::max({1, labelH, fieldH});

        if (row.label)
            row.label->setGeometry(QRect(x, currentY, maxLabelWidth, rowHeight));

        if (row.field)
            row.field->setGeometry(QRect(x + maxLabelWidth, currentY, fieldWidth, rowHeight));

        currentY += rowHeight;
    }
}

} // namespace QtTUI
