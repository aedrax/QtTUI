// Tests for the structure features of the item views: hidden rows, the
// root index, alternating row colors, the elide mode, delegates, check
// states, headers, tree columns and spans.

#include <QtTest/QtTest>
#include <QtTUI/QtTUI>
#include <QtTUI/RenderUtils.h>
#include <QStringListModel>
#include <QAbstractTableModel>
#include <QAbstractListModel>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier,
                    const QString &text = QString())
{
    TuiKeyEvent ev(key, mods, text);
    QCoreApplication::sendEvent(&w, &ev);
}

static void click(QTuiWidget &w, int x, int y, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiMouseEvent ev(TuiMouseEvent::Press, Qt::LeftButton, w.geometry().topLeft() + QPoint(x, y), mods);
    QCoreApplication::sendEvent(&w, &ev);
}

static void wheel(QTuiWidget &w, int delta)
{
    TuiMouseEvent ev(TuiMouseEvent::Wheel, Qt::NoButton, w.geometry().topLeft() + QPoint(1, 2),
                     Qt::NoModifier, delta);
    QCoreApplication::sendEvent(&w, &ev);
}

static QStringList numberedRows(int count)
{
    QStringList rows;
    for (int i = 0; i < count; ++i)
        rows << QStringLiteral("row %1").arg(i);
    return rows;
}

// The text of one line of the buffer, without the spaces at the end
static QString lineText(const ScreenBuffer &buffer, int y)
{
    QString text;
    for (int x = 0; x < buffer.width(); ++x) {
        const char32_t ch = buffer.backCell(x, y).ch;
        if (ch != 0)
            text += QString::fromUcs4(&ch, 1);
    }
    while (text.endsWith(QLatin1Char(' ')))
        text.chop(1);
    return text;
}

static QString paintedLine(QTuiWidget &view, int y)
{
    ScreenBuffer buffer(view.geometry().right() + 1, view.geometry().bottom() + 1);
    view.paintEvent(buffer);
    return lineText(buffer, y);
}

// A table model with a row count that the test sets. It counts the calls
// of data(), so a test can show that a view does not read all rows.
class GridModel : public QAbstractTableModel {
public:
    int rows = 50;
    int cols = 3;
    mutable int dataCalls = 0;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : rows;
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : cols;
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        ++dataCalls;
        if (role == Qt::DisplayRole)
            return QStringLiteral("r%1c%2").arg(index.row()).arg(index.column());
        return {};
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        return orientation == Qt::Horizontal ? QVariant(QStringLiteral("H%1").arg(section))
                                             : QVariant(section + 1);
    }
    void insertAt(int row, int count)
    {
        beginInsertRows(QModelIndex(), row, row + count - 1);
        rows += count;
        endInsertRows();
    }
    void removeAt(int row, int count)
    {
        beginRemoveRows(QModelIndex(), row, row + count - 1);
        rows -= count;
        endRemoveRows();
    }
    void reset()
    {
        beginResetModel();
        endResetModel();
    }
};

// Row 2 has a blue Qt::BackgroundRole
class BlueRowModel : public GridModel {
public:
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::BackgroundRole && index.row() == 2)
            return 0x0000ff;
        return GridModel::data(index, role);
    }
};

// Writes the item text in upper case, and makes a spin box as the editor
class UpperDelegate : public QTuiStyledItemDelegate {
public:
    mutable int paints = 0;
    void paint(ScreenBuffer &buffer, const QTuiStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        ++paints;
        renderText(buffer, option.rect.x(), option.rect.y(), index.data().toString().toUpper(),
                   option.rect.width(), option.style);
    }
    QTuiWidget *createEditor(QTuiWidget *parent, const QModelIndex &) const override
    {
        return new QTuiSpinBox(parent);
    }
};

class EditableGridModel : public GridModel {
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        return GridModel::flags(index) | Qt::ItemIsEditable;
    }
};

// A list with check boxes. Row 0 and 1 are checkable, row 2 goes through
// the three states, row 3 has a check state that the user cannot change,
// and row 4 has no check state. Row 1 has a glyph, row 4 a decoration
// that is not a string.
class CheckModel : public QAbstractListModel {
public:
    QList<int> states{Qt::Unchecked, Qt::Checked, Qt::Unchecked, Qt::Checked, -1};
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : states.size();
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::DisplayRole)
            return QStringLiteral("item %1").arg(index.row());
        if (role == Qt::CheckStateRole && states.at(index.row()) >= 0)
            return states.at(index.row());
        if (role == Qt::DecorationRole && index.row() == 1)
            return QStringLiteral("★");
        if (role == Qt::DecorationRole && index.row() == 4)
            return 42;
        return {};
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags f = QAbstractListModel::flags(index);
        if (index.row() != 3)
            f |= Qt::ItemIsUserCheckable;
        if (index.row() == 2)
            f |= Qt::ItemIsUserTristate;
        return f;
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role != Qt::CheckStateRole)
            return false;
        states[index.row()] = value.toInt();
        emit dataChanged(index, index, {role});
        return true;
    }
};

static QString truncated(const QString &text, int width, Qt::TextElideMode mode)
{
    ScreenBuffer buffer(width + 2, 1);
    renderTextTruncated(buffer, 0, 0, text, width, Cell(), mode);
    return lineText(buffer, 0);
}

// A tree of two levels: groups "A", "B", ... and their children "A0",
// "A1", ... Each item has two columns; column 1 has the text and "+".
class GroupModel : public QAbstractItemModel {
public:
    QList<QStringList> groups;
    explicit GroupModel(int groupCount = 3, int children = 4)
    {
        for (int g = 0; g < groupCount; ++g) {
            QStringList names;
            for (int c = 0; c < children; ++c)
                names << QStringLiteral("%1%2").arg(QChar('A' + g)).arg(c);
            groups << names;
        }
    }
    // The internal id is 0 for a group and the group row + 1 for a child
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();
        return createIndex(row, column, quintptr(parent.isValid() ? parent.row() + 1 : 0));
    }
    QModelIndex parent(const QModelIndex &child) const override
    {
        const int group = int(child.internalId()) - 1;
        return group < 0 ? QModelIndex() : createIndex(group, 0, quintptr(0));
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (!parent.isValid())
            return groups.size();
        return parent.internalId() == 0 && parent.column() == 0 ? groups.at(parent.row()).size() : 0;
    }
    int columnCount(const QModelIndex & = QModelIndex()) const override { return 2; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        const int group = int(index.internalId()) - 1;
        const QString name = group < 0 ? QString(QChar('A' + index.row()))
                                       : groups.at(group).at(index.row());
        return index.column() == 0 ? name : name + QLatin1Char('+');
    }
    QVariant headerData(int section, Qt::Orientation, int role) const override
    {
        return role == Qt::DisplayRole ? QVariant(QStringLiteral("H%1").arg(section)) : QVariant();
    }
    void removeGroup(int row)
    {
        beginRemoveRows(QModelIndex(), row, row);
        groups.removeAt(row);
        endRemoveRows();
    }
    void addChild(int group, const QString &name)
    {
        const int row = groups.at(group).size();
        beginInsertRows(index(group, 0), row, row);
        groups[group] << name;
        endInsertRows();
    }
    void reset()
    {
        beginResetModel();
        endResetModel();
    }
};

class TestItemViewStructure : public QObject {
    Q_OBJECT

private slots:
    void cleanup()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::TrueColor);
    }

    // --- Hidden rows ---

    void hiddenRowsInTheListView()
    {
        QStringListModel model(numberedRows(20));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 20, 5));
        view.setModel(&model);
        view.setRowHidden(1, true);
        view.setRowHidden(2, true);
        QVERIFY(view.isRowHidden(1));
        QVERIFY(!view.isRowHidden(0));
        QCOMPARE(paintedLine(view, 0), QStringLiteral("row 0"));
        QCOMPARE(paintedLine(view, 1), QStringLiteral("row 3"));

        // The keys go past the hidden rows
        sendKey(view, Qt::Key_Down);
        QCOMPARE(view.currentIndex().row(), 3);
        sendKey(view, Qt::Key_Up);
        QCOMPARE(view.currentIndex().row(), 0);
        // A page is 4 view rows: rows 0, 3, 4, 5, 6
        sendKey(view, Qt::Key_PageDown);
        QCOMPARE(view.currentIndex().row(), 6);
        sendKey(view, Qt::Key_End);
        QCOMPARE(view.currentIndex().row(), 19);
        QCOMPARE(paintedLine(view, 4), QStringLiteral("row 19"));
        sendKey(view, Qt::Key_Home);

        // The mouse and visualRect() use the same rows
        click(view, 1, 1);
        QCOMPARE(view.currentIndex().row(), 3);
        QCOMPARE(view.visualRect(model.index(3, 0)), QRect(0, 1, 20, 1));
        QVERIFY(view.visualRect(model.index(2, 0)).isEmpty());

        // The wheel stops at the last row that shows: 18 rows, 5 lines
        wheel(view, -120 * 100);
        QCOMPARE(paintedLine(view, 0), QStringLiteral("row 15"));
        QCOMPARE(paintedLine(view, 4), QStringLiteral("row 19"));

        view.scrollTo(model.index(3, 0), QTuiAbstractItemView::PositionAtTop);
        QCOMPARE(paintedLine(view, 0), QStringLiteral("row 3"));

        view.setRowHidden(1, false);
        view.setRowHidden(2, false);
        view.scrollTo(model.index(0, 0));
        QCOMPARE(paintedLine(view, 1), QStringLiteral("row 1"));
    }

    void hiddenRowsInTheTableView()
    {
        GridModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        view.hideRow(0);
        view.hideRow(3);
        QVERIFY(view.isRowHidden(3));
        // Line 0 is the header
        QVERIFY(paintedLine(view, 1).startsWith(QStringLiteral("r1c0")));
        QVERIFY(paintedLine(view, 3).startsWith(QStringLiteral("r4c0")));
        // The current row was row 0: the next row that shows is current
        QCOMPARE(view.currentIndex().row(), 1);
        sendKey(view, Qt::Key_Down);
        sendKey(view, Qt::Key_Down);
        QCOMPARE(view.currentIndex().row(), 4);
        click(view, 1, 2);
        QCOMPARE(view.currentIndex().row(), 2);
        QCOMPARE(view.visualRect(model.index(4, 0)).y(), 3);
        QVERIFY(view.visualRect(model.index(3, 0)).isEmpty());

        // A hidden row cannot be current
        view.setCurrentIndex(model.index(3, 1));
        QCOMPARE(view.currentIndex(), model.index(4, 1));

        view.showRow(0);
        QVERIFY(paintedLine(view, 1).startsWith(QStringLiteral("r0c0")));
    }

    void hiddenRowsInTheTreeView()
    {
        QStringListModel model(numberedRows(5));
        QTuiTreeView view;
        view.setGeometry(QRect(0, 0, 20, 5));
        view.setModel(&model);
        view.setRowHidden(0, QModelIndex(), true);
        view.setRowHidden(2, QModelIndex(), true);
        QVERIFY(view.isRowHidden(2, QModelIndex()));
        QCOMPARE(paintedLine(view, 0).trimmed(), QStringLiteral("row 1"));
        QCOMPARE(paintedLine(view, 1).trimmed(), QStringLiteral("row 3"));
        QCOMPARE(view.currentIndex().row(), 1);
        sendKey(view, Qt::Key_Down);
        QCOMPARE(view.currentIndex().row(), 3);
        view.setRowHidden(2, QModelIndex(), false);
        QCOMPARE(view.currentIndex().row(), 3);
        QCOMPARE(paintedLine(view, 1).trimmed(), QStringLiteral("row 2"));
    }

    void shiftSelectionSkipsHiddenRows()
    {
        GridModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 10));
        view.setModel(&model);
        view.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        view.setRowHidden(2, true);
        sendKey(view, Qt::Key_Down);
        sendKey(view, Qt::Key_Down, Qt::ShiftModifier);
        sendKey(view, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(view.currentIndex().row(), 4);
        QItemSelectionModel *selection = view.selectionModel();
        QVERIFY(selection->isSelected(model.index(1, 0)));
        QVERIFY(!selection->isSelected(model.index(2, 0)));
        QVERIFY(selection->isSelected(model.index(3, 0)));
        QVERIFY(selection->isSelected(model.index(4, 0)));

        view.selectAll();
        QVERIFY(!selection->isSelected(model.index(2, 1)));
        QVERIFY(selection->isSelected(model.index(49, 2)));
    }

    void hiddenRowsFollowInsertsAndRemoves()
    {
        GridModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        view.setRowHidden(5, true);
        view.setRowHidden(10, true);
        model.insertAt(2, 3);
        QVERIFY(view.isRowHidden(8));
        QVERIFY(view.isRowHidden(13));
        QVERIFY(!view.isRowHidden(5));
        // The removed rows 6 to 9 contain the hidden row 8
        model.removeAt(6, 4);
        QVERIFY(!view.isRowHidden(8));
        QVERIFY(view.isRowHidden(9));
        // A row after the hidden rows does not move them
        model.insertAt(40, 1);
        QVERIFY(view.isRowHidden(9));
        // A reset shows all rows
        model.reset();
        QVERIFY(!view.isRowHidden(9));
    }

    void hiddenRowsDirtyRegion()
    {
        GridModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 8));
        view.setModel(&model);
        view.clearDirtyRegions();
        // Row 3 is on line 4: the lines from there to the bottom change
        view.setRowHidden(3, true);
        QCOMPARE(view.dirtyRegion(), QRect(0, 4, 30, 4));
        view.clearDirtyRegions();
        // Row 5 is now on line 5
        emit model.dataChanged(model.index(5, 0), model.index(5, 0));
        QCOMPARE(view.dirtyRegion(), QRect(0, 5, 30, 1));
        view.clearDirtyRegions();
        view.setRowHidden(3, true); // no change
        QVERIFY(view.dirtyRegion().isNull());
    }

    void hiddenRowsDoNotReadAllRows()
    {
        GridModel model;
        model.rows = 2600000;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 10));
        view.setModel(&model);
        for (int row = 0; row < 2000; row += 2)
            view.setRowHidden(row, true);
        model.dataCalls = 0;
        ScreenBuffer buffer(30, 10);
        view.paintEvent(buffer);
        sendKey(view, Qt::Key_End);
        sendKey(view, Qt::Key_PageUp);
        view.paintEvent(buffer);
        QCOMPARE(view.currentIndex().row(), 2600000 - 1 - 8);
        QVERIFY2(model.dataCalls < 5000, qPrintable(QString::number(model.dataCalls)));
    }

    void allRowsHidden()
    {
        QStringListModel model(numberedRows(2));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 20, 5));
        view.setModel(&model);
        view.setRowHidden(0, true);
        view.setRowHidden(1, true);
        QVERIFY(!view.currentIndex().isValid());
        sendKey(view, Qt::Key_Down);
        QCOMPARE(paintedLine(view, 0), QString());
        view.setRowHidden(1, false);
        QCOMPARE(view.currentIndex().row(), 1);
    }

    void editorClosesWhenItsRowHides()
    {
        QStringListModel model(numberedRows(5));
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 20, 5));
        view.setModel(&model);
        QVERIFY(view.edit(model.index(2, 0)));
        view.setRowHidden(1, true);
        QVERIFY(view.isEditing());
        QCOMPARE(view.editor()->geometry().y(), 1);
        view.setRowHidden(2, true);
        QVERIFY(!view.isEditing());
    }

    // --- Root index ---

    void rootIndexInTheListView()
    {
        GroupModel model;
        QTuiListView view;
        view.setGeometry(QRect(0, 0, 20, 5));
        view.setModel(&model);
        QCOMPARE(paintedLine(view, 1), QStringLiteral("B"));
        sendKey(view, Qt::Key_Down);

        const QModelIndex root = model.index(1, 0);
        view.setRootIndex(root);
        QCOMPARE(view.rootIndex(), root);
        QCOMPARE(paintedLine(view, 0), QStringLiteral("B0"));
        QCOMPARE(paintedLine(view, 3), QStringLiteral("B3"));
        QCOMPARE(paintedLine(view, 4), QString());
        // The first row of the root is current, and the keys stay in it
        QCOMPARE(view.currentIndex(), model.index(0, 0, root));
        sendKey(view, Qt::Key_End);
        QCOMPARE(view.currentIndex(), model.index(3, 0, root));
        QVERIFY(view.selectionModel()->isSelected(model.index(3, 0, root)));
        click(view, 1, 1);
        QCOMPARE(view.currentIndex(), model.index(1, 0, root));
        QCOMPARE(view.visualRect(model.index(2, 0, root)).y(), 2);
        QVERIFY(view.visualRect(model.index(2, 0)).isEmpty());

        // Rows of the root repaint; rows of a different parent do not
        view.clearDirtyRegions();
        model.addChild(0, QStringLiteral("A4"));
        QVERIFY(view.dirtyRegion().isNull());
        model.addChild(1, QStringLiteral("B4"));
        QCOMPARE(view.dirtyRegion(), QRect(0, 4, 20, 1));
        QCOMPARE(paintedLine(view, 4), QStringLiteral("B4"));

        // An index of a different model has no effect
        GroupModel other;
        view.setRootIndex(other.index(0, 0));
        QCOMPARE(view.rootIndex(), root);
    }

    void rootIndexInTheTableView()
    {
        GroupModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 20, 6));
        view.setModel(&model);
        const QModelIndex root = model.index(2, 0);
        view.setRootIndex(root);
        QCOMPARE(paintedLine(view, 0), QStringLiteral("H0 H1"));
        QCOMPARE(paintedLine(view, 1), QStringLiteral("C0 C0+"));
        QCOMPARE(paintedLine(view, 4), QStringLiteral("C3 C3+"));
        QCOMPARE(paintedLine(view, 5), QString());
        sendKey(view, Qt::Key_Down);
        sendKey(view, Qt::Key_Right);
        QCOMPARE(view.currentIndex(), model.index(1, 1, root));
        view.setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
        view.selectAll();
        QVERIFY(view.selectionModel()->isSelected(model.index(3, 1, root)));
        QVERIFY(!view.selectionModel()->isSelected(model.index(0, 0)));
    }

    void rootIndexInTheTreeAndColumnViews()
    {
        GroupModel model;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 6));
        tree.setModel(&model);
        tree.setRootIndex(model.index(0, 0));
        QCOMPARE(paintedLine(tree, 0).trimmed(), QStringLiteral("A0 A0+"));
        QCOMPARE(paintedLine(tree, 4), QString());
        QCOMPARE(tree.currentIndex(), model.index(0, 0, model.index(0, 0)));

        QTuiColumnView columns;
        columns.setGeometry(QRect(0, 0, 40, 6));
        columns.setModel(&model);
        columns.setRootIndex(model.index(1, 0));
        QVERIFY(paintedLine(columns, 0).startsWith(QStringLiteral("B0")));
        QCOMPARE(columns.currentIndex(), model.index(0, 0, model.index(1, 0)));
        columns.setCurrentIndex(model.index(2, 0, model.index(1, 0)));
        QVERIFY(paintedLine(columns, 0).startsWith(QStringLiteral("B0")));
        QCOMPARE(columns.currentIndex(), model.index(2, 0, model.index(1, 0)));
    }

    void removedRootResetsToTheInvalidIndex()
    {
        GroupModel model;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 5));
        list.setModel(&model);
        list.setRootIndex(model.index(1, 0));
        list.setRowHidden(0, true);
        list.clearDirtyRegions();
        model.removeGroup(1);
        QVERIFY(!list.rootIndex().isValid());
        QCOMPARE(list.dirtyRegion(), list.geometry());
        QCOMPARE(paintedLine(list, 0), QStringLiteral("A"));
        // The two groups that stay have the names of their rows
        QCOMPARE(paintedLine(list, 1), QStringLiteral("B"));
        QCOMPARE(paintedLine(list, 2), QString());
        QVERIFY(!list.isRowHidden(0));
        QCOMPARE(list.currentIndex(), model.index(0, 0));

        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 5));
        tree.setModel(&model);
        tree.setRootIndex(model.index(0, 0));
        model.reset();
        QVERIFY(!tree.rootIndex().isValid());
        QCOMPARE(paintedLine(tree, 0).trimmed(), QStringLiteral("▶ A A+"));

        // A new model starts at the invalid index
        list.setRootIndex(model.index(0, 0));
        QStringListModel strings(numberedRows(3));
        list.setModel(&strings);
        QVERIFY(!list.rootIndex().isValid());
        QCOMPARE(paintedLine(list, 2), QStringLiteral("row 2"));
    }

    // --- Alternating row colors ---

    void alternatingRowColors()
    {
        BlueRowModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 8));
        view.setModel(&model);
        const ThemeProperties theme = Theme_Engine::instance()->properties(QStringLiteral("QTuiTableView"));
        ScreenBuffer buffer(30, 8);
        view.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 2).background, theme.background);

        QVERIFY(!view.alternatingRowColors());
        view.clearDirtyRegions();
        view.setAlternatingRowColors(true);
        QVERIFY(view.alternatingRowColors());
        QCOMPARE(view.dirtyRegion(), view.geometry());
        view.paintEvent(buffer);
        // Line 0 is the header. Rows 1, 3 and 5 have the shade, also in
        // the separators and after the last column.
        const Color shade = buffer.backCell(0, 2).background;
        QVERIFY(shade != theme.background);
        QCOMPARE(buffer.backCell(4, 2).background, shade);
        QCOMPARE(buffer.backCell(29, 2).background, shade);
        QCOMPARE(buffer.backCell(0, 4).background, shade);
        QCOMPARE(buffer.backCell(0, 5).background, theme.background);
        // The text stays readable on the shade
        QCOMPARE(buffer.backCell(0, 2).foreground, theme.foreground);
        // The header has no shade, and the current row 0 has the focus colors
        QCOMPARE(buffer.backCell(0, 0).background, theme.background);
        // The color of the model comes before the shade
        QCOMPARE(buffer.backCell(0, 3).background, Color(0, 0, 255));

        // The selection comes before the shade
        view.setFocus();
        view.setCurrentIndex(model.index(1, 0));
        view.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 2).background, theme.focusBackground);
        view.setCurrentIndex(model.index(0, 0));

        // A hidden row does not count
        view.setRowHidden(1, true);
        view.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 2).background, Color(0, 0, 255)); // row 2
        QCOMPARE(buffer.backCell(0, 3).background, theme.background); // row 3
        QCOMPARE(buffer.backCell(0, 4).background, shade);            // row 4

        // The NoColor mode shows no shade
        AnsiRenderer::setColorMode(AnsiRenderer::NoColor);
        view.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 4).background, theme.background);
    }

    void alternatingRowColorsInTheOtherViews()
    {
        GroupModel model;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 5));
        list.setModel(&model);
        list.setAlternatingRowColors(true);
        ScreenBuffer buffer(20, 5);
        list.paintEvent(buffer);
        const Color plain = buffer.backCell(19, 2).background;
        const Color shade = buffer.backCell(19, 1).background;
        QVERIFY(shade != plain);

        // In the tree, the lines alternate, not the rows of each parent
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 5));
        tree.setModel(&model);
        tree.setAlternatingRowColors(true);
        tree.expand(model.index(0, 0));
        tree.setCurrentIndex(model.index(2, 0));
        tree.scrollTo(model.index(0, 0));
        tree.paintEvent(buffer);
        // Lines: A, A0, A1, A2, A3
        QCOMPARE(buffer.backCell(19, 0).background, plain);
        QCOMPARE(buffer.backCell(19, 1).background, shade);
        QCOMPARE(buffer.backCell(19, 2).background, plain);
        QCOMPARE(buffer.backCell(19, 3).background, shade);
    }

    // --- Elide mode ---

    void renderTextTruncatedModes()
    {
        const QString text = QStringLiteral("abcdefghij");
        QCOMPARE(truncated(text, 10, Qt::ElideLeft), text);
        QCOMPARE(truncated(text, 6, Qt::ElideRight), QStringLiteral("abcde…"));
        QCOMPARE(truncated(text, 6, Qt::ElideLeft), QStringLiteral("…fghij"));
        QCOMPARE(truncated(text, 6, Qt::ElideMiddle), QStringLiteral("abc…ij"));
        QCOMPARE(truncated(text, 7, Qt::ElideMiddle), QStringLiteral("abc…hij"));
        QCOMPARE(truncated(text, 6, Qt::ElideNone), QStringLiteral("abcdef"));
        QCOMPARE(truncated(text, 1, Qt::ElideLeft), QStringLiteral("…"));
        QCOMPARE(truncated(text, 2, Qt::ElideMiddle), QStringLiteral("a…"));
        QCOMPARE(truncated(text, 2, Qt::ElideLeft), QStringLiteral("…j"));
        // A wide character that does not fit becomes a space
        const QString wide = QStringLiteral("ab日本語cd");
        QCOMPARE(truncated(wide, 6, Qt::ElideLeft), QStringLiteral("… 語cd"));
        QCOMPARE(truncated(wide, 7, Qt::ElideLeft), QStringLiteral("…本語cd"));
        QCOMPARE(truncated(wide, 8, Qt::ElideMiddle), QStringLiteral("ab日… cd"));
        QCOMPARE(truncated(wide, 7, Qt::ElideMiddle), QStringLiteral("ab … cd"));
    }

    void textElideModeInTheViews()
    {
        QStringListModel model({QStringLiteral("a long line of text")});
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 8, 2));
        list.setModel(&model);
        QCOMPARE(list.textElideMode(), Qt::ElideRight);
        QCOMPARE(paintedLine(list, 0), QStringLiteral("a long …"));
        list.clearDirtyRegions();
        list.setTextElideMode(Qt::ElideLeft);
        QCOMPARE(list.dirtyRegion(), list.geometry());
        QCOMPARE(paintedLine(list, 0), QStringLiteral("…of text"));
        list.setTextElideMode(Qt::ElideMiddle);
        QCOMPARE(paintedLine(list, 0), QStringLiteral("a lo…ext"));
        list.setTextElideMode(Qt::ElideNone);
        QCOMPARE(paintedLine(list, 0), QStringLiteral("a long l"));

        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 8, 2));
        table.setModel(&model);
        table.setTextElideMode(Qt::ElideLeft);
        QCOMPARE(paintedLine(table, 1), QStringLiteral("…of text"));

        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 10, 2));
        tree.setModel(&model);
        tree.setTextElideMode(Qt::ElideLeft);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("  …of text"));

        // The property works by name, as UI_Loader sets it
        QVERIFY(list.setProperty("textElideMode", QStringLiteral("ElideMiddle")));
        QCOMPARE(list.textElideMode(), Qt::ElideMiddle);
        QVERIFY(list.setProperty("alternatingRowColors", true));
        QVERIFY(list.alternatingRowColors());
    }

    // --- Delegates ---

    void listAndTablePaintThroughTheDelegate()
    {
        QStringListModel strings(numberedRows(3));
        UpperDelegate upper;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 3));
        list.setModel(&strings);
        QCOMPARE(paintedLine(list, 1), QStringLiteral("row 1"));
        list.clearDirtyRegions();
        list.setItemDelegate(&upper);
        QCOMPARE(list.dirtyRegion(), list.geometry());
        QCOMPARE(paintedLine(list, 1), QStringLiteral("ROW 1"));

        GridModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 20, 4));
        table.setModel(&model);
        table.setItemDelegate(&upper);
        // The header does not use the delegate
        QCOMPARE(paintedLine(table, 0), QStringLiteral("H0   H1   H2"));
        QCOMPARE(paintedLine(table, 1), QStringLiteral("R0C0 R0C1 R0C2"));

        // The delegate gets the colors of the item: the current cell
        table.setFocus();
        ScreenBuffer buffer(20, 4);
        table.paintEvent(buffer);
        const ThemeProperties theme = Theme_Engine::instance()->properties(QStringLiteral("QTuiTableView"));
        QCOMPARE(buffer.backCell(0, 1).ch, U'R');
        QCOMPARE(buffer.backCell(0, 1).background, theme.focusBackground);
        QCOMPARE(buffer.backCell(5, 1).background, theme.background);
    }

    void delegatesForRowsAndColumns()
    {
        EditableGridModel model;
        UpperDelegate upper;
        QTuiStyledItemDelegate plain;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 20, 4));
        table.setModel(&model);

        QVERIFY(!table.itemDelegate());
        QTuiItemDelegate *standard = table.itemDelegateForIndex(model.index(0, 0));
        QVERIFY(standard);
        QVERIFY(qobject_cast<QTuiStyledItemDelegate *>(standard));

        table.clearDirtyRegions();
        table.setItemDelegateForColumn(1, &upper);
        QCOMPARE(table.dirtyRegion(), table.geometry());
        QCOMPARE(table.itemDelegateForColumn(1), &upper);
        QVERIFY(!table.itemDelegateForColumn(0));
        QCOMPARE(table.itemDelegateForIndex(model.index(2, 1)), &upper);
        QCOMPARE(table.itemDelegateForIndex(model.index(2, 0)), standard);
        QCOMPARE(paintedLine(table, 1), QStringLiteral("r0c0 R0C1 r0c2"));

        // The delegate of a row comes before the delegate of a column
        table.setItemDelegateForRow(1, &plain);
        QCOMPARE(table.itemDelegateForRow(1), &plain);
        QCOMPARE(table.itemDelegateForIndex(model.index(1, 1)), &plain);
        QCOMPARE(paintedLine(table, 2), QStringLiteral("r1c0 r1c1 r1c2"));
        QCOMPARE(paintedLine(table, 3), QStringLiteral("r2c0 R2C1 r2c2"));

        // The delegate of the item also makes the editor
        QVERIFY(table.edit(model.index(0, 1)));
        QVERIFY(qobject_cast<QTuiSpinBox *>(table.editor()));
        table.closeEditor(false);
        QVERIFY(table.edit(model.index(0, 0)));
        QVERIFY(qobject_cast<QTuiLineEdit *>(table.editor()));
        // A new delegate closes the editor
        table.setItemDelegateForRow(1, nullptr);
        QVERIFY(!table.isEditing());
        QCOMPARE(paintedLine(table, 2), QStringLiteral("r1c0 R1C1 r1c2"));

        // The view delegate is for the items without a row or column delegate
        table.setItemDelegate(&plain);
        QCOMPARE(table.itemDelegateForIndex(model.index(0, 0)), &plain);
        QCOMPARE(table.itemDelegateForIndex(model.index(0, 1)), &upper);
    }

    void deletedDelegateFallsBack()
    {
        GridModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 20, 4));
        table.setModel(&model);
        auto *upper = new UpperDelegate;
        table.setItemDelegateForColumn(0, upper);
        table.setItemDelegateForRow(0, upper);
        delete upper;
        QVERIFY(qobject_cast<QTuiStyledItemDelegate *>(table.itemDelegateForIndex(model.index(0, 0))));
        QCOMPARE(paintedLine(table, 1), QStringLiteral("r0c0 r0c1 r0c2"));
    }

    void treePaintsThroughTheDelegate()
    {
        GroupModel model;
        UpperDelegate upper;
        QTuiStyledItemDelegate plain;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 4));
        tree.setModel(&model);
        tree.expand(model.index(0, 0));
        tree.setItemDelegateForRow(0, &upper);
        // Row 0 of each parent: the two columns of the group "A" and of
        // its child "A0"
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▼ A    A+"));
        QCOMPARE(upper.paints, 4);
        // A styled delegate keeps the selection colors
        tree.setItemDelegate(&plain);
        tree.setFocus();
        tree.setCurrentIndex(model.index(1, 0, model.index(0, 0)));
        ScreenBuffer buffer(20, 4);
        tree.paintEvent(buffer);
        const ThemeProperties theme = Theme_Engine::instance()->properties(QStringLiteral("QTuiTreeView"));
        QCOMPARE(buffer.backCell(4, 2).ch, U'A');
        QCOMPARE(buffer.backCell(4, 2).background, theme.focusBackground);
    }

    // --- Check states and decorations ---

    void checkStatesShowBeforeTheText()
    {
        CheckModel model;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 5));
        list.setModel(&model);
        QCOMPARE(paintedLine(list, 0), QStringLiteral("[ ] item 0"));
        QCOMPARE(paintedLine(list, 1), QStringLiteral("[x] ★ item 1"));
        QCOMPARE(paintedLine(list, 3), QStringLiteral("[x] item 3"));
        // No check state, and a decoration that is not a string
        QCOMPARE(paintedLine(list, 4), QStringLiteral("item 4"));
        model.states[2] = Qt::PartiallyChecked;
        QCOMPARE(paintedLine(list, 2), QStringLiteral("[-] item 2"));

        // The check box stays when the text is too long
        list.setGeometry(QRect(0, 0, 8, 5));
        QCOMPARE(paintedLine(list, 0), QStringLiteral("[ ] ite…"));
        list.setTextElideMode(Qt::ElideLeft);
        QCOMPARE(paintedLine(list, 0), QStringLiteral("[ ] …m 0"));

        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 20, 5));
        table.setModel(&model);
        QCOMPARE(paintedLine(table, 2), QStringLiteral("[x] ★ item 1"));
        // The natural width has space for the check box and the glyph
        QCOMPARE(table.columnWidth(0), 12);

        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 5));
        tree.setModel(&model);
        QCOMPARE(paintedLine(tree, 1), QStringLiteral("  [x] ★ item 1"));
    }

    void spaceTogglesTheCheckState()
    {
        CheckModel model;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 5));
        list.setModel(&model);
        list.clearDirtyRegions();
        sendKey(list, Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QCOMPARE(model.states.at(0), int(Qt::Checked));
        // Only the row of the item repaints
        QCOMPARE(list.dirtyRegion(), QRect(0, 0, 20, 1));
        sendKey(list, Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QCOMPARE(model.states.at(0), int(Qt::Unchecked));

        // Qt::ItemIsUserTristate goes through the three states
        list.setCurrentIndex(model.index(2, 0));
        sendKey(list, Qt::Key_Space);
        QCOMPARE(model.states.at(2), int(Qt::PartiallyChecked));
        sendKey(list, Qt::Key_Space);
        QCOMPARE(model.states.at(2), int(Qt::Checked));
        sendKey(list, Qt::Key_Space);
        QCOMPARE(model.states.at(2), int(Qt::Unchecked));

        // Without Qt::ItemIsUserCheckable, the state stays
        list.setCurrentIndex(model.index(3, 0));
        sendKey(list, Qt::Key_Space);
        QCOMPARE(model.states.at(3), int(Qt::Checked));
    }

    void spaceInTheMultipleSelectionModes()
    {
        CheckModel model;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 5));
        list.setModel(&model);
        list.setSelectionMode(QTuiAbstractItemView::MultiSelection);
        QItemSelectionModel *selection = list.selectionModel();
        // On an item with a check box, Space toggles the check box only
        sendKey(list, Qt::Key_Space);
        QCOMPARE(model.states.at(0), int(Qt::Checked));
        QVERIFY(!selection->isSelected(model.index(0, 0)));
        // Ctrl+Space toggles the selection
        sendKey(list, Qt::Key_Space, Qt::ControlModifier);
        QCOMPARE(model.states.at(0), int(Qt::Checked));
        QVERIFY(selection->isSelected(model.index(0, 0)));
        // On an item without a check box, Space toggles the selection
        list.setCurrentIndex(model.index(4, 0));
        sendKey(list, Qt::Key_Space);
        QVERIFY(selection->isSelected(model.index(4, 0)));
        // Space toggles the check box also in NoSelection
        list.setSelectionMode(QTuiAbstractItemView::NoSelection);
        list.setCurrentIndex(model.index(1, 0));
        sendKey(list, Qt::Key_Space);
        QCOMPARE(model.states.at(1), int(Qt::Unchecked));
    }

    void clickOnTheCheckBoxToggles()
    {
        CheckModel model;
        QTuiListView list;
        list.setGeometry(QRect(0, 0, 20, 5));
        list.setModel(&model);
        click(list, 1, 0);
        QCOMPARE(model.states.at(0), int(Qt::Checked));
        // A click on the text selects only
        click(list, 6, 1);
        QCOMPARE(model.states.at(1), int(Qt::Checked));
        QCOMPARE(list.currentIndex().row(), 1);

        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 20, 5));
        table.setModel(&model);
        click(table, 2, 2); // row 1 is on line 2
        QCOMPARE(model.states.at(1), int(Qt::Unchecked));

        // In the tree, the check box is after the indent
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 20, 5));
        tree.setModel(&model);
        click(tree, 1, 0);
        QCOMPARE(model.states.at(0), int(Qt::Checked));
        click(tree, 3, 0);
        QCOMPARE(model.states.at(0), int(Qt::Unchecked));
    }

    void itemWidgetsShowCheckStates()
    {
        QTuiListWidget list;
        list.setGeometry(QRect(0, 0, 20, 3));
        list.addItem(QStringLiteral("plain"));
        auto *item = new QTuiListWidgetItem(QStringLiteral("task"));
        item->setCheckState(Qt::Unchecked);
        item->setIcon(QStringLiteral("★"));
        list.addItem(item);
        QVERIFY(item->flags() & Qt::ItemIsUserCheckable);
        // An item without a check state has no check box
        QCOMPARE(paintedLine(list, 0), QStringLiteral("plain"));
        QCOMPARE(paintedLine(list, 1), QStringLiteral("[ ] ★ task"));
        list.setCurrentRow(1);
        sendKey(list, Qt::Key_Space);
        QCOMPARE(item->checkState(), Qt::Checked);
        QCOMPARE(paintedLine(list, 1), QStringLiteral("[x] ★ task"));
        list.clearDirtyRegions();
        item->setCheckState(Qt::PartiallyChecked);
        QCOMPARE(list.dirtyRegion(), QRect(0, 1, 20, 1));
        QCOMPARE(paintedLine(list, 1), QStringLiteral("[-] ★ task"));

        // The table measures its columns one time: add the item first
        QTuiTableWidget table(2, 1);
        auto *cell = new QTuiTableWidgetItem(QStringLiteral("cell"));
        cell->setCheckState(Qt::Checked);
        table.setItem(0, 0, cell);
        table.setGeometry(QRect(0, 0, 20, 3));
        QCOMPARE(cell->checkState(), Qt::Checked);
        QCOMPARE(paintedLine(table, 1), QStringLiteral("[x] cell"));
        sendKey(table, Qt::Key_Space);
        QCOMPARE(cell->checkState(), Qt::Unchecked);

        QTuiTreeWidget tree;
        tree.setGeometry(QRect(0, 0, 20, 3));
        auto *node = new QTuiTreeWidgetItem({QStringLiteral("node")});
        node->setCheckState(0, Qt::Checked);
        tree.addTopLevelItem(node);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("  [x] node"));
        sendKey(tree, Qt::Key_Space);
        QCOMPARE(node->checkState(0), Qt::Unchecked);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("  [ ] node"));
    }

    // --- Table headers ---

    void tableHeadersAreNotWidgetsOnTheScreen()
    {
        QTuiTableView table;
        for (QTuiHeaderView *header : {table.horizontalHeader(), table.verticalHeader()}) {
            QCOMPARE(header->focusPolicy(), Qt::NoFocus);
            QVERIFY(header->isPaintedByParent());
        }
        QVERIFY(table.horizontalHeader()->isVisible());
        QVERIFY(!table.verticalHeader()->isVisible());
    }

    void tablePaintsTheHeaderRowWithTheHeaderView()
    {
        GridModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        table.setSortingEnabled(true);
        table.sortByColumn(1, Qt::DescendingOrder);
        QCOMPARE(table.horizontalHeader()->sortIndicatorSection(), 1);
        QCOMPARE(table.horizontalHeader()->sortIndicatorOrder(), Qt::DescendingOrder);
        QVERIFY(table.horizontalHeader()->isSortIndicatorShown());
        QCOMPARE(table.sortColumn(), 1);
        QCOMPARE(paintedLine(table, 0), QStringLiteral("H0   H1 ▼ H2"));
        // The header keeps the indicator
        table.clearDirtyRegions();
        table.horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
        QCOMPARE(table.dirtyRegion(), table.geometry());
        QCOMPARE(paintedLine(table, 0), QStringLiteral("H0 ▲ H1   H2"));
        table.hideColumn(0);
        QCOMPARE(paintedLine(table, 0), QStringLiteral("H1   H2"));

        // A header without a view shows the indicator in the same way
        QTuiHeaderView header(Qt::Horizontal);
        header.setGeometry(QRect(0, 0, 30, 1));
        header.setModel(&model);
        header.setSortIndicator(2, Qt::AscendingOrder);
        QCOMPARE(paintedLine(header, 0), QStringLiteral("H0        H1        H2"));
        header.setSortIndicatorShown(true);
        QCOMPARE(paintedLine(header, 0), QStringLiteral("H0        H1        H2 ▲"));
        header.resizeSection(0, 2);
        QCOMPARE(paintedLine(header, 0), QStringLiteral("H0H1        H2 ▲"));
    }

    void horizontalHeaderCanHide()
    {
        GridModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        sendKey(table, Qt::Key_End);
        table.clearDirtyRegions();
        table.horizontalHeader()->setVisible(false);
        QCOMPARE(table.dirtyRegion(), table.geometry());
        // 6 lines of rows: the last row stays at the bottom
        QVERIFY(paintedLine(table, 0).startsWith(QStringLiteral("r44c0")));
        QVERIFY(paintedLine(table, 5).startsWith(QStringLiteral("r49c0")));
        QCOMPARE(table.visualRect(model.index(44, 0)).y(), 0);
        click(table, 1, 0);
        QCOMPARE(table.currentIndex().row(), 44);
        sendKey(table, Qt::Key_PageDown);
        QCOMPARE(table.currentIndex().row(), 49);
        table.horizontalHeader()->setVisible(true);
        QVERIFY(paintedLine(table, 0).startsWith(QStringLiteral("H0")));
        QVERIFY(paintedLine(table, 5).startsWith(QStringLiteral("r49c0")));
    }

    void verticalHeader()
    {
        GridModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        QCOMPARE(paintedLine(table, 1), QStringLiteral("r0c0 r0c1 r0c2"));
        table.clearDirtyRegions();
        table.verticalHeader()->setVisible(true);
        QCOMPARE(table.dirtyRegion(), table.geometry());
        // 50 rows: 2 cells for the number, and a separator
        QCOMPARE(paintedLine(table, 0), QStringLiteral("   H0   H1   H2"));
        QCOMPARE(paintedLine(table, 1), QStringLiteral("1  r0c0 r0c1 r0c2"));
        QCOMPARE(paintedLine(table, 5), QStringLiteral("5  r4c0 r4c1 r4c2"));
        QCOMPARE(table.visualRect(model.index(0, 0)), QRect(3, 1, 4, 1));

        // The width stays when the view scrolls to the wider numbers
        sendKey(table, Qt::Key_End);
        // (the columns keep the widths of the first rows)
        QCOMPARE(paintedLine(table, 5), QStringLiteral("50 r49… r49… r49…"));
        QCOMPARE(table.visualRect(model.index(49, 0)).x(), 3);
        sendKey(table, Qt::Key_Home);

        // A click uses the same columns. A click on the header does nothing.
        click(table, 8, 2);
        QCOMPARE(table.currentIndex(), model.index(1, 1));
        click(table, 0, 4);
        QCOMPARE(table.currentIndex(), model.index(1, 1));

        // A hidden row keeps the numbers of the other rows
        table.setRowHidden(1, true);
        QCOMPARE(paintedLine(table, 2), QStringLiteral("3  r2c0 r2c1 r2c2"));
        table.setRowHidden(1, false);

        // The last column that stretches ends at the edge of the view
        table.horizontalHeader()->setStretchLastSection(true);
        QCOMPARE(table.visualRect(model.index(0, 2)).right(), 29);
    }

    void verticalHeaderGrowsWithTheRowCount()
    {
        GridModel model;
        QTuiTableView table;
        table.setGeometry(QRect(0, 0, 30, 6));
        table.setModel(&model);
        table.verticalHeader()->setVisible(true);
        ScreenBuffer buffer(30, 6);
        table.paintEvent(buffer);
        table.clearDirtyRegions();
        // Rows below the view: nothing to repaint
        model.insertAt(50, 10);
        QVERIFY(table.dirtyRegion().isNull());
        // 100 rows need 3 cells: all columns move
        model.insertAt(60, 40);
        QCOMPARE(table.dirtyRegion(), table.geometry());
        QCOMPARE(paintedLine(table, 1), QStringLiteral("1   r0c0 r0c1 r0c2"));

        // A large model costs nothing for each row
        model.rows = 2600000;
        model.reset();
        model.dataCalls = 0;
        model.insertAt(2600000, 1);
        QCOMPARE(paintedLine(table, 1), QStringLiteral("1       r0c0 r0c1 r0c2"));
        QVERIFY2(model.dataCalls < 1000, qPrintable(QString::number(model.dataCalls)));
    }

    void tableWidgetShowsVerticalHeaderLabels()
    {
        QTuiTableWidget table(3, 1);
        table.setItem(0, 0, new QTuiTableWidgetItem(QStringLiteral("a")));
        table.setHorizontalHeaderLabels({QStringLiteral("Value")});
        table.setVerticalHeaderLabels({QStringLiteral("first"), QStringLiteral("second")});
        table.setGeometry(QRect(0, 0, 20, 4));
        table.verticalHeader()->setVisible(true);
        QCOMPARE(paintedLine(table, 0), QStringLiteral("       Value"));
        QCOMPARE(paintedLine(table, 1), QStringLiteral("first  a"));
        QCOMPARE(paintedLine(table, 2), QStringLiteral("second"));
        // A row without a label has its number
        QCOMPARE(paintedLine(table, 3), QStringLiteral("3"));

        // A new label can be wider: the view measures again
        table.clearDirtyRegions();
        table.setVerticalHeaderLabels({QStringLiteral("the first row")});
        QCOMPARE(table.dirtyRegion(), table.geometry());
        QCOMPARE(paintedLine(table, 1), QStringLiteral("the first row a"));
    }

    // --- Tree columns and header ---

    void treeShowsAllColumns()
    {
        GroupModel model;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 30, 6));
        tree.setModel(&model);
        // The header row is hidden by default
        QVERIFY(tree.isHeaderHidden());
        QCOMPARE(tree.header()->focusPolicy(), Qt::NoFocus);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▶ A A+"));
        QCOMPARE(paintedLine(tree, 2), QStringLiteral("▶ C C+"));
        // Column 0 has the natural width of the first rows; the last
        // column gets the free width, as in Qt
        QCOMPARE(tree.columnWidth(0), 3);
        QCOMPARE(tree.columnWidth(1), 26);
        QVERIFY(tree.header()->stretchLastSection());

        // An expand makes the column wider for the children that show
        tree.clearDirtyRegions();
        tree.expand(model.index(0, 0));
        QCOMPARE(tree.dirtyRegion(), tree.geometry());
        QCOMPARE(tree.columnWidth(0), 6);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▼ A    A+"));
        QCOMPARE(paintedLine(tree, 1), QStringLiteral("    A0 A0+"));
        // A collapse and a scroll keep the width
        tree.collapse(model.index(0, 0));
        QCOMPARE(tree.columnWidth(0), 6);
        tree.expand(model.index(0, 0));
        sendKey(tree, Qt::Key_End);
        QCOMPARE(tree.columnWidth(0), 6);
        sendKey(tree, Qt::Key_Home);
        // A width that the user sets stays
        tree.setColumnWidth(0, 3);
        tree.expand(model.index(1, 0));
        QCOMPARE(tree.columnWidth(0), 3);
        tree.resizeColumnToContents(0);
        QCOMPARE(tree.columnWidth(0), 6);

        // visualRect() is the text area of each column
        const QModelIndex child = model.index(0, 0, model.index(0, 0));
        QCOMPARE(tree.visualRect(child), QRect(4, 1, 2, 1));
        QCOMPARE(tree.visualRect(child.siblingAtColumn(1)), QRect(7, 1, 23, 1));
    }

    void treeHeaderRow()
    {
        GroupModel model;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 30, 4));
        tree.setModel(&model);
        tree.clearDirtyRegions();
        tree.setHeaderHidden(false);
        QVERIFY(!tree.isHeaderHidden());
        QCOMPARE(tree.dirtyRegion(), tree.geometry());
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("H0  H1"));
        QCOMPARE(paintedLine(tree, 1), QStringLiteral("▶ A A+"));
        ScreenBuffer buffer(30, 4);
        tree.paintEvent(buffer);
        QVERIFY(buffer.backCell(0, 0).bold);

        // The rows use the lines below the header: 3 lines for 3 groups
        tree.expand(model.index(2, 0));
        sendKey(tree, Qt::Key_End);
        QCOMPARE(tree.currentIndex(), model.index(3, 0, model.index(2, 0)));
        QCOMPARE(paintedLine(tree, 3), QStringLiteral("    C3 C3+"));
        QCOMPARE(tree.visualRect(tree.currentIndex().siblingAtColumn(1)).y(), 3);
        // A click on line 1 is the first row that shows
        click(tree, 8, 1);
        QCOMPARE(tree.currentIndex(), model.index(1, 1, model.index(2, 0)));
        // A click on the header does not change the current item
        click(tree, 1, 0);
        QCOMPARE(tree.currentIndex(), model.index(1, 1, model.index(2, 0)));

        // A change of one row repaints its line below the header
        tree.clearDirtyRegions();
        emit model.dataChanged(model.index(2, 0, model.index(2, 0)), model.index(2, 0, model.index(2, 0)));
        QCOMPARE(tree.dirtyRegion(), QRect(0, 2, 30, 1));
    }

    void treeColumnKeys()
    {
        GroupModel model;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 30, 6));
        tree.setModel(&model);
        tree.expand(model.index(0, 0));
        const QModelIndex child = model.index(0, 0, model.index(0, 0));
        tree.setCurrentIndex(child);
        // Right on an item without children goes to the next column
        sendKey(tree, Qt::Key_Right);
        QCOMPARE(tree.currentIndex(), child.siblingAtColumn(1));
        QVERIFY(tree.selectionModel()->isSelected(child.siblingAtColumn(1)));
        // Up and Down keep the column
        sendKey(tree, Qt::Key_Down);
        QCOMPARE(tree.currentIndex(), model.index(1, 1, model.index(0, 0)));
        // Left goes to the column before, and then to the parent
        sendKey(tree, Qt::Key_Left);
        QCOMPARE(tree.currentIndex(), model.index(1, 0, model.index(0, 0)));
        sendKey(tree, Qt::Key_Left);
        QCOMPARE(tree.currentIndex(), model.index(0, 0));
        // Right on an item with children expands it or goes to its child
        sendKey(tree, Qt::Key_Right);
        QCOMPARE(tree.currentIndex(), child);

        // Alt+Left and Alt+Right resize the current column
        sendKey(tree, Qt::Key_Right, Qt::AltModifier);
        QCOMPARE(tree.columnWidth(0), 7);
        sendKey(tree, Qt::Key_Left, Qt::AltModifier);
        sendKey(tree, Qt::Key_Left, Qt::AltModifier);
        QCOMPARE(tree.columnWidth(0), 5);
        tree.setColumnWidth(0, 10);
        QCOMPARE(paintedLine(tree, 1), QStringLiteral("    A0     A0+"));

        // The current cell has the focus colors; the other column does not
        tree.setFocus();
        ScreenBuffer buffer(30, 6);
        tree.paintEvent(buffer);
        const ThemeProperties theme = Theme_Engine::instance()->properties(QStringLiteral("QTuiTreeView"));
        QCOMPARE(buffer.backCell(4, 1).background, theme.focusBackground);
        QCOMPARE(buffer.backCell(11, 1).background, theme.background);
        // With SelectRows, the row is one bar
        tree.setSelectionBehavior(QTuiAbstractItemView::SelectRows);
        sendKey(tree, Qt::Key_Down);
        tree.paintEvent(buffer);
        QCOMPARE(buffer.backCell(4, 2).background, theme.focusBackground);
        QCOMPARE(buffer.backCell(10, 2).background, theme.focusBackground);
        QCOMPARE(buffer.backCell(11, 2).background, theme.focusBackground);
    }

    void treeHiddenColumnsAndScroll()
    {
        GroupModel model;
        QTuiTreeView tree;
        tree.setGeometry(QRect(0, 0, 30, 6));
        tree.setModel(&model);
        tree.setColumnHidden(1, true);
        QVERIFY(tree.isColumnHidden(1));
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▶ A"));
        // The one column that shows gets the free width
        QCOMPARE(tree.columnWidth(0), 30);
        tree.showColumn(1);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▶ A A+"));

        // If the current column becomes hidden, the next one is current
        tree.setCurrentIndex(model.index(1, 1));
        tree.hideColumn(1);
        QCOMPARE(tree.currentIndex(), model.index(1, 0));
        tree.showColumn(1);

        // Columns that do not fit: the view scrolls by column
        tree.header()->setStretchLastSection(false);
        tree.setColumnWidth(0, 20);
        tree.setColumnWidth(1, 20);
        QCOMPARE(tree.horizontalScrollOffset(), 0);
        tree.setCurrentIndex(model.index(1, 1));
        QCOMPARE(tree.horizontalScrollOffset(), 1);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("A+"));
        QCOMPARE(tree.columnWidth(0), 20);
    }

    void treeWidgetShowsColumnsAndHeaderLabels()
    {
        QTuiTreeWidget tree;
        tree.setGeometry(QRect(0, 0, 30, 5));
        auto *src = new QTuiTreeWidgetItem({QStringLiteral("src"), QStringLiteral("dir")});
        src->addChild(new QTuiTreeWidgetItem({QStringLiteral("main.cpp"), QStringLiteral("12 kB")}));
        tree.addTopLevelItem(src);
        // One column and no header until the tree has more
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▶ src"));
        QVERIFY(tree.isHeaderHidden());

        tree.setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Size")});
        QCOMPARE(tree.columnCount(), 2);
        QVERIFY(!tree.isHeaderHidden());
        tree.expand(tree.model()->index(0, 0));
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("Name         Size"));
        QCOMPARE(paintedLine(tree, 1), QStringLiteral("▼ src        dir"));
        QCOMPARE(paintedLine(tree, 2), QStringLiteral("    main.cpp 12 kB"));

        // The text of a column changes on the screen
        tree.clearDirtyRegions();
        src->child(0)->setText(1, QStringLiteral("13 kB"));
        QCOMPARE(tree.dirtyRegion(), QRect(0, 2, 30, 1));
        QCOMPARE(paintedLine(tree, 2), QStringLiteral("    main.cpp 13 kB"));

        tree.setHeaderHidden(true);
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▼ src        dir"));
    }
};

QTEST_MAIN(TestItemViewStructure)
#include "test_item_view_structure.moc"
