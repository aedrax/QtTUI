#ifndef QTTUI_QTUIABSTRACTSCROLLAREA_H
#define QTTUI_QTUIABSTRACTSCROLLAREA_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiFrame.h>

namespace QtTUI {

// Base class of widgets that show a part of larger content, like
// QAbstractScrollArea: the scroll area and the item views. It keeps the
// vertical scroll offset (the first content row that shows).
class QTuiAbstractScrollArea : public QTuiFrame {
    Q_OBJECT
public:
    explicit QTuiAbstractScrollArea(QTuiWidget *parent = nullptr);

    // The first content row that shows
    int scrollOffset() const;

protected:
    // Changes the scroll offset as little as possible so that row shows in
    // a viewport of visibleRows rows
    void scrollRowIntoView(int row, int visibleRows);

    int m_scrollOffset = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIABSTRACTSCROLLAREA_H
