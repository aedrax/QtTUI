#ifndef QTTUI_QTUIFILEDIALOG_H
#define QTTUI_QTUIFILEDIALOG_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiDialog.h>
#include <QStringList>

class QStringListModel;

namespace QtTUI {

class QTuiLabel;
class QTuiLineEdit;
class QTuiListView;

// File and folder picker, like QFileDialog. The static functions are the
// usual way to use it; each returns an absolute path, or an empty string
// if the user cancels.
//
// The dialog shows the current folder, its entries (folders first, with
// "/", and ".." to go up) and a file name field. Enter or a click on a
// folder opens it; a file puts its name in the field. Enter in the field
// or OK accepts. A filter such as "Images (*.png *.jpg)" limits the files.
class QTuiFileDialog : public QTuiDialog {
    Q_OBJECT
public:
    enum FileMode { AnyFile, ExistingFile, Directory };

    explicit QTuiFileDialog(QTuiWidget *parent = nullptr, const QString &caption = QString(),
                            const QString &directory = QString(), const QString &filter = QString());

    FileMode fileMode() const;
    void setFileMode(FileMode mode);

    QString directory() const;
    void setDirectory(const QString &path);

    // The chosen path after the dialog is accepted
    QString selectedFile() const;

    static QString getOpenFileName(QTuiWidget *parent = nullptr, const QString &caption = QString(),
                                   const QString &dir = QString(), const QString &filter = QString());
    static QString getSaveFileName(QTuiWidget *parent = nullptr, const QString &caption = QString(),
                                   const QString &dir = QString(), const QString &filter = QString());
    static QString getExistingDirectory(QTuiWidget *parent = nullptr, const QString &caption = QString(),
                                        const QString &dir = QString());

private:
    void refresh();
    void openEntry(const QModelIndex &index);
    void tryAccept();

    FileMode m_mode = AnyFile;
    QString m_dir;
    QStringList m_nameFilters;
    QString m_selected;
    QTuiLabel *m_pathLabel;
    QTuiListView *m_list;
    QStringListModel *m_model;
    QTuiLineEdit *m_nameEdit;
};

} // namespace QtTUI

#endif // QTTUI_QTUIFILEDIALOG_H
