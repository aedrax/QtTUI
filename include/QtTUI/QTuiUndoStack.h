#ifndef QTTUI_QTUIUNDOSTACK_H
#define QTTUI_QTUIUNDOSTACK_H

#include <QObject>
#include <QString>
#include <QList>

namespace QtTUI {

class QTuiUndoCommand {
public:
    explicit QTuiUndoCommand(const QString &text = QString());
    virtual ~QTuiUndoCommand();

    virtual void undo();
    virtual void redo();

    QString text() const;
    void setText(const QString &text);

private:
    Q_DISABLE_COPY(QTuiUndoCommand)
    QString m_text;
};

class QTuiUndoStack : public QObject {
    Q_OBJECT
public:
    explicit QTuiUndoStack(QObject *parent = nullptr);
    ~QTuiUndoStack() override;

    void push(QTuiUndoCommand *cmd);
    void undo();
    void redo();

    int count() const;
    int index() const;
    void setIndex(int idx);

    QString text(int idx) const;

    bool canUndo() const;
    bool canRedo() const;

    void clear();

signals:
    void indexChanged(int idx);
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);

private:
    QList<QTuiUndoCommand *> m_commands;
    int m_index = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUIUNDOSTACK_H
