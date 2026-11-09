#ifndef QTTUI_QTUITEXTBROWSER_H
#define QTTUI_QTUITEXTBROWSER_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiScrollArea.h>
#include <QString>
#include <QStringList>

namespace QtTUI {

class QTuiLabel;

// A read-only viewer for long rich text, like QTextBrowser: it scrolls,
// and the user follows links with the keyboard or the mouse.
//
// The content is a word-wrapped QTuiLabel in a scroll area, so the text
// formats and the links are those of QTuiLabel. The width of the text
// follows the width of the viewport. Do not call setWidget().
//
// The browser reads only local files. It never gets data from the network
// and never starts a program.
class QTuiTextBrowser : public QTuiScrollArea {
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool openLinks READ openLinks WRITE setOpenLinks)
public:
    explicit QTuiTextBrowser(QTuiWidget *parent = nullptr);

    // The text as it shows, without markup
    QString toPlainText() const;

    // The path of the file that shows, or an empty string
    QString source() const;

    // If true (the default), a link to a file opens that file: a link
    // without a scheme is a path relative to the directory of source(),
    // and a "file:" link is a local path. A link with a different scheme
    // and a link that starts with "#" are only reported with
    // anchorClicked().
    bool openLinks() const;
    void setOpenLinks(bool open);

    bool isBackwardAvailable() const;
    bool isForwardAvailable() const;

public slots:
    // Shows text in one of the formats of QTuiLabel. source() and the
    // history do not change.
    void setMarkdown(const QString &markdown);
    void setHtml(const QString &html);
    void setPlainText(const QString &text);

    // Reads and shows a local file: .md and .markdown as Markdown, .html
    // and .htm as rich text, all other files as plain text. The file must
    // be UTF-8. If the browser cannot read the file, it warns and shows
    // no text. Adds the path to the history.
    void setSource(const QString &path);
    // Go to the previous, the next and the first file of the history
    void backward();
    void forward();
    void home();

signals:
    // The user activated a link (Enter or a left click)
    void anchorClicked(const QString &link);
    void sourceChanged(const QString &path);
    void backwardAvailable(bool available);
    void forwardAvailable(bool available);

protected:
    void keyPressEvent(TuiKeyEvent *event) override;
    // Tab and Shift+Tab go to the next and the previous link. At the last
    // and the first link, the focus goes to the next widget.
    bool focusNextPrevChild(bool next) override;
    void paintContent(ScreenBuffer &buffer, const QRect &viewport) override;

private:
    void showText(Qt::TextFormat format, const QString &text);
    // Reads the file and shows it; does not change the history
    void load(const QString &path);
    // Gives the label the width of the viewport and the height of its text
    void relayout();
    bool moveLink(bool next);
    void activate(const QString &link);
    void updateAvailable();

    QTuiLabel *m_label;
    bool m_openLinks = true;
    QStringList m_history; // paths; m_historyPos is the one that shows
    int m_historyPos = -1;
    bool m_backwardAvailable = false;
    bool m_forwardAvailable = false;
};

} // namespace QtTUI

#endif // QTTUI_QTUITEXTBROWSER_H
