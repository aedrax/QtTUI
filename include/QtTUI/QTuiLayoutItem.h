#ifndef QTTUI_QTUILAYOUTITEM_H
#define QTTUI_QTUILAYOUTITEM_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiSizePolicy.h>
#include <QPointer>
#include <QRect>
#include <QSize>

namespace QtTUI {

class QTuiWidget;
class QTuiLayout;

class QTuiLayoutItem {
public:
    virtual ~QTuiLayoutItem() = default;

    virtual QSize sizeHint() const = 0;
    virtual QSize minimumSize() const = 0;
    virtual QSize maximumSize() const = 0;

    virtual void setGeometry(const QRect &rect) = 0;
    virtual QRect geometry() const = 0;

    QTuiSizePolicy sizePolicy() const;
    void setSizePolicy(const QTuiSizePolicy &policy);

    virtual bool isEmpty() const = 0;

protected:
    QTuiSizePolicy m_sizePolicy;
};

class QTuiWidgetItem : public QTuiLayoutItem {
public:
    explicit QTuiWidgetItem(QTuiWidget *widget);

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;

    bool isEmpty() const override;

    QTuiWidget *widget() const;

private:
    // Guarded pointer: nulls automatically if the widget is destroyed while
    // still referenced by a layout, so the null checks above stay valid.
    QPointer<QTuiWidget> m_widget;
};

class QTuiSpacerItem : public QTuiLayoutItem {
public:
    QTuiSpacerItem(int width, int height,
                   QTuiSizePolicy::Policy hPolicy = QTuiSizePolicy::Minimum,
                   QTuiSizePolicy::Policy vPolicy = QTuiSizePolicy::Minimum);

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;

    bool isEmpty() const override;

private:
    int m_width;
    int m_height;
    QRect m_geometry;
};

class QTuiLayoutItemWrapper : public QTuiLayoutItem {
public:
    explicit QTuiLayoutItemWrapper(QTuiLayout *layout);

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;

    bool isEmpty() const override;

    QTuiLayout *layout() const;

private:
    // Guarded pointer: nulls automatically if the nested layout is destroyed
    // while still referenced, so the null checks above stay valid.
    QPointer<QTuiLayout> m_layout;
    QRect m_geometry;
};

} // namespace QtTUI

#endif // QTTUI_QTUILAYOUTITEM_H
