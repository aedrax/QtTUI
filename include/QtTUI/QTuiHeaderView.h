#ifndef QTTUI_QTUIHEADERVIEW_H
#define QTTUI_QTUIHEADERVIEW_H

#include <QtTUI/QTuiWidget.h>
#include <QList>
#include <QSet>

class QAbstractItemModel;

namespace QtTUI {

class QTuiHeaderView : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation)
public:
    explicit QTuiHeaderView(Qt::Orientation orientation, QTuiWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    Qt::Orientation orientation() const;

    int sectionSize(int logicalIndex) const;
    void resizeSection(int logicalIndex, int size);

    bool isSectionHidden(int logicalIndex) const;
    void setSectionHidden(int logicalIndex, bool hide);

    int count() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void sectionClicked(int logicalIndex);
    void sectionResized(int logicalIndex, int oldSize, int newSize);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;

private:
    void initSections();
    int nextVisibleSection(int from, int direction) const;

    QAbstractItemModel *m_model = nullptr;
    Qt::Orientation m_orientation;
    QList<int> m_sectionSizes;
    QSet<int> m_hiddenSections;
    int m_focusedSection = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIHEADERVIEW_H
