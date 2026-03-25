#ifndef QTTUI_QTUIDATAWIDGETMAPPER_H
#define QTTUI_QTUIDATAWIDGETMAPPER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QList>

class QAbstractItemModel;

namespace QtTUI {

class QTuiWidget;

class QTuiDataWidgetMapper : public QObject {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
public:
    enum SubmitPolicy { AutoSubmit, ManualSubmit };

    explicit QTuiDataWidgetMapper(QObject *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    void addMapping(QTuiWidget *widget, int section);
    void removeMapping(QTuiWidget *widget);
    void clearMapping();

    int currentIndex() const;

    SubmitPolicy submitPolicy() const;
    void setSubmitPolicy(SubmitPolicy policy);

public slots:
    void toFirst();
    void toLast();
    void toNext();
    void toPrevious();
    void setCurrentIndex(int index);
    bool submit();
    void revert();

signals:
    void currentIndexChanged(int index);

private slots:
    void onWidgetTextChanged();

private:
    void populate();
    void connectWidgetSignals(QTuiWidget *widget);
    void disconnectWidgetSignals(QTuiWidget *widget);

    QAbstractItemModel *m_model = nullptr;
    struct Mapping {
        QTuiWidget *widget;
        int section;
    };
    QList<Mapping> m_mappings;
    int m_currentIndex = -1;
    SubmitPolicy m_submitPolicy = AutoSubmit;
};

} // namespace QtTUI

#endif // QTTUI_QTUIDATAWIDGETMAPPER_H
