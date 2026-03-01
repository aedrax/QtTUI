#ifndef QTTUI_QTUIBOXLAYOUT_H
#define QTTUI_QTUIBOXLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiLayout.h>
#include <QList>

namespace QtTUI {

class QTuiWidget;

class QTuiBoxLayout : public QTuiLayout {
    Q_OBJECT
public:
    enum Direction { TopToBottom, LeftToRight };

    explicit QTuiBoxLayout(Direction direction, QObject *parent = nullptr);

    void addWidget(QTuiWidget *widget, int stretch = 0);
    void removeWidget(QTuiWidget *widget);

    void apply(const QRect &availableArea) override;

    Direction direction() const;
    int count() const;

    struct Item {
        QTuiWidget *widget;
        int stretch;
    };

    const QList<Item> &items() const;

private:
    Direction m_direction;
    QList<Item> m_boxItems;
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
