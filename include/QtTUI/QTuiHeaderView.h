#ifndef QTTUI_QTUIHEADERVIEW_H
#define QTTUI_QTUIHEADERVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>
#include <QList>
#include <QHash>
#include <QSet>
#include <QPersistentModelIndex>

class QAbstractItemModel;

namespace QtTUI {

// The sections of a header, as in QHeaderView. QTuiTableView keeps a
// QTuiHeaderView for its columns (horizontalHeader()) and one for its rows
// (verticalHeader()). The resize modes and the section sizes of the
// horizontal header set the column widths. The table gives each header its
// place and paints it with paintSections() and paintSection(); such a
// header is not a focus stop and does not take the mouse.
//
// The header keeps a size, a resize mode and a hidden state only for the
// sections that have one, so a vertical header of a model with millions of
// rows costs nothing for each row.
class QTuiHeaderView : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation)
    Q_PROPERTY(bool stretchLastSection READ stretchLastSection WRITE setStretchLastSection)
public:
    // How a table view sets the width of a column (the values are the same
    // as in Qt):
    //   Interactive       The user can resize the column (a drag on the
    //                     header, Alt+Left, Alt+Right). Before a resize,
    //                     the width comes from the contents.
    //   Stretch           The column gets a part of the free width. The
    //                     user cannot resize it.
    //   Fixed             The width is sectionSize(). The user cannot
    //                     resize it; resizeSection() can.
    //   ResizeToContents  The width comes from the header and the visible
    //                     rows. The user cannot resize it.
    // If the columns are too wide for the view, all of them get narrower.
    enum ResizeMode { Interactive, Stretch, Fixed, ResizeToContents };
    Q_ENUM(ResizeMode)

    explicit QTuiHeaderView(Qt::Orientation orientation, QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    // The parent of the columns or rows that the header counts, as in Qt
    void setRootIndex(const QModelIndex &index);
    QModelIndex rootIndex() const;

    Qt::Orientation orientation() const;

    int sectionSize(int logicalIndex) const;
    void resizeSection(int logicalIndex, int size);

    bool isSectionHidden(int logicalIndex) const;
    void setSectionHidden(int logicalIndex, bool hide);

    int count() const;

    // Sets the mode of all sections. The default is Interactive.
    void setSectionResizeMode(ResizeMode mode);
    void setSectionResizeMode(int logicalIndex, ResizeMode mode);
    ResizeMode sectionResizeMode(int logicalIndex) const;
    // True after resizeSection() set the size of the section
    bool isSectionResized(int logicalIndex) const;

    // With stretchLastSection, the last section also gets the free width,
    // as a Stretch section does. The default is false.
    bool stretchLastSection() const;
    void setStretchLastSection(bool stretch);

    // The sort indicator, as in QHeaderView: "▲" (ascending) or "▼"
    // (descending) after the text of one section. It shows only if
    // isSortIndicatorShown(). The default is not shown, and section -1.
    void setSortIndicator(int logicalIndex, Qt::SortOrder order);
    int sortIndicatorSection() const;
    Qt::SortOrder sortIndicatorOrder() const;
    bool isSortIndicatorShown() const;
    void setSortIndicatorShown(bool show);

    // Fills rect with style and writes the text of the section and its
    // sort indicator. A text that is too long ends in "…".
    void paintSection(ScreenBuffer &buffer, const QRect &rect, int logicalIndex,
                      const Cell &style) const;
    // Paints the sections from `first` in the row `rect`, from left to
    // right. sizes has the width of each section. `spacing` cells separate
    // two sections. A hidden section takes no space. If the header has the
    // focus, its focused section has the focus colors.
    void paintSections(ScreenBuffer &buffer, const QRect &rect, int first,
                       const QList<int> &sizes, int spacing, const Cell &style) const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void sectionClicked(int logicalIndex);
    void sectionResized(int logicalIndex, int oldSize, int newSize);
    // A resize mode or stretchLastSection changed, or resizeSection() set
    // a section for the first time
    void geometriesChanged();

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    Cell sectionStyle(int logicalIndex, Cell style) const;
    void initSections();
    int nextVisibleSection(int from, int direction) const;

    QAbstractItemModel *m_model = nullptr;
    QPersistentModelIndex m_rootIndex;
    Qt::Orientation m_orientation;
    // The sizes of the sections that resizeSection() set
    QHash<int, int> m_sectionSizes;
    int m_sortSection = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_sortIndicatorShown = false;
    QSet<int> m_hiddenSections;
    QSet<int> m_resizedSections;
    QHash<int, ResizeMode> m_resizeModes;
    ResizeMode m_defaultResizeMode = Interactive;
    bool m_stretchLastSection = false;
    int m_focusedSection = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIHEADERVIEW_H
