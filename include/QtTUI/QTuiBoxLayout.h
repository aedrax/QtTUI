#ifndef QTTUI_QTUIBOXLAYOUT_H
#define QTTUI_QTUIBOXLAYOUT_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QRect>
#include <QList>

namespace QtTUI {

class QTuiWidget;

class QTuiBoxLayout : public QObject {
    Q_OBJECT
public:
    enum Direction { TopToBottom, LeftToRight };

    explicit QTuiBoxLayout(Direction direction, QObject *parent = nullptr);

    void addWidget(QTuiWidget *widget, int stretch = 0);
    void removeWidget(QTuiWidget *widget);

    void setContentsMargins(int left, int top, int right, int bottom);
    void contentsMargins(int *left, int *top, int *right, int *bottom) const;

    void apply(const QRect &availableArea);

    Direction direction() const;
    int count() const;

    struct Item {
        QTuiWidget *widget;
        int stretch;
    };

    const QList<Item> &items() const;

private:
    Direction m_direction;
    QList<Item> m_items;
    int m_marginLeft = 0;
    int m_marginTop = 0;
    int m_marginRight = 0;
    int m_marginBottom = 0;
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
