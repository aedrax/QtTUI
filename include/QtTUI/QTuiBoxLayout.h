#ifndef QTTUI_QTUIBOXLAYOUT_H
#define QTTUI_QTUIBOXLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayout.h>
#include <QList>
#include <QSet>

namespace QtTUI {

class QTuiWidget;

class QTuiBoxLayout : public QTuiLayout {
    Q_OBJECT
public:
    enum Direction { TopToBottom, LeftToRight };

    explicit QTuiBoxLayout(Direction direction, QObject *parent = nullptr);

    void addWidget(QTuiWidget *widget, int stretch = 0);
    void removeWidget(QTuiWidget *widget);

    // Empty space: addSpacing() adds a fixed number of cells, addStretch()
    // adds a part that takes free space by its stretch factor.
    void addSpacing(int size);
    void addStretch(int stretch = 0);

    // Cells between two adjacent items (default 0)
    int spacing() const;
    void setSpacing(int spacing);

    void apply(const QRect &availableArea) override;

    Direction direction() const;
    int count() const override;
    QSize minimumSize() const override;

    // widget is nullptr for an item from addSpacing() or addStretch()
    struct Item {
        QTuiWidget *widget;
        int stretch;
        int spacerSize = 0;
    };

    const QList<Item> &items() const;

private:
    bool participatesInLayout(const Item &item) const;
    int itemMinimum(const Item &item, bool vertical) const;

    Direction m_direction;
    QList<Item> m_boxItems;
    int m_spacing = 0;
    // Widgets this layout hid because they did not fit. They stay layout
    // candidates (unlike user-hidden widgets) and are re-shown when space
    // returns.
    QSet<QTuiWidget *> m_layoutHidden;
};

// Convenience factory functions
inline QTuiBoxLayout *createVBoxLayout(QObject *parent = nullptr)
{
    return new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, parent);
}

inline QTuiBoxLayout *createHBoxLayout(QObject *parent = nullptr)
{
    return new QTuiBoxLayout(QTuiBoxLayout::LeftToRight, parent);
}

// Convenience type aliases
using QTuiVBoxLayout = QTuiBoxLayout;
using QTuiHBoxLayout = QTuiBoxLayout;

} // namespace QtTUI

#endif // QTTUI_QTUIBOXLAYOUT_H
