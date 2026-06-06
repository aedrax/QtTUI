#ifndef QTTUI_QTUICOMPLETER_H
#define QTTUI_QTUICOMPLETER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QObject>
#include <QPointer>
#include <QStringList>

class QAbstractItemModel;

namespace QtTUI {

class QTuiLineEdit;
class QTuiWidget;
class CompleterPopup;

// Completions for a QTuiLineEdit, like QCompleter. Set it with
// QTuiLineEdit::setCompleter(). While the user types, a popup below the
// line edit lists the model rows (column 0) that match the text. Up and
// Down select a row, Enter accepts it, Escape closes the popup. The line
// edit keeps focus.
//
// The completer reads the model each time; it does not copy the data.
class QTuiCompleter : public QObject {
    Q_OBJECT
public:
    enum FilterMode { StartsWith, Contains };

    explicit QTuiCompleter(const QStringList &list, QObject *parent = nullptr);
    explicit QTuiCompleter(QAbstractItemModel *model = nullptr, QObject *parent = nullptr);
    ~QTuiCompleter() override;

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity sensitivity);

    FilterMode filterMode() const;
    void setFilterMode(FilterMode mode);

    // Rows the popup shows before it scrolls (default 7)
    int maxVisibleItems() const;
    void setMaxVisibleItems(int count);

    QString completionPrefix() const;
    void setCompletionPrefix(const QString &prefix);

    // Matches for the completion prefix, in model order
    QStringList completions() const;
    int completionCount() const;
    int currentRow() const;
    void setCurrentRow(int row);
    QString currentCompletion() const;

    bool isPopupVisible() const;

    // Used by QTuiLineEdit: shows the popup below the line edit when the
    // text has matches, and hides it when it has none
    void updateForText(QTuiLineEdit *lineEdit, const QString &text);
    // Used by QTuiLineEdit: returns true if the popup used the key
    bool handleKey(int key);
    void hidePopup();

signals:
    // The user accepted a completion (Enter or a click)
    void activated(const QString &text);
    // The selected row changed
    void highlighted(const QString &text);

private:
    friend class CompleterPopup;
    void activateRow(int row);

    QPointer<QAbstractItemModel> m_model;
    QPointer<QTuiLineEdit> m_lineEdit;
    QPointer<CompleterPopup> m_popup;
    QString m_prefix;
    Qt::CaseSensitivity m_caseSensitivity = Qt::CaseSensitive;
    FilterMode m_filterMode = StartsWith;
    int m_maxVisibleItems = 7;
    int m_currentRow = -1;
};

} // namespace QtTUI

#endif // QTTUI_QTUICOMPLETER_H
