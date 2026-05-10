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
    // Owned labels may now also have a QObject parent (they are parented into
    // the widget tree so they get painted). Disconnect the destroyed watcher
    // and null the entry before deleting, so the deletion does not re-enter
    // our own bookkeeping while m_rows is being torn down.
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].ownsLabel && m_rows[i].label) {
            QTuiWidget *label = m_rows[i].label;
            disconnect(label, &QObject::destroyed, this, nullptr);
            m_rows[i].label = nullptr;
            delete label;
        }
    }
}

void QTuiFormLayout::addRow(const QString &labelText, QTuiWidget *field)
{
    // Parent the created label into the widget tree (next to the field) so
    // the render pass, which walks childWidgets(), actually paints it. If the
    // field has no parent yet, apply() reparents the label later.
    QTuiWidget *labelParent = field ? field->parentWidget() : nullptr;
    auto *label = new QTuiLabel(labelText, labelParent);
    m_rows.append({label, field, true});
    watchRowWidgets(m_rows.last());

    // Invalidate the layout's parent widget so the entire content area is repainted
    if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
        pw->markDirty();
    }
    invalidate();
}

void QTuiFormLayout::addRow(QTuiWidget *label, QTuiWidget *field)
{
    m_rows.append({label, field, false});
    watchRowWidgets(m_rows.last());

    // Invalidate the layout's parent widget so the entire content area is repainted
    if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
        pw->markDirty();
    }
    invalidate();
}

int QTuiFormLayout::rowCount() const
{
    return m_rows.size();
}

int QTuiFormLayout::count() const
{
    return m_rows.size();
}

void QTuiFormLayout::removeRow(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;

    const auto &r = m_rows[row];
    if (r.field)
        disconnect(r.field, &QObject::destroyed, this, nullptr);
    if (r.label)
        disconnect(r.label, &QObject::destroyed, this, nullptr);
    if (r.ownsLabel)
        delete r.label;

    m_rows.removeAt(row);

    // Invalidate the layout's parent widget so the entire content area is repainted
    if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
        pw->markDirty();
    }
    invalidate();
}

void QTuiFormLayout::watchRowWidgets(const FormRow &row)
{
    // Drop or null out entries if their widgets are destroyed while still in
    // the layout. Owned labels are watched too: since they are parented into
    // the widget tree, their QObject parent can destroy them before we do.
    if (row.field) {
        QTuiWidget *field = row.field;
        connect(field, &QObject::destroyed, this, [this, field]() {
            for (int i = 0; i < m_rows.size(); ++i) {
                if (m_rows[i].field == field) {
                    if (m_rows[i].ownsLabel)
                        delete m_rows[i].label; // its watcher nulls the entry
                    m_rows.removeAt(i);
                    break;
                }
            }
            if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
                pw->markDirty();
            }
            invalidate();
        });
    }

    if (row.label) {
        QTuiWidget *label = row.label;
        connect(label, &QObject::destroyed, this, [this, label]() {
            for (int i = 0; i < m_rows.size(); ++i) {
                if (m_rows[i].label == label) {
                    m_rows[i].label = nullptr;
                    break;
                }
            }
            if (auto *pw = qobject_cast<QTuiWidget *>(parent())) {
                pw->markDirty();
            }
            invalidate();
        });
    }
}

QSize QTuiFormLayout::minimumSize() const
{
    int maxLabelWidth = 0;
    int maxFieldWidth = 0;
    int totalHeight = 0;

    for (const auto &row : m_rows) {
        const QSize labelMin = row.label ? row.label->minimumSize() : QSize(0, 0);
        const QSize fieldMin = row.field ? row.field->minimumSize() : QSize(0, 0);
        maxLabelWidth = std::max(maxLabelWidth, labelMin.width());
        maxFieldWidth = std::max(maxFieldWidth, fieldMin.width());
        totalHeight += std::max({1, labelMin.height(), fieldMin.height()});
    }

    return QSize(maxLabelWidth + maxFieldWidth, totalHeight)
           + QSize(m_marginLeft + m_marginRight, m_marginTop + m_marginBottom);
}

void QTuiFormLayout::apply(const QRect &availableArea)
{
    recordAppliedRect(availableArea);

    if (m_rows.isEmpty())
        return;

    // Late reparenting: an owned label created while its field was still
    // parentless must end up in the widget tree once the field is parented,
    // otherwise the render pass never paints it.
    for (const auto &row : m_rows) {
        if (row.ownsLabel && row.label && !row.label->parent()
            && row.field && row.field->parentWidget()) {
            row.label->setParent(row.field->parentWidget());
        }
    }

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
