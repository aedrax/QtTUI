// Table view demo, editable process table with keyboard navigation.
// Arrow keys to navigate, Enter to edit a cell, Escape to cancel.
// Ctrl+Q to quit.

#include <QtTUI/QtTUI>
#include <QAbstractTableModel>

using namespace QtTUI;

// Editable in-memory table model (Qt6::Core only).
class ProcessModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ProcessModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {
        m_headers = {"PID", "Name", "CPU%", "Memory"};
        m_data = {
            {"1",    "systemd",      "0.1", "12 MB"},
            {"305",  "sshd",         "0.0", "8 MB"},
            {"1024", "bash",         "0.2", "6 MB"},
            {"1337", "vim",          "1.5", "24 MB"},
            {"2048", "htop",         "0.8", "16 MB"},
            {"3001", "node",         "4.2", "128 MB"},
            {"3200", "postgres",     "2.1", "256 MB"},
            {"4096", "nginx",        "0.3", "32 MB"},
            {"5000", "python3",      "3.7", "64 MB"},
            {"6100", "redis-server", "0.5", "48 MB"},
        };
    }

    int rowCount(const QModelIndex & = {}) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = {}) const override { return m_headers.size(); }

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || !idx.isValid()) return {};
        return m_data[idx.row()][idx.column()];
    }

    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override {
        if (role != Qt::EditRole || !idx.isValid()) return false;
        m_data[idx.row()][idx.column()] = value.toString();
        emit dataChanged(idx, idx, {Qt::DisplayRole});
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &idx) const override {
        return QAbstractTableModel::flags(idx) | Qt::ItemIsEditable;
    }

    QVariant headerData(int section, Qt::Orientation o, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
        return m_headers.value(section);
    }

private:
    QStringList m_headers;
    QVector<QStringList> m_data;
};

class Label : public QTuiWidget {
    Q_OBJECT
public:
    explicit Label(const QString &text, QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_text(text) {}

    void setText(const QString &t) { m_text = t; }
    QSize minimumSize() const override { return QSize(0, 1); }

    void paintEvent(ScreenBuffer &buf) override {
        const QRect g = geometry();
        for (int x = 0; x < g.width(); ++x) {
            Cell &c = buf.backCell(g.x() + x, g.y());
            c.character = (x < m_text.size()) ? m_text.at(x) : QLatin1Char(' ');
            c.foreground = Color(255, 200, 80);
            c.bold = true;
        }
    }

private:
    QString m_text;
};

// Helper: compute column x-offset and width (mirrors QTuiTableView layout).
static void columnGeometry(int viewX, int viewWidth, int colCount, int col,
                           int &outX, int &outW)
{
    int base = viewWidth / colCount;
    int rem  = viewWidth % colCount;
    int x = viewX;
    for (int c = 0; c < col; ++c)
        x += base + (c < rem ? 1 : 0);
    outX = x;
    outW = base + (col < rem ? 1 : 0);
}

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *layout = createVBoxLayout(&window);

    auto *title  = new Label(QStringLiteral("=== Process Table (Ctrl+Q to quit) ==="), &window);
    auto *status = new Label(QStringLiteral("Arrows=navigate  Enter=edit  Ctrl+Q=quit"), &window);
    auto *table  = new QTuiTableView(&window);

    auto *model = new ProcessModel(&window);
    table->setModel(model);

    // Hidden editor overlay, shown on top of the current cell when editing
    auto *editor = new QTuiLineEdit(&window);
    editor->setVisible(false);

    layout->addWidget(title,  0);
    layout->addWidget(table,  1);
    layout->addWidget(status, 0);

    window.setLayout(layout);

    bool editing = false;
    QModelIndex editIndex;

    // Enter pressed on a cell, start editing
    QObject::connect(table, &QTuiTableView::activated, [&](const QModelIndex &idx) {
        if (editing) return;
        editing = true;
        editIndex = idx;

        // Position the editor over the cell
        QRect tg = table->geometry();
        int colCount = model->columnCount();
        int cellX, cellW;
        columnGeometry(tg.x(), tg.width(), colCount, idx.column(), cellX, cellW);
        int cellY = tg.y() + 1 + (idx.row() - table->scrollOffset()); // +1 for header

        editor->setGeometry(QRect(cellX, cellY, cellW, 1));
        editor->setText(model->data(idx).toString());
        editor->setCursorPosition(editor->text().length());
        editor->setVisible(true);
        editor->setFocus();

        status->setText(QStringLiteral("Editing [%1,%2], Enter=commit  Esc=cancel")
                            .arg(idx.row()).arg(idx.column()));
    });

    // Enter in editor, commit
    QObject::connect(editor, &QTuiLineEdit::returnPressed, [&]() {
        if (!editing) return;
        model->setData(editIndex, editor->text());
        editing = false;
        editor->setVisible(false);
        table->setFocus();
        status->setText(QStringLiteral("Saved [%1,%2] = %3")
                            .arg(editIndex.row()).arg(editIndex.column()).arg(editor->text()));
    });

    // Ctrl+Q to quit, Escape to cancel editing
    struct AppFilter : public QObject {
        bool *editing;
        QTuiLineEdit *editor;
        QTuiTableView *table;
        Label *status;

        bool eventFilter(QObject *, QEvent *ev) override {
            if (ev->type() != TuiEventTypes::keyEventType())
                return false;
            auto *ke = static_cast<TuiKeyEvent *>(ev);
            if (ke->key() == Qt::Key_Q && (ke->modifiers() & Qt::ControlModifier)) {
                QCoreApplication::quit();
                return true;
            }
            if (ke->key() == Qt::Key_Escape && *editing) {
                *editing = false;
                editor->setVisible(false);
                table->setFocus();
                status->setText(QStringLiteral("Edit cancelled"));
                return true;
            }
            return false;
        }
    };
    auto *filter = new AppFilter;
    filter->editing = &editing;
    filter->editor = editor;
    filter->table = table;
    filter->status = status;
    filter->setParent(&app);
    app.installEventFilter(filter);

    table->setFocus();
    window.show();
    app.triggerRender();

    return app.exec();
}

#include "table.moc"
