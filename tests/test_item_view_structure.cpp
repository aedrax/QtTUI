// Tests for the structure features of the item views: hidden rows, the
// root index, alternating row colors, the elide mode, delegates, check
// states, headers, tree columns and spans.

#include <QtTest/QtTest>
#include <QtTUI/QtTUI>
#include <QtTUI/RenderUtils.h>
#include <QStringListModel>
#include <QAbstractTableModel>

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
        QCOMPARE(paintedLine(tree, 0).trimmed(), QStringLiteral("A0"));
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
        QCOMPARE(paintedLine(tree, 0).trimmed(), QStringLiteral("▶ A"));

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
        // Row 0 of each parent: the group "A" and its child "A0"
        QCOMPARE(paintedLine(tree, 0), QStringLiteral("▼ A"));
        QCOMPARE(upper.paints, 2);
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
};

QTEST_MAIN(TestItemViewStructure)
#include "test_item_view_structure.moc"
