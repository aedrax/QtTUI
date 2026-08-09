#ifndef QTTUI_QTUILABEL_H
#define QTTUI_QTUILABEL_H

#include <QtTUI/QtTUIGlobal.h>
#include <QtTUI/QTuiFrame.h>
#include <QString>
#include <QVector>
#include <Qt>

namespace QtTUI {

class QTuiLabel : public QTuiFrame {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap)
    Q_PROPERTY(Qt::TextFormat textFormat READ textFormat WRITE setTextFormat)
public:
    explicit QTuiLabel(const QString &text = QString(), QTuiWidget *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    bool wordWrap() const;
    void setWordWrap(bool on);

    // How the text is read, as in QLabel. Qt::PlainText (the default)
    // shows the text as it is. Qt::MarkdownText reads **bold**, *italic*,
    // `code` and ~~strikethrough~~ (__ and _ also work; \ escapes).
    // Qt::RichText reads the HTML tags <b> <strong> <i> <em> <u> <s>
    // <del> <code> <br> <p>, and &lt; &gt; &amp; &quot; &#39; &nbsp;. Other
    // tags are removed. Code shows in reverse video. Qt::AutoText is
    // plain text (Qt's detection is in Qt GUI).
    Qt::TextFormat textFormat() const;
    void setTextFormat(Qt::TextFormat format);
    // The text as it shows, without markup
    QString displayText() const;

    QTuiWidget *buddy() const;
    void setBuddy(QTuiWidget *buddy);

    QSize minimumSize() const override;
    void paintEvent(ScreenBuffer &buffer) override;

signals:
    void textChanged(const QString &text);

private:
    QStringList wrapText(const QString &text, int width) const;

    QString m_text;
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
    bool m_wordWrap = false;
    Qt::TextFormat m_textFormat = Qt::PlainText;

    // Style bits per UTF-16 unit of displayText()
    enum StyleBit : quint8 { Bold = 1, Italic = 2, Underline = 4, Strike = 8, Code = 16 };
    void parse(QString *shown, QVector<quint8> *styles) const;
    QTuiWidget *m_buddy = nullptr;
};

} // namespace QtTUI

#endif // QTTUI_QTUILABEL_H
