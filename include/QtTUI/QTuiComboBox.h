#ifndef QTTUI_QTUICOMBOBOX_H
#define QTTUI_QTUICOMBOBOX_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiWidget.h>
#include <QString>
#include <QStringList>

class QAbstractItemModel;

namespace QtTUI {

class QTuiComboBox : public QTuiWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable)
public:
    explicit QTuiComboBox(QTuiWidget *parent = nullptr);

    void addItem(const QString &text);
    void addItems(const QStringList &texts);
    void insertItem(int index, const QString &text);
    void removeItem(int index);
    void clear();

    int count() const;
    QString itemText(int index) const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    QString currentText() const;

    bool isEditable() const;
    void setEditable(bool editable);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    void paintEvent(ScreenBuffer &buffer) override;
    QSize minimumSize() const override;

signals:
    void currentIndexChanged(int index);
    void currentTextChanged(const QString &text);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    void mousePressEvent(TuiMouseEvent *event) override;

private:
    void openDropdown();
    void closeDropdown();
    void syncItemsFromModel();

    QStringList m_items;
    int m_currentIndex = -1;
    bool m_editable = false;
    bool m_dropdownOpen = false;
    int m_dropdownHighlight = 0;
    QAbstractItemModel *m_model = nullptr;
    QString m_editText;
};

} // namespace QtTUI

#endif // QTTUI_QTUICOMBOBOX_H
