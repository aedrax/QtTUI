#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QAbstractTableModel>

#include <QtTUI/QTuiItemDelegate.h>
#include <QtTUI/QTuiStyledItemDelegate.h>
#include <QtTUI/QTuiHeaderView.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiSpinBox.h>
#include <QtTUI/QTuiComboBox.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QtTUI/Theme_Engine.h>

using namespace QtTUI;

static void sendKey(QTuiWidget &w, int key, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    TuiKeyEvent ev(key, mods);
    QCoreApplication::sendEvent(&w, &ev);
}

// Minimal concrete widget for use as editor parent
class DummyParentWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit DummyParentWidget(QTuiWidget *parent = nullptr) : QTuiWidget(parent) {}
    void paintEvent(ScreenBuffer &) override {}
};

// Flexible test model supporting DisplayRole, EditRole, UserRole, and headerData
class TestModel : public QAbstractTableModel {
    Q_OBJECT
public:
    TestModel(int rows, int cols, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r)
            m_data[r].resize(cols);
        m_hHeaders.resize(cols);
        m_vHeaders.resize(rows);
        for (int c = 0; c < cols; ++c)
            m_hHeaders[c] = QStringLiteral("Col%1").arg(c);
        for (int r = 0; r < rows; ++r)
            m_vHeaders[r] = QStringLiteral("Row%1").arg(r);
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_data.isEmpty() ? 0 : m_data[0].size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
            return {};
        int r = index.row(), c = index.column();
        if (r < 0 || r >= rowCount() || c < 0 || c >= columnCount())
            return {};
        const auto &cell = m_data[r][c];
        if (role == Qt::DisplayRole)
            return cell.display;
        if (role == Qt::EditRole)
            return cell.edit.isValid() ? cell.edit : cell.display;
        if (role == Qt::UserRole)
            return cell.userData;
        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (!index.isValid())
            return false;
        int r = index.row(), c = index.column();
        if (r < 0 || r >= rowCount() || c < 0 || c >= columnCount())
            return false;
        if (role == Qt::EditRole) {
            m_data[r][c].edit = value;
            m_data[r][c].display = value;
            emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
            return true;
        }
        if (role == Qt::UserRole) {
            m_data[r][c].userData = value;
            return true;
        }
        return false;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        if (orientation == Qt::Horizontal) {
            if (section < 0 || section >= m_hHeaders.size())
                return {};
            return m_hHeaders[section];
        } else {
            if (section < 0 || section >= m_vHeaders.size())
                return {};
            return m_vHeaders[section];
        }
    }

    void setDisplayData(int row, int col, const QVariant &value)
    {
        m_data[row][col].display = value;
    }
    void setEditData(int row, int col, const QVariant &value)
    {
        m_data[row][col].edit = value;
    }
    void setUserData(int row, int col, const QVariant &value)
    {
        m_data[row][col].userData = value;
    }
    void setHHeader(int col, const QString &label) { m_hHeaders[col] = label; }
    void setVHeader(int row, const QString &label) { m_vHeaders[row] = label; }

private:
    struct CellData {
        QVariant display;
        QVariant edit;
        QVariant userData;
    };
    QVector<QVector<CellData>> m_data;
    QVector<QString> m_hHeaders;
    QVector<QString> m_vHeaders;
};

class TestQTuiDelegatesHeaderView : public QObject {
    Q_OBJECT
private slots:

    // ---------------------------------------------------------------
    // QTuiItemDelegate tests
    // ---------------------------------------------------------------

    void itemDelegate_paintRendersDisplayRole()
    {
        QTuiItemDelegate delegate;
        ScreenBuffer buffer(20, 5);
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Hello"));

        QRect rect(2, 1, 10, 1);
        delegate.paint(buffer, rect, model.index(0, 0));

        QCOMPARE(buffer.backCell(2, 1).character, QChar('H'));
        QCOMPARE(buffer.backCell(3, 1).character, QChar('e'));
        QCOMPARE(buffer.backCell(4, 1).character, QChar('l'));
        QCOMPARE(buffer.backCell(5, 1).character, QChar('l'));
        QCOMPARE(buffer.backCell(6, 1).character, QChar('o'));
    }

    void itemDelegate_paintSkipsInvalidIndex()
    {
        QTuiItemDelegate delegate;
        ScreenBuffer buffer(10, 5);
        QRect rect(0, 0, 5, 1);
        delegate.paint(buffer, rect, QModelIndex());
        QCOMPARE(buffer.backCell(0, 0).character, QChar(' '));
    }

    void itemDelegate_createEditorReturnsNullptr()
    {
        QTuiItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("test"));

        QTuiWidget *editor = delegate.createEditor(&parent, model.index(0, 0));
        QVERIFY(editor == nullptr);
    }

    void itemDelegate_setEditorDataNoOp()
    {
        QTuiItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("test"));

        // Should not crash
        delegate.setEditorData(nullptr, model.index(0, 0));
        delegate.setEditorData(&parent, model.index(0, 0));
    }

    void itemDelegate_setModelDataNoOp()
    {
        QTuiItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("test"));

        // Should not crash
        delegate.setModelData(nullptr, &model, model.index(0, 0));
        delegate.setModelData(&parent, &model, model.index(0, 0));
    }

    void itemDelegate_sizeHintValidIndex()
    {
        QTuiItemDelegate delegate;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Hello"));

        QSize hint = delegate.sizeHint(model.index(0, 0));
        QCOMPARE(hint.width(), 5);  // "Hello" length
        QCOMPARE(hint.height(), 1);
    }

    void itemDelegate_sizeHintInvalidIndex()
    {
        QTuiItemDelegate delegate;
        QSize hint = delegate.sizeHint(QModelIndex());
        QCOMPARE(hint.width(), 0);
        QCOMPARE(hint.height(), 1);
    }

    void itemDelegate_signalsExist()
    {
        QTuiItemDelegate delegate;
        QSignalSpy commitSpy(&delegate, &QTuiItemDelegate::commitData);
        QSignalSpy closeSpy(&delegate, &QTuiItemDelegate::closeEditor);
        QVERIFY(commitSpy.isValid());
        QVERIFY(closeSpy.isValid());
    }

    // ---------------------------------------------------------------
    // QTuiStyledItemDelegate tests
    // ---------------------------------------------------------------

    void styledDelegate_paintUsesThemeColors()
    {
        QTuiStyledItemDelegate delegate;
        ScreenBuffer buffer(20, 5);
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Styled"));

        ThemeProperties props = Theme_Engine::instance()->properties(
            QStringLiteral("QTuiStyledItemDelegate"));

        QRect rect(0, 0, 10, 1);
        delegate.paint(buffer, rect, model.index(0, 0));

        // Verify themed colors on text cell
        const Cell &cell = buffer.backCell(0, 0);
        QCOMPARE(cell.character, QChar('S'));
        QCOMPARE(cell.foreground, props.foreground);
        QCOMPARE(cell.background, props.background);
        QCOMPARE(cell.bold, props.bold);
        QCOMPARE(cell.italic, props.italic);
        QCOMPARE(cell.underline, props.underline);

        // Verify background fill beyond text
        const Cell &bgCell = buffer.backCell(7, 0);
        QCOMPARE(bgCell.character, QChar(' '));
        QCOMPARE(bgCell.foreground, props.foreground);
        QCOMPARE(bgCell.background, props.background);
    }

    void styledDelegate_createEditorLineEditForString()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("text"));
        model.setEditData(0, 0, QStringLiteral("text"));

        QTuiWidget *editor = delegate.createEditor(&parent, model.index(0, 0));
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QTuiLineEdit *>(editor) != nullptr);
        delete editor;
    }

    void styledDelegate_createEditorSpinBoxForInt()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, 42);
        model.setEditData(0, 0, 42);

        QTuiWidget *editor = delegate.createEditor(&parent, model.index(0, 0));
        QVERIFY(editor != nullptr);
        QVERIFY(qobject_cast<QTuiSpinBox *>(editor) != nullptr);
        delete editor;
    }

    void styledDelegate_createEditorComboBoxForStringList()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Option A"));
        model.setEditData(0, 0, QStringLiteral("Option A"));
        model.setUserData(0, 0,
            QStringList{QStringLiteral("Option A"), QStringLiteral("Option B"), QStringLiteral("Option C")});

        QTuiWidget *editor = delegate.createEditor(&parent, model.index(0, 0));
        QVERIFY(editor != nullptr);
        auto *combo = qobject_cast<QTuiComboBox *>(editor);
        QVERIFY(combo != nullptr);
        QCOMPARE(combo->count(), 3);
        delete editor;
    }

    void styledDelegate_setEditorDataLineEdit()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Hello"));
        model.setEditData(0, 0, QStringLiteral("Hello"));

        auto *lineEdit = new QTuiLineEdit(&parent);
        delegate.setEditorData(lineEdit, model.index(0, 0));
        QCOMPARE(lineEdit->text(), QStringLiteral("Hello"));
    }

    void styledDelegate_setEditorDataSpinBox()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, 77);
        model.setEditData(0, 0, 77);

        auto *spinBox = new QTuiSpinBox(&parent);
        spinBox->setRange(0, 100);
        delegate.setEditorData(spinBox, model.index(0, 0));
        QCOMPARE(spinBox->value(), 77);
    }

    void styledDelegate_setEditorDataComboBox()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Option B"));
        model.setEditData(0, 0, QStringLiteral("Option B"));

        auto *combo = new QTuiComboBox(&parent);
        combo->addItems({QStringLiteral("Option A"), QStringLiteral("Option B"), QStringLiteral("Option C")});
        delegate.setEditorData(combo, model.index(0, 0));
        QCOMPARE(combo->currentIndex(), 1);
    }

    void styledDelegate_setModelDataLineEdit()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("old"));
        model.setEditData(0, 0, QStringLiteral("old"));

        auto *lineEdit = new QTuiLineEdit(&parent);
        lineEdit->setText(QStringLiteral("new value"));
        delegate.setModelData(lineEdit, &model, model.index(0, 0));
        QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toString(), QStringLiteral("new value"));
    }

    void styledDelegate_setModelDataSpinBox()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, 10);
        model.setEditData(0, 0, 10);

        auto *spinBox = new QTuiSpinBox(&parent);
        spinBox->setRange(0, 100);
        spinBox->setValue(55);
        delegate.setModelData(spinBox, &model, model.index(0, 0));
        QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toInt(), 55);
    }

    void styledDelegate_setModelDataComboBox()
    {
        QTuiStyledItemDelegate delegate;
        DummyParentWidget parent;
        TestModel model(1, 1);
        model.setDisplayData(0, 0, QStringLiteral("Option A"));
        model.setEditData(0, 0, QStringLiteral("Option A"));

        auto *combo = new QTuiComboBox(&parent);
        combo->addItems({QStringLiteral("Option A"), QStringLiteral("Option B"), QStringLiteral("Option C")});
        combo->setCurrentIndex(2);
        delegate.setModelData(combo, &model, model.index(0, 0));
        QCOMPARE(model.data(model.index(0, 0), Qt::EditRole).toString(), QStringLiteral("Option C"));
    }

    // ---------------------------------------------------------------
    // QTuiHeaderView tests
    // ---------------------------------------------------------------

    void headerView_constructHorizontal()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        QCOMPARE(hv.orientation(), Qt::Horizontal);
        QCOMPARE(hv.count(), 0);
        QVERIFY(hv.model() == nullptr);
    }

    void headerView_constructVertical()
    {
        QTuiHeaderView hv(Qt::Vertical);
        QCOMPARE(hv.orientation(), Qt::Vertical);
        QCOMPARE(hv.count(), 0);
    }

    void headerView_setModelInitializesSections()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        TestModel model(3, 4);
        hv.setModel(&model);

        QCOMPARE(hv.model(), &model);
        QCOMPARE(hv.count(), 4);  // 4 columns for horizontal
    }

    void headerView_verticalCountReturnsRows()
    {
        QTuiHeaderView hv(Qt::Vertical);
        TestModel model(5, 2);
        hv.setModel(&model);

        QCOMPARE(hv.count(), 5);  // 5 rows for vertical
    }

    void headerView_sectionSizeDefault()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        TestModel model(2, 3);
        hv.setModel(&model);

        // Default section size is 10
        QCOMPARE(hv.sectionSize(0), 10);
        QCOMPARE(hv.sectionSize(1), 10);
        QCOMPARE(hv.sectionSize(2), 10);
    }

    void headerView_resizeSectionChangesSize()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        TestModel model(2, 3);
        hv.setModel(&model);

        hv.resizeSection(1, 20);
        QCOMPARE(hv.sectionSize(1), 20);
    }

    void headerView_resizeSectionEmitsSignal()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        TestModel model(2, 3);
        hv.setModel(&model);

        QSignalSpy spy(&hv, &QTuiHeaderView::sectionResized);
        hv.resizeSection(0, 15);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 0);   // logicalIndex
        QCOMPARE(spy.at(0).at(1).toInt(), 10);  // oldSize (default)
        QCOMPARE(spy.at(0).at(2).toInt(), 15);  // newSize
    }

    void headerView_resizeSectionEnforcesMinimum()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        TestModel model(2, 3);
        hv.setModel(&model);

        hv.resizeSection(0, 1);  // Below minimum of 2
        QCOMPARE(hv.sectionSize(0), 2);  // Clamped to MinSectionSize
    }

    void headerView_sectionHiddenShowHide()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        TestModel model(2, 3);
        hv.setModel(&model);

        QCOMPARE(hv.isSectionHidden(0), false);
        hv.setSectionHidden(0, true);
        QCOMPARE(hv.isSectionHidden(0), true);
        hv.setSectionHidden(0, false);
        QCOMPARE(hv.isSectionHidden(0), false);
    }

    void headerView_paintRendersHeaderLabels()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        hv.setGeometry(QRect(0, 0, 40, 1));
        TestModel model(2, 3);
        model.setHHeader(0, QStringLiteral("Name"));
        model.setHHeader(1, QStringLiteral("Age"));
        model.setHHeader(2, QStringLiteral("City"));
        hv.setModel(&model);

        ScreenBuffer buffer(40, 5);
        hv.paintEvent(buffer);

        // "Name" starts at x=0 (section 0, default size 10)
        QCOMPARE(buffer.backCell(0, 0).character, QChar('N'));
        QCOMPARE(buffer.backCell(1, 0).character, QChar('a'));
        QCOMPARE(buffer.backCell(2, 0).character, QChar('m'));
        QCOMPARE(buffer.backCell(3, 0).character, QChar('e'));

        // "Age" starts at x=10 (section 1)
        QCOMPARE(buffer.backCell(10, 0).character, QChar('A'));
        QCOMPARE(buffer.backCell(11, 0).character, QChar('g'));
        QCOMPARE(buffer.backCell(12, 0).character, QChar('e'));

        // "City" starts at x=20 (section 2)
        QCOMPARE(buffer.backCell(20, 0).character, QChar('C'));
        QCOMPARE(buffer.backCell(21, 0).character, QChar('i'));
        QCOMPARE(buffer.backCell(22, 0).character, QChar('t'));
        QCOMPARE(buffer.backCell(23, 0).character, QChar('y'));
    }

    void headerView_paintSkipsHiddenSections()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        hv.setGeometry(QRect(0, 0, 40, 1));
        TestModel model(2, 3);
        model.setHHeader(0, QStringLiteral("AAA"));
        model.setHHeader(1, QStringLiteral("BBB"));
        model.setHHeader(2, QStringLiteral("CCC"));
        hv.setModel(&model);

        hv.setSectionHidden(1, true);

        ScreenBuffer buffer(40, 5);
        hv.paintEvent(buffer);

        // "AAA" at x=0
        QCOMPARE(buffer.backCell(0, 0).character, QChar('A'));
        // "CCC" follows at x=10 (section 0 width), skipping hidden BBB
        QCOMPARE(buffer.backCell(10, 0).character, QChar('C'));
        QCOMPARE(buffer.backCell(11, 0).character, QChar('C'));
        QCOMPARE(buffer.backCell(12, 0).character, QChar('C'));
    }

    void headerView_keyNavigateHorizontal()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        hv.setGeometry(QRect(0, 0, 40, 1));
        TestModel model(2, 3);
        hv.setModel(&model);
        hv.setFocus();

        // Move right twice to section 2
        sendKey(hv, Qt::Key_Right);
        sendKey(hv, Qt::Key_Right);

        QSignalSpy spy(&hv, &QTuiHeaderView::sectionClicked);
        sendKey(hv, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 2);
    }

    void headerView_keyNavigateVertical()
    {
        QTuiHeaderView hv(Qt::Vertical);
        hv.setGeometry(QRect(0, 0, 10, 5));
        TestModel model(3, 2);
        hv.setModel(&model);
        hv.setFocus();

        sendKey(hv, Qt::Key_Down);
        QSignalSpy spy(&hv, &QTuiHeaderView::sectionClicked);
        sendKey(hv, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void headerView_keyEnterEmitsSectionClicked()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        hv.setGeometry(QRect(0, 0, 40, 1));
        TestModel model(2, 3);
        hv.setModel(&model);
        hv.setFocus();

        QSignalSpy spy(&hv, &QTuiHeaderView::sectionClicked);
        sendKey(hv, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 0);
    }

    void headerView_keyPlusMinusResizeSection()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        hv.setGeometry(QRect(0, 0, 40, 1));
        TestModel model(2, 3);
        hv.setModel(&model);
        hv.setFocus();

        int origSize = hv.sectionSize(0);
        QSignalSpy spy(&hv, &QTuiHeaderView::sectionResized);

        sendKey(hv, Qt::Key_Plus);
        QCOMPARE(hv.sectionSize(0), origSize + 1);
        QCOMPARE(spy.count(), 1);

        sendKey(hv, Qt::Key_Minus);
        QCOMPARE(hv.sectionSize(0), origSize);
        QCOMPARE(spy.count(), 2);
    }

    void headerView_navigationSkipsHiddenSections()
    {
        QTuiHeaderView hv(Qt::Horizontal);
        hv.setGeometry(QRect(0, 0, 40, 1));
        TestModel model(2, 4);
        hv.setModel(&model);
        hv.setFocus();

        // Hide sections 1 and 2
        hv.setSectionHidden(1, true);
        hv.setSectionHidden(2, true);

        // Start at section 0, press Right → should skip to section 3
        sendKey(hv, Qt::Key_Right);

        QSignalSpy spy(&hv, &QTuiHeaderView::sectionClicked);
        sendKey(hv, Qt::Key_Return);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 3);
    }
};

QTEST_MAIN(TestQTuiDelegatesHeaderView)
#include "test_qtui_delegates_headerview.moc"
