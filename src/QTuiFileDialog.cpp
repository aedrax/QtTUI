#include <QtTUI/QTuiFileDialog.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiListView.h>
#include <QtTUI/QTuiPushButton.h>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringListModel>

namespace QtTUI {

namespace {

// The entry list asks for a useful size inside the dialog
class FileListView : public QTuiListView {
public:
    using QTuiListView::QTuiListView;
    QSize minimumSize() const override { return QSize(50, 12); }
};

// "Images (*.png *.jpg);;Text (*.txt)" -> the patterns of the first filter
QStringList patternsFromFilter(const QString &filter)
{
    const QString first = filter.split(QStringLiteral(";;")).value(0);
    const QRegularExpressionMatch m = QRegularExpression(QStringLiteral("\\(([^)]*)\\)")).match(first);
    const QString patterns = m.hasMatch() ? m.captured(1) : first;
    return patterns.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

} // namespace

QTuiFileDialog::QTuiFileDialog(QTuiWidget *parent, const QString &caption,
                               const QString &directory, const QString &filter)
    : QTuiDialog(parent)
    , m_nameFilters(patternsFromFilter(filter))
    , m_pathLabel(new QTuiLabel(QString(), this))
    , m_list(new FileListView(this))
    , m_model(new QStringListModel(this))
    , m_nameEdit(new QTuiLineEdit(this))
{
    setWindowTitle(caption.isEmpty() ? QStringLiteral("Choose a file") : caption);
    m_list->setModel(m_model);

    auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    column->addWidget(m_pathLabel);
    column->addWidget(m_list);
    column->addSpacing(1);
    column->addWidget(m_nameEdit);
    column->addSpacing(1);

    auto *buttons = new QTuiWidget(this);
    auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    row->setSpacing(1);
    row->addStretch();
    auto *ok = new QTuiPushButton(QStringLiteral("OK"), buttons);
    auto *cancel = new QTuiPushButton(QStringLiteral("Cancel"), buttons);
    row->addWidget(ok);
    row->addWidget(cancel);
    buttons->setLayout(row);
    column->addWidget(buttons);
    setLayout(column);

    connect(m_list, &QTuiListView::activated, this, &QTuiFileDialog::openEntry);
    connect(m_nameEdit, &QTuiLineEdit::returnPressed, this, &QTuiFileDialog::tryAccept);
    connect(ok, &QTuiPushButton::clicked, this, &QTuiFileDialog::tryAccept);
    connect(cancel, &QTuiPushButton::clicked, this, &QTuiDialog::reject);

    setDirectory(directory.isEmpty() ? QDir::currentPath() : directory);
}

QTuiFileDialog::FileMode QTuiFileDialog::fileMode() const
{
    return m_mode;
}

void QTuiFileDialog::setFileMode(FileMode mode)
{
    m_mode = mode;
    refresh();
}

QString QTuiFileDialog::directory() const
{
    return m_dir;
}

void QTuiFileDialog::setDirectory(const QString &path)
{
    m_dir = QDir(path).absolutePath();
    refresh();
}

QString QTuiFileDialog::selectedFile() const
{
    return m_selected;
}

void QTuiFileDialog::refresh()
{
    QDir dir(m_dir);
    QStringList entries;
    if (!dir.isRoot())
        entries.append(QStringLiteral("../"));
    const auto dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QString &name : dirs)
        entries.append(name + QLatin1Char('/'));
    if (m_mode != Directory) {
        const auto files = dir.entryList(m_nameFilters, QDir::Files, QDir::Name | QDir::IgnoreCase);
        entries += files;
    }
    m_model->setStringList(entries);
    m_list->setCurrentIndex(m_model->index(0));
    m_pathLabel->setText(m_dir);
}

void QTuiFileDialog::openEntry(const QModelIndex &index)
{
    const QString entry = index.data().toString();
    if (entry.endsWith(QLatin1Char('/'))) {
        setDirectory(QDir(m_dir).filePath(entry));
        return;
    }
    m_nameEdit->setText(entry);
}

void QTuiFileDialog::tryAccept()
{
    const QString name = m_nameEdit->text().trimmed();
    const QString path = name.isEmpty() ? m_dir : QDir(m_dir).absoluteFilePath(name);
    const QFileInfo info(path);

    if (info.isDir() && m_mode != Directory) {
        setDirectory(path); // a folder name opens the folder
        m_nameEdit->setText(QString());
        return;
    }
    if (m_mode == Directory && !info.isDir())
        return;
    if (m_mode == ExistingFile && !info.isFile())
        return;
    if (m_mode == AnyFile && name.isEmpty())
        return;

    m_selected = QDir::cleanPath(path);
    accept();
}

static QString runDialog(QTuiWidget *parent, const QString &caption, const QString &dir,
                         const QString &filter, QTuiFileDialog::FileMode mode)
{
    QTuiFileDialog dialog(parent, caption, dir, filter);
    dialog.setFileMode(mode);
    return dialog.exec() == QTuiDialog::Accepted ? dialog.selectedFile() : QString();
}

QString QTuiFileDialog::getOpenFileName(QTuiWidget *parent, const QString &caption,
                                        const QString &dir, const QString &filter)
{
    return runDialog(parent, caption, dir, filter, ExistingFile);
}

QString QTuiFileDialog::getSaveFileName(QTuiWidget *parent, const QString &caption,
                                        const QString &dir, const QString &filter)
{
    return runDialog(parent, caption, dir, filter, AnyFile);
}

QString QTuiFileDialog::getExistingDirectory(QTuiWidget *parent, const QString &caption,
                                             const QString &dir)
{
    return runDialog(parent, caption, dir, QString(), Directory);
}

} // namespace QtTUI
