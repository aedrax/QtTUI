#include <QtTUI/QTuiListView.h>
#include <QtTUI/Theme_Engine.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QDebug>

namespace QtTUI {

QTuiListView::QTuiListView(QTuiWidget *parent)
    : QTuiWidget(parent)
{
}

void QTuiListView::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    disconnectModel();
    m_model = model;
    m_currentRow = 0;
    m_scrollOffset = 0;

    if (m_model)
        connectModel();
}

QAbstractItemModel *QTuiListView::model() const
{
    return m_model;
}

QModelIndex QTuiListView::currentIndex() const
{
    if (!m_model || m_model->rowCount() == 0)
        return QModelIndex();
    return m_model->index(m_currentRow, 0);
}

void QTuiListView::setCurrentIndex(const QModelIndex &index)
{
    if (!m_model)
        return;
    m_currentRow = qBound(0, index.row(), qMax(0, m_model->rowCount() - 1));
    ensureCurrentVisible();
}

int QTuiListView::scrollOffset() const
{
    return m_scrollOffset;
}

void QTuiListView::paintEvent(ScreenBuffer &buffer)
{
    const QRect geo = geometry();
    if (geo.width() <= 0 || geo.height() <= 0 || !m_model)
        return;

    const auto theme = themeProperties();
    const int rows = m_model->rowCount();
    const int viewWidth = geo.width();
    const int viewHeight = geo.height();

    for (int vr = 0; vr < viewHeight; ++vr) {
        int modelRow = m_scrollOffset + vr;
        int screenY = geo.y() + vr;

        QString itemText;
        if (modelRow < rows) {
            QModelIndex idx = m_model->index(modelRow, 0);
            itemText = m_model->data(idx, Qt::DisplayRole).toString();
        }

        bool isCurrent = (modelRow == m_currentRow && modelRow < rows);
        for (int x = 0; x < viewWidth; ++x) {
            Cell &cell = buffer.backCell(geo.x() + x, screenY);
            cell.character = (x < itemText.length()) ? itemText.at(x) : QLatin1Char(' ');
            cell.bold = false;
            if (isCurrent) {
                cell.foreground = theme.focusForeground;
                cell.background = theme.focusBackground;
            } else {
                cell.foreground = theme.foreground;
                cell.background = theme.background;
            }
            cell.inverse = isCurrent;
        }
    }
}

void QTuiListView::keyPressEvent(TuiKeyEvent *event)
{
    if (!m_model || m_model->rowCount() == 0)
        return;

    const int rowCount = m_model->rowCount();

    switch (event->key()) {
    case Qt::Key_Up:
        m_currentRow = qMax(0, m_currentRow - 1);
        ensureCurrentVisible();
        break;
    case Qt::Key_Down:
        m_currentRow = qMin(rowCount - 1, m_currentRow + 1);
        ensureCurrentVisible();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit activated(m_model->index(m_currentRow, 0));
        break;
    default:
        break;
    }
}

void QTuiListView::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_model)
        return;

    int top = qBound(0, topLeft.row(), m_model->rowCount() - 1);
    int bottom = qBound(0, bottomRight.row(), m_model->rowCount() - 1);

    if (top > bottom)
        qWarning() << "QTuiListView::onDataChanged: out-of-range indices clamped";

    Q_UNUSED(top);
    Q_UNUSED(bottom);
}

void QTuiListView::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if (!m_model)
        return;

    clampCurrentIndex();
}

void QTuiListView::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if (!m_model)
        return;

    clampCurrentIndex();
    ensureCurrentVisible();
}

void QTuiListView::onModelReset()
{
    m_currentRow = 0;
    m_scrollOffset = 0;
}

void QTuiListView::disconnectModel()
{
    if (!m_model)
        return;

    disconnect(m_model, &QAbstractItemModel::dataChanged,
               this, &QTuiListView::onDataChanged);
    disconnect(m_model, &QAbstractItemModel::rowsInserted,
               this, &QTuiListView::onRowsInserted);
    disconnect(m_model, &QAbstractItemModel::rowsRemoved,
               this, &QTuiListView::onRowsRemoved);
    disconnect(m_model, &QAbstractItemModel::modelReset,
               this, &QTuiListView::onModelReset);
}

void QTuiListView::connectModel()
{
    if (!m_model)
        return;

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, &QTuiListView::onDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QTuiListView::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QTuiListView::onRowsRemoved);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QTuiListView::onModelReset);
}

void QTuiListView::ensureCurrentVisible()
{
    if (!m_model)
        return;

    int visibleRows = geometry().height();
    if (visibleRows <= 0)
        return;

    if (m_currentRow < m_scrollOffset)
        m_scrollOffset = m_currentRow;
    else if (m_currentRow >= m_scrollOffset + visibleRows)
        m_scrollOffset = m_currentRow - visibleRows + 1;
}

void QTuiListView::clampCurrentIndex()
{
    if (!m_model || m_model->rowCount() == 0) {
        m_currentRow = 0;
        return;
    }
    m_currentRow = qBound(0, m_currentRow, m_model->rowCount() - 1);
}

} // namespace QtTUI
