#ifndef QTTUI_QTUIHEADERVIEW_H
#define QTTUI_QTUIHEADERVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QList>
#include <QHash>
#include <QSet>

class QAbstractItemModel;

namespace QtTUI {

// The sections of a header, as in QHeaderView. QTuiTableView keeps a
// hidden QTuiHeaderView for its columns (horizontalHeader()): the resize
// modes and the section sizes of that header set the column widths, and
// the table paints the header row itself.
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
    void initSections();
    int nextVisibleSection(int from, int direction) const;

    QAbstractItemModel *m_model = nullptr;
    Qt::Orientation m_orientation;
    QList<int> m_sectionSizes;
    QSet<int> m_hiddenSections;
    QSet<int> m_resizedSections;
    QHash<int, ResizeMode> m_resizeModes;
    ResizeMode m_defaultResizeMode = Interactive;
    bool m_stretchLastSection = false;
    int m_focusedSection = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIHEADERVIEW_H
