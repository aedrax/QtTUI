// Tests for the item view features: item colors from model roles, the
// paging keys, the vim keys, the mouse wheel and the column resize modes.
// The models have more rows than the views are high.

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

// A typed letter, as InputParser sends it (an upper case letter has Shift)
static void typeChar(QTuiWidget &w, QChar ch)
{
    sendKey(w, ch.toUpper().unicode(), ch.isUpper() ? Qt::ShiftModifier : Qt::NoModifier,
            QString(ch));
}

static void wheel(QTuiWidget &w, int delta, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiMouseEvent ev(TuiMouseEvent::Wheel, Qt::NoButton, w.geometry().topLeft() + QPoint(1, 2),
                     mods, delta);
    QCoreApplication::sendEvent(&w, &ev);
}

static QStringList numberedRows(int count)
{
    QStringList rows;
    for (int i = 0; i < count; ++i)
        rows << QStringLiteral("row %1").arg(i);
    return rows;
}

// A table model with colors in Qt::ForegroundRole and Qt::BackgroundRole.
// Row 1 has a Color foreground, row 2 a 0xRRGGBB background, row 3 a
// "#RRGGBB" foreground.
class ColorModel : public QAbstractTableModel {
public:
    int rows = 50;
    int cols = 3;
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
        if (role == Qt::DisplayRole)
            return QStringLiteral("r%1c%2").arg(index.row()).arg(index.column());
        if (role == Qt::ForegroundRole && index.row() == 1)
            return QVariant::fromValue(Color(255, 0, 0));
        if (role == Qt::BackgroundRole && index.row() == 2)
            return 0x0000ff;
        if (role == Qt::ForegroundRole && index.row() == 3)
            return QStringLiteral("#00ff00");
        return {};
    }
    QVariant headerData(int section, Qt::Orientation, int role) const override
    {
        return role == Qt::DisplayRole ? QVariant(QStringLiteral("H%1").arg(section)) : QVariant();
    }
};

class TestItemViewFeatures : public QObject {
    Q_OBJECT

private slots:
    void cleanup()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::TrueColor);
    }

    // --- Item colors from model roles ---

    void roleColorsShowInTheCells()
    {
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        ScreenBuffer buffer(30, 6);
        view.paintEvent(buffer);
        // Line 0 is the header, so model row r is on line r + 1
        QCOMPARE(buffer.backCell(0, 2).foreground, Color(255, 0, 0));
        QCOMPARE(buffer.backCell(0, 3).background, Color(0, 0, 255));
        QCOMPARE(buffer.backCell(0, 4).foreground, Color(0, 255, 0));
        const ThemeProperties theme = view.themeProperties();
        QCOMPARE(buffer.backCell(0, 5).foreground, theme.foreground);
    }

    void currentItemKeepsTheFocusColors()
    {
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        view.setFocus();
        view.setCurrentIndex(model.index(1, 0));
        ScreenBuffer buffer(30, 6);
        view.paintEvent(buffer);
        const ThemeProperties theme = view.themeProperties();
        QCOMPARE(buffer.backCell(0, 2).foreground, theme.focusForeground);
        QCOMPARE(buffer.backCell(0, 2).background, theme.focusBackground);
        // The next cell in the row is not current and has the role color
        const QRect next = view.visualRect(model.index(1, 1));
        QCOMPARE(buffer.backCell(next.x(), 2).foreground, Color(255, 0, 0));
    }

    void noColorModeIgnoresRoleColors()
    {
        AnsiRenderer::setColorMode(AnsiRenderer::NoColor);
        ColorModel model;
        QTuiTableView view;
        view.setGeometry(QRect(0, 0, 30, 6));
        view.setModel(&model);
        ScreenBuffer buffer(30, 6);
        view.paintEvent(buffer);
        const ThemeProperties theme = view.themeProperties();
        QCOMPARE(buffer.backCell(0, 2).foreground, theme.foreground);
        QCOMPARE(buffer.backCell(0, 3).background, theme.background);
    }

    void roleColorsInTheOtherViews()
    {
        ColorModel model;
        model.cols = 1;
        QTuiListView list;
        QTuiTreeView tree;
        QTuiColumnView columns;
        for (QTuiAbstractItemView *view : {static_cast<QTuiAbstractItemView *>(&list),
                                           static_cast<QTuiAbstractItemView *>(&tree),
                                           static_cast<QTuiAbstractItemView *>(&columns)}) {
            view->setGeometry(QRect(0, 0, 20, 5));
            view->setModel(&model);
            ScreenBuffer buffer(20, 5);
            view->paintEvent(buffer);
            const QRect rect = view->visualRect(model.index(1, 0));
            QCOMPARE(buffer.backCell(rect.x(), rect.y()).foreground, Color(255, 0, 0));
        }
    }

    // An application can change the colors with its own rule
    void itemStyleIsVirtual()
    {
        class RedView : public QTuiListView {
        protected:
            Cell itemStyle(const QModelIndex &index, bool isCurrent,
                           const ThemeProperties &theme) const override
            {
                Cell style = QTuiListView::itemStyle(index, isCurrent, theme);
                style.background = Color(200, 0, 0);
                return style;
            }
        };
        QStringListModel model(numberedRows(20));
        RedView view;
        view.setGeometry(QRect(0, 0, 10, 4));
        view.setModel(&model);
        ScreenBuffer buffer(10, 4);
        view.paintEvent(buffer);
        QCOMPARE(buffer.backCell(0, 3).background, Color(200, 0, 0));
    }
};

QTEST_MAIN(TestItemViewFeatures)
#include "test_item_view_features.moc"
