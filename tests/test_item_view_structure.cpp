// Tests for the structure features of the item views: hidden rows, the
// root index, alternating row colors, the elide mode, delegates, check
// states, headers, tree columns and spans.

#include <QtTest/QtTest>
#include <QtTUI/QtTUI>
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
};

QTEST_MAIN(TestItemViewStructure)
#include "test_item_view_structure.moc"
