// Widget Gallery builder: every gallery tab, the theme, the menu and the
// status bar. The demo (gallery.cpp) and tests/test_gallery.cpp both use it,
// so the test always checks the real gallery.

#include "GalleryBuilder.h"
#include <QtTUI/QtTUI>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <algorithm>
#include <functional>
#include <QTimer>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// TabPage: lightweight container widget with a box layout.
// A vertical TabPage is the page widget for each category tab.
// A horizontal TabPage is a row of widgets inside a page.
// ---------------------------------------------------------------------------
class TabPage : public QTuiWidget {
    Q_OBJECT
public:
    explicit TabPage(QTuiWidget *parent = nullptr,
                     QTuiBoxLayout::Direction direction = QTuiBoxLayout::TopToBottom)
        : QTuiWidget(parent)
        , m_layout(new QTuiBoxLayout(direction, this))
    {
        if (direction == QTuiBoxLayout::TopToBottom)
            m_layout->setContentsMargins(2, 0, 2, 0);
    }

    QTuiBoxLayout *layout() const override { return m_layout; }

    void addWidget(QTuiWidget *w, int stretch = 0) { m_layout->addWidget(w, stretch); }

    QSize minimumSize() const override {
        if (m_layout->direction() == QTuiBoxLayout::LeftToRight)
            return m_layout->minimumSize();
        return QSize(0, 1);
    }

    void paintEvent(ScreenBuffer &buffer) override {
        if (!isVisible() || geometry().isEmpty())
            return;
        clearBackground(buffer, geometry());
        m_layout->apply(geometry());
        for (auto *child : childWidgets()) {
            if (child->isVisible())
                child->paintEvent(buffer);
        }
    }

private:
    QTuiBoxLayout *m_layout;
};

// ---------------------------------------------------------------------------
// Gallery palette: a dark background with one accent color.
// Focus uses an inverted accent block, so focus stays visible without color.
// ---------------------------------------------------------------------------
namespace Palette {
const Color Background(26, 27, 38);
const Color Surface(36, 40, 59);
const Color Text(192, 202, 245);
const Color Muted(86, 95, 137);
const Color Accent(122, 162, 247);
}

void applyGalleryTheme()
{
    ThemeProperties base;
    base.foreground = Palette::Text;
    base.background = Palette::Background;
    base.focusForeground = Palette::Background;
    base.focusBackground = Palette::Accent;
    base.disabledForeground = Palette::Muted;
    base.disabledBackground = Palette::Background;

    ThemeProperties bar = base;
    bar.background = Palette::Surface;

    QHash<QString, ThemeProperties> theme;
    theme.insert(QStringLiteral("default"), base);
    theme.insert(QStringLiteral("QTuiMenuBar"), bar);
    theme.insert(QStringLiteral("QTuiStatusBar"), bar);

    auto *engine = Theme_Engine::instance();
    engine->registerTheme(QStringLiteral("gallery"), theme);
    engine->setActiveTheme(QStringLiteral("gallery"));
}

// ---------------------------------------------------------------------------
// Section label helper: produces a bold accent QTuiLabel as a section header
// that shows the widget class name (e.g. "▍ QTuiPushButton").
// The first section on a page gets no blank line above it.
// ---------------------------------------------------------------------------
static QTuiLabel *sectionLabel(const QString &className, QTuiWidget *parent)
{
    auto *page = qobject_cast<TabPage *>(parent);
    if (page && page->layout()->count() > 0)
        page->addWidget(new QTuiLabel(QString(), parent));

    auto *label = new QTuiLabel(QStringLiteral("▍ %1").arg(className), parent);
    // Bold accent text on the background of the current theme; follow a
    // theme change (View menu)
    auto applyHeaderStyle = [label]() {
        ThemeProperties header = Theme_Engine::instance()->properties(QStringLiteral("default"));
        header.foreground = Theme_Engine::instance()->activeTheme() == QLatin1String("light")
            ? header.focusBackground : Palette::Accent;
        header.bold = true;
        Theme_Engine::instance()->setWidgetOverride(label, header);
    };
    applyHeaderStyle();
    QObject::connect(Theme_Engine::instance(), &Theme_Engine::themeChanged, label, applyHeaderStyle);
    return label;
}

// ---------------------------------------------------------------------------
// Row helper: puts widgets side by side at their minimum width, with a
// one-cell gap between them. A stretch takes the remaining width.
// ---------------------------------------------------------------------------
static TabPage *row(TabPage *page, const QList<QTuiWidget *> &widgets)
{
    auto *r = new TabPage(page, QTuiBoxLayout::LeftToRight);
    r->layout()->setSpacing(1);
    for (auto *w : widgets) {
        w->setParent(r);
        r->addWidget(w);
    }
    r->layout()->addStretch();
    return r;
}

// ---------------------------------------------------------------------------
// Tab builder: Buttons & Toggles
// ---------------------------------------------------------------------------
static QTuiWidget *createButtonsTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiPushButton section --
    page->addWidget(sectionLabel("QTuiPushButton", page));
    auto *btn1 = new QTuiPushButton("Click Me", page);
    auto *btn2 = new QTuiPushButton("Another Button", page);
    btn1->setToolTip("Opens a QTuiProgressDialog");
    btn2->setToolTip("Opens a QTuiMessageBox");
    // A simulated copy: a timer moves a progress dialog to 50 in 5 seconds
    QObject::connect(btn1, &QTuiPushButton::clicked, [statusBar]() {
        auto *progress = new QTuiProgressDialog("Copying files...", "Cancel", 0, 50);
        progress->setWindowTitle("QTuiProgressDialog");
        progress->setMinimumDuration(0);
        auto *timer = new QTimer(progress);
        QObject::connect(timer, &QTimer::timeout, progress, [progress, statusBar]() {
            progress->setValue(progress->value() + 1);
            if (progress->value() >= progress->maximum()) {
                statusBar->showMessage("Copy done");
                progress->deleteLater();
            }
        });
        QObject::connect(progress, &QTuiProgressDialog::canceled, progress, [progress, statusBar]() {
            statusBar->showMessage("Copy canceled");
            progress->deleteLater();
        });
        progress->setValue(0);
        timer->start(100);
    });
    QObject::connect(btn2, &QTuiPushButton::clicked, [statusBar]() {
        const auto answer = QTuiMessageBox::question(
            nullptr, "QTuiMessageBox",
            "Do you like the modal dialog? Tab moves between the buttons, "
            "Enter chooses, and Escape cancels.",
            QTuiMessageBox::Yes | QTuiMessageBox::No | QTuiMessageBox::Cancel,
            QTuiMessageBox::Yes);
        const char *name = answer == QTuiMessageBox::Yes ? "Yes"
            : answer == QTuiMessageBox::No ? "No" : "Cancel";
        statusBar->showMessage(QStringLiteral("Message box answer: %1").arg(name));
    });
    page->addWidget(row(page, {btn1, btn2}));

    // -- QTuiToolButton section --
    page->addWidget(sectionLabel("QTuiToolButton", page));
    auto *toolBtn = new QTuiToolButton("Tool", page);
    toolBtn->setToolTip("Opens a QTuiInputDialog");
    QObject::connect(toolBtn, &QTuiToolButton::clicked, [statusBar]() {
        bool ok = false;
        const QString name = QTuiInputDialog::getText(nullptr, "QTuiInputDialog",
                                                      "Your name:", "Ada", &ok);
        statusBar->showMessage(ok ? QStringLiteral("Hello, %1").arg(name)
                                  : QStringLiteral("Input canceled"));
    });
    page->addWidget(row(page, {toolBtn}));

    // -- QTuiCheckBox section --
    page->addWidget(sectionLabel("QTuiCheckBox", page));
    auto *chk1 = new QTuiCheckBox("Checked option", page);
    chk1->setChecked(true);
    auto *chk2 = new QTuiCheckBox("Unchecked option", page);
    page->addWidget(chk1);
    page->addWidget(chk2);

    // -- QTuiRadioButton + QTuiButtonGroup section --
    page->addWidget(sectionLabel("QTuiRadioButton", page));
    auto *radio1 = new QTuiRadioButton("Option A", page);
    auto *radio2 = new QTuiRadioButton("Option B", page);
    auto *radio3 = new QTuiRadioButton("Option C", page);
    radio1->setChecked(true);

    auto *group = new QTuiButtonGroup(page);
    group->addButton(radio1, 0);
    group->addButton(radio2, 1);
    group->addButton(radio3, 2);

    page->addWidget(radio1);
    page->addWidget(radio2);
    page->addWidget(radio3);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Text Input
// ---------------------------------------------------------------------------
static QTuiWidget *createTextInputTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiLineEdit section --
    page->addWidget(sectionLabel("QTuiLineEdit", page));
    auto *lineEdit = new QTuiLineEdit(page);
    lineEdit->setText(QString());
    lineEdit->setToolTip("Type a color: completions show below");
    lineEdit->setPlaceholderText("Type a color, for example Blue");
    auto *completer = new QTuiCompleter({"Amber", "Aqua", "Azure", "Beige", "Black", "Blue",
                                         "Coral", "Crimson", "Cyan", "Gold", "Gray", "Green"},
                                        lineEdit);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    lineEdit->setCompleter(completer);
    QObject::connect(lineEdit, &QTuiLineEdit::textChanged, [statusBar](const QString &text) {
        statusBar->showMessage(QStringLiteral("Text length: %1").arg(text.length()));
    });
    page->addWidget(lineEdit);

    // -- QTuiTextEdit section --
    page->addWidget(sectionLabel("QTuiTextEdit", page));
    auto *textEdit = new QTuiTextEdit(page);
    textEdit->setPlainText("This is a multi-line text editor.\nEdit this sample text freely.");
    page->addWidget(textEdit, 1);

    // -- QTuiPlainTextEdit section: a log that follows new lines --
    page->addWidget(sectionLabel("QTuiPlainTextEdit", page));
    auto *log = new QTuiPlainTextEdit(page);
    log->setReadOnly(true);
    log->setMaximumBlockCount(100);
    log->appendPlainText("Log: a line every second; move up to stop following");
    auto *logTimer = new QTimer(log);
    QObject::connect(logTimer, &QTimer::timeout, log, [log]() {
        static int count = 0;
        log->appendPlainText(QStringLiteral("Event %1 at %2")
                                 .arg(++count)
                                 .arg(QTime::currentTime().toString(Qt::ISODate)));
    });
    logTimer->start(1000);
    page->addWidget(log, 1);

    // -- QTuiDateTimeEdit section --
    page->addWidget(sectionLabel("QTuiDateTimeEdit", page));
    auto *dateTimeEdit = new QTuiDateTimeEdit(QDateTime(QDate(2025, 6, 15), QTime(14, 30, 0)), page);
    page->addWidget(dateTimeEdit);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Numeric Input
// ---------------------------------------------------------------------------
static QTuiWidget *createNumericInputTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiSpinBox section --
    page->addWidget(sectionLabel("QTuiSpinBox", page));
    auto *spinBox = new QTuiSpinBox(page);
    spinBox->setRange(0, 100);
    spinBox->setValue(42);
    page->addWidget(spinBox);

    // -- QTuiDoubleSpinBox section --
    page->addWidget(sectionLabel("QTuiDoubleSpinBox", page));
    auto *dblSpinBox = new QTuiDoubleSpinBox(page);
    dblSpinBox->setRange(0.0, 1.0);
    dblSpinBox->setSingleStep(0.1);
    dblSpinBox->setDecimals(1);
    dblSpinBox->setValue(0.5);
    page->addWidget(dblSpinBox);

    // -- QTuiSlider section --
    page->addWidget(sectionLabel("QTuiSlider", page));
    auto *slider = new QTuiSlider(Qt::Horizontal, page);
    slider->setRange(0, 100);
    page->addWidget(slider);

    // -- QTuiDial section --
    page->addWidget(sectionLabel("QTuiDial", page));
    auto *dial = new QTuiDial(page);
    page->addWidget(dial);

    // -- QTuiLCDNumber section (linked to slider) --
    page->addWidget(sectionLabel("QTuiLCDNumber", page));
    auto *lcd = new QTuiLCDNumber(3, page);
    lcd->display(slider->value());
    QObject::connect(slider, &QTuiSlider::valueChanged, lcd, qOverload<int>(&QTuiLCDNumber::display));
    page->addWidget(lcd);

    Q_UNUSED(statusBar);
    return page;
}

// ---------------------------------------------------------------------------
// SampleTableModel: local model for the Model/View tab's QTuiTableView.
// 3 columns ("Name", "Type", "Value") with 5 rows of sample data.
// ---------------------------------------------------------------------------
class SampleTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SampleTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , m_headers{QStringLiteral("Name"), QStringLiteral("Type"), QStringLiteral("Value")}
        , m_data{{QStringLiteral("Width"),   QStringLiteral("int"),    QStringLiteral("800")},
                 {QStringLiteral("Height"),  QStringLiteral("int"),    QStringLiteral("600")},
                 {QStringLiteral("Title"),   QStringLiteral("string"), QStringLiteral("Demo")},
                 {QStringLiteral("Visible"), QStringLiteral("bool"),   QStringLiteral("true")},
                 {QStringLiteral("Opacity"), QStringLiteral("double"), QStringLiteral("0.95")}}
    {}

    int rowCount(const QModelIndex & = {}) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = {}) const override { return m_headers.size(); }

    QVariant data(const QModelIndex &idx, int role) const override {
        if ((role != Qt::DisplayRole && role != Qt::EditRole) || !idx.isValid())
            return {};
        return m_data.at(idx.row()).at(idx.column());
    }

    // The Value column is editable (F2 or a double click)
    Qt::ItemFlags flags(const QModelIndex &idx) const override {
        Qt::ItemFlags f = QAbstractTableModel::flags(idx);
        if (idx.column() == 2)
            f |= Qt::ItemIsEditable;
        return f;
    }

    bool setData(const QModelIndex &idx, const QVariant &value, int role) override {
        if (role != Qt::EditRole || !(flags(idx) & Qt::ItemIsEditable))
            return false;
        m_data[idx.row()][idx.column()] = value.toString();
        emit dataChanged(idx, idx);
        return true;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};
        return m_headers.at(section);
    }

private:
    QStringList m_headers;
    QVector<QStringList> m_data;
};

// ---------------------------------------------------------------------------
// SampleCommand: minimal QTuiUndoCommand subclass for the undo stack demo.
// ---------------------------------------------------------------------------
class SampleCommand : public QTuiUndoCommand {
public:
    explicit SampleCommand(const QString &text)
        : QTuiUndoCommand(text) {}
    void undo() override {}
    void redo() override {}
};

// ---------------------------------------------------------------------------
// SimpleTreeNode / SimpleTreeModel: lightweight tree model for Qt6::Core only
// (QStandardItemModel lives in Qt6::Gui which is not linked).
// ---------------------------------------------------------------------------
class SimpleTreeNode {
public:
    explicit SimpleTreeNode(const QString &text, SimpleTreeNode *parent = nullptr)
        : m_text(text), m_parent(parent)
    {
        if (m_parent)
            m_parent->m_children.append(this);
    }
    ~SimpleTreeNode() { qDeleteAll(m_children); }

    QString text() const { return m_text; }
    SimpleTreeNode *parent() const { return m_parent; }
    SimpleTreeNode *child(int row) const { return m_children.value(row); }
    int childCount() const { return m_children.size(); }
    int row() const {
        if (m_parent)
            return m_parent->m_children.indexOf(const_cast<SimpleTreeNode*>(this));
        return 0;
    }

private:
    QString m_text;
    SimpleTreeNode *m_parent = nullptr;
    QList<SimpleTreeNode*> m_children;
};

class SimpleTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    explicit SimpleTreeModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent), m_root(new SimpleTreeNode(QString())) {}
    ~SimpleTreeModel() override { delete m_root; }

    SimpleTreeNode *rootNode() const { return m_root; }

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override {
        if (!hasIndex(row, column, parent)) return {};
        auto *parentNode = parent.isValid()
            ? static_cast<SimpleTreeNode*>(parent.internalPointer()) : m_root;
        auto *child = parentNode->child(row);
        return child ? createIndex(row, column, child) : QModelIndex();
    }

    QModelIndex parent(const QModelIndex &idx) const override {
        if (!idx.isValid()) return {};
        auto *child = static_cast<SimpleTreeNode*>(idx.internalPointer());
        auto *parentNode = child->parent();
        if (!parentNode || parentNode == m_root) return {};
        return createIndex(parentNode->row(), 0, parentNode);
    }

    int rowCount(const QModelIndex &parent = {}) const override {
        auto *node = parent.isValid()
            ? static_cast<SimpleTreeNode*>(parent.internalPointer()) : m_root;
        return node->childCount();
    }

    int columnCount(const QModelIndex & = {}) const override { return 1; }

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override {
        if (!idx.isValid() || role != Qt::DisplayRole) return {};
        return static_cast<SimpleTreeNode*>(idx.internalPointer())->text();
    }

    bool hasChildren(const QModelIndex &parent = {}) const override {
        auto *node = parent.isValid()
            ? static_cast<SimpleTreeNode*>(parent.internalPointer()) : m_root;
        return node->childCount() > 0;
    }

private:
    SimpleTreeNode *m_root;
};

// ---------------------------------------------------------------------------
// Tab builder: Selection & Display
// ---------------------------------------------------------------------------
static QTuiWidget *createSelectionTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiComboBox section --
    page->addWidget(sectionLabel("QTuiComboBox", page));
    auto *combo = new QTuiComboBox(page);
    combo->addItems({QStringLiteral("Red"), QStringLiteral("Green"),
                     QStringLiteral("Blue"), QStringLiteral("Yellow")});
    combo->setCurrentIndex(0);
    QObject::connect(combo, &QTuiComboBox::currentTextChanged, [statusBar](const QString &text) {
        statusBar->showMessage(QStringLiteral("ComboBox: %1").arg(text));
    });
    page->addWidget(combo);

    // -- QTuiProgressBar section --
    page->addWidget(sectionLabel("QTuiProgressBar", page));
    auto *progress = new QTuiProgressBar(page);
    progress->setRange(0, 100);
    progress->setValue(65);
    page->addWidget(progress);

    // -- QTuiProgressBar in busy mode: setRange(0, 0) --
    page->addWidget(sectionLabel("QTuiProgressBar (busy)", page));
    auto *busy = new QTuiProgressBar(page);
    busy->setRange(0, 0);
    page->addWidget(busy);

    // -- QTuiLabel section (styled text) --
    page->addWidget(sectionLabel("QTuiLabel", page));
    auto *styledLabel = new QTuiLabel(
        QStringLiteral("Markdown: **bold**, *italic*, `code` and ~~strikethrough~~"), page);
    styledLabel->setTextFormat(Qt::MarkdownText);
    page->addWidget(styledLabel);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Model/View
// ---------------------------------------------------------------------------
static QTuiWidget *createModelViewTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiTableView section --
    page->addWidget(sectionLabel("QTuiTableView", page));
    // A sort proxy in front of the model: a header click sorts the table
    auto *sourceModel = new SampleTableModel(page);
    auto *tableModel = new QSortFilterProxyModel(page);
    tableModel->setSourceModel(sourceModel);
    auto *tableView = new QTuiTableView(page);
    tableView->setModel(tableModel);
    tableView->setSortingEnabled(true);
    tableView->setToolTip("F2 or a double click edits a value");
    QObject::connect(tableView, &QTuiTableView::activated, [statusBar, tableModel](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        const QString name = tableModel->data(tableModel->index(idx.row(), 0), Qt::DisplayRole).toString();
        statusBar->showMessage(QStringLiteral("Table: %1").arg(name));
    });
    page->addWidget(tableView, 1);

    // -- QTuiListView section --
    page->addWidget(sectionLabel("QTuiListView", page));
    auto *listModel = new QStringListModel(
        {QStringLiteral("Alpha"), QStringLiteral("Bravo"), QStringLiteral("Charlie"),
         QStringLiteral("Delta"), QStringLiteral("Echo")}, page);
    auto *listView = new QTuiListView(page);
    listView->setModel(listModel);
    // Shift and Ctrl with the arrow keys, Space or a click select more items
    listView->setSelectionMode(QTuiAbstractItemView::ExtendedSelection);
    QObject::connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged,
                     [listView, statusBar]() {
        const int count = listView->selectionModel()->selectedIndexes().size();
        statusBar->showMessage(QStringLiteral("List: %1 selected").arg(count));
    });
    // Right click (or Shift+F10) opens a menu with the list actions
    listView->setContextMenuPolicy(Qt::ActionsContextMenu);
    listView->setToolTip("Shift+arrows or a drag select more; Shift+F10 or a right click opens a menu");
    auto *addItem = new QTuiAction("Add item", listView);
    QObject::connect(addItem, &QTuiAction::triggered, [listModel, statusBar]() {
        const int row = listModel->rowCount();
        listModel->insertRows(row, 1);
        listModel->setData(listModel->index(row), QStringLiteral("Item %1").arg(row + 1));
        statusBar->showMessage("Added an item");
    });
    auto *removeItem = new QTuiAction("Remove selected items", listView);
    QObject::connect(removeItem, &QTuiAction::triggered, [listModel, listView, statusBar]() {
        // Remove from the last row, so the other row numbers stay correct
        QList<int> rows;
        for (const QModelIndex &index : listView->selectionModel()->selectedIndexes())
            rows.append(index.row());
        std::sort(rows.begin(), rows.end(), std::greater<int>());
        for (int row : rows)
            listModel->removeRows(row, 1);
        statusBar->showMessage(QStringLiteral("Removed %1 items").arg(rows.size()));
    });
    listView->addAction(addItem);
    listView->addAction(removeItem);
    page->addWidget(listView, 1);

    // -- QTuiTreeView section --
    page->addWidget(sectionLabel("QTuiTreeView", page));
    auto *treeModel = new SimpleTreeModel(page);
    auto *fruits = new SimpleTreeNode(QStringLiteral("Fruits"), treeModel->rootNode());
    new SimpleTreeNode(QStringLiteral("Apple"), fruits);
    new SimpleTreeNode(QStringLiteral("Banana"), fruits);
    auto *vegetables = new SimpleTreeNode(QStringLiteral("Vegetables"), treeModel->rootNode());
    new SimpleTreeNode(QStringLiteral("Carrot"), vegetables);
    new SimpleTreeNode(QStringLiteral("Broccoli"), vegetables);

    auto *treeView = new QTuiTreeView(page);
    treeView->setModel(treeModel);
    page->addWidget(treeView, 1);

    // -- QTuiColumnView section (shares tree model) --
    page->addWidget(sectionLabel("QTuiColumnView", page));
    auto *columnView = new QTuiColumnView(page);
    columnView->setModel(treeModel);
    page->addWidget(columnView, 1);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Containers
// ---------------------------------------------------------------------------
static QTuiWidget *createContainersTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    Q_UNUSED(statusBar);
    auto *page = new TabPage(parent);

    // -- QTuiGroupBox section --
    page->addWidget(sectionLabel("QTuiGroupBox", page));
    auto *groupBox = new QTuiGroupBox("Sample Group", page);
    auto *groupLayout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    groupLayout->addWidget(new QTuiLabel("Child widget 1", groupBox));
    groupLayout->addWidget(new QTuiLabel("Child widget 2", groupBox));
    groupBox->setLayout(groupLayout);
    page->addWidget(groupBox);

    // -- QTuiCollapsible section --
    page->addWidget(sectionLabel("QTuiCollapsible", page));
    auto *collapsible = new QTuiCollapsible("Collapsible Section", page);
    collapsible->setExpanded(true);
    new QTuiLabel("Expanded content", collapsible);
    page->addWidget(collapsible);

    // -- QTuiTabWidget section (nested) --
    page->addWidget(sectionLabel("QTuiTabWidget", page));
    auto *nestedTabs = new QTuiTabWidget(page);
    nestedTabs->addTab(new QTuiLabel("Page A content", nestedTabs), "Page A");
    nestedTabs->addTab(new QTuiLabel("Page B content", nestedTabs), "Page B");
    page->addWidget(nestedTabs);

    // -- QTuiToolBox section --
    page->addWidget(sectionLabel("QTuiToolBox", page));
    auto *toolBox = new QTuiToolBox(page);
    toolBox->addItem(new QTuiLabel("Tool item 1", toolBox), "Tool 1");
    toolBox->addItem(new QTuiLabel("Tool item 2", toolBox), "Tool 2");
    page->addWidget(toolBox);

    // -- QTuiStackedWidget section --
    page->addWidget(sectionLabel("QTuiStackedWidget", page));
    auto *stacked = new QTuiStackedWidget(page);
    stacked->addWidget(new QTuiLabel("Stacked page 0", stacked));
    stacked->addWidget(new QTuiLabel("Stacked page 1", stacked));
    stacked->setCurrentIndex(0);
    page->addWidget(stacked);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Panes
// ---------------------------------------------------------------------------
static QTuiWidget *createPanesTab(QTuiWidget *parent)
{
    auto *page = new TabPage(parent);

    // -- QTuiSplitter section --
    page->addWidget(sectionLabel("QTuiSplitter", page));
    auto *splitter = new QTuiSplitter(Qt::Horizontal, page);
    splitter->addWidget(new QTuiLabel("Left pane", splitter));
    splitter->addWidget(new QTuiLabel("Right pane", splitter));
    page->addWidget(splitter);

    // -- QTuiScrollArea section --
    page->addWidget(sectionLabel("QTuiScrollArea", page));
    auto *scrollArea = new QTuiScrollArea(page);
    // One check box on each line: the hover highlights the line under the
    // mouse, because a click on it does something
    auto *scrollContent = new QTuiWidget(scrollArea);
    for (int i = 0; i < 10; ++i) {
        auto *line = new QTuiCheckBox(QStringLiteral("Line %1").arg(i + 1), scrollContent);
        line->setGeometry(QRect(0, i, 12, 1));
    }
    // The scroll area does not size its content, so set the size here
    scrollContent->setGeometry(QRect(0, 0, 12, 10));
    scrollArea->setWidget(scrollContent);
    page->addWidget(scrollArea, 1);

    // -- QTuiMdiArea section --
    page->addWidget(sectionLabel("QTuiMdiArea", page));
    auto *mdiArea = new QTuiMdiArea(page);
    auto *sub1 = mdiArea->addSubWindow(new QTuiLabel("Sub-window 1", mdiArea));
    sub1->setWindowTitle("Window 1");
    auto *sub2 = mdiArea->addSubWindow(new QTuiLabel("Sub-window 2", mdiArea));
    sub2->setWindowTitle("Window 2");
    page->addWidget(mdiArea, 2);

    // -- QTuiDockWidget section --
    page->addWidget(sectionLabel("QTuiDockWidget", page));
    auto *dock = new QTuiDockWidget("Sample Dock", page);
    dock->setWidget(new QTuiLabel("Dock content", dock));
    page->addWidget(dock);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Layouts. Each QTuiGroupBox applies one layout type.
// ---------------------------------------------------------------------------
static QTuiWidget *createLayoutsTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiFormLayout section: label and field rows --
    page->addWidget(sectionLabel("QTuiFormLayout", page));
    auto *formBox = new QTuiGroupBox("Account", page);
    auto *form = new QTuiFormLayout;
    auto *nameEdit = new QTuiLineEdit(formBox);
    nameEdit->setText("Ada Lovelace");
    auto *emailEdit = new QTuiLineEdit(formBox);
    emailEdit->setText("ada@example.com");
    form->setHorizontalSpacing(1);
    form->addRow("Name:", nameEdit);
    form->addRow("Email:", emailEdit);
    auto *passwordEdit = new QTuiLineEdit(formBox);
    passwordEdit->setEchoMode(QTuiLineEdit::Password);
    passwordEdit->setText("secret");
    form->addRow("Password:", passwordEdit);
    auto *ageEdit = new QTuiLineEdit(formBox);
    ageEdit->setValidator(new QTuiIntValidator(0, 150, ageEdit));
    ageEdit->setPlaceholderText("0 to 150");
    form->addRow("Age:", ageEdit);
    form->addRow("Admin:", new QTuiCheckBox("Can change settings", formBox));
    formBox->setLayout(form);
    page->addWidget(formBox);

    // -- QTuiGridLayout section: a 3x3 keypad with stretched columns --
    page->addWidget(sectionLabel("QTuiGridLayout", page));
    auto *gridBox = new QTuiGroupBox("Keypad", page);
    auto *grid = new QTuiGridLayout;
    for (int i = 0; i < 9; ++i) {
        const QString digit = QString::number(9 - i);
        auto *key = new QTuiToolButton(digit, gridBox);
        QObject::connect(key, &QTuiToolButton::clicked, [statusBar, digit]() {
            statusBar->showMessage(QStringLiteral("Keypad: %1").arg(digit));
        });
        // Rows 7 8 9 / 4 5 6 / 1 2 3, as on a calculator keypad
        grid->addWidget(key, i / 3, 2 - i % 3);
    }
    for (int column = 0; column < 3; ++column)
        grid->setColumnStretch(column, 1);
    gridBox->setLayout(grid);
    page->addWidget(gridBox);

    // -- QTuiStackedLayout section: one page at a time --
    page->addWidget(sectionLabel("QTuiStackedLayout", page));
    auto *nextPage = new QTuiToolButton("Next page", page);
    page->addWidget(row(page, {nextPage}));
    auto *stackBox = new QTuiGroupBox("Pages", page);
    auto *stack = new QTuiStackedLayout;
    stack->addWidget(new QTuiLabel("Page 1 of 3: only the current page is visible", stackBox));
    stack->addWidget(new QTuiLabel("Page 2 of 3: the other pages are hidden", stackBox));
    stack->addWidget(new QTuiLabel("Page 3 of 3: Next page goes back to page 1", stackBox));
    stack->setCurrentIndex(0);
    stackBox->setLayout(stack);
    QObject::connect(nextPage, &QTuiToolButton::clicked, [stack, stackBox]() {
        stack->setCurrentIndex((stack->currentIndex() + 1) % stack->count());
        stackBox->markDirty();
    });
    page->addWidget(stackBox);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Main Window Parts
// ---------------------------------------------------------------------------
static QTuiWidget *createMainWindowPartsTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    Q_UNUSED(statusBar);
    auto *page = new TabPage(parent);

    // -- QTuiMenuBar section --
    page->addWidget(sectionLabel("QTuiMenuBar", page));
    auto *sampleMenuBar = new QTuiMenuBar(page);
    auto *editMenu = sampleMenuBar->addMenu("&Edit");
    editMenu->addAction("&Copy", "Ctrl+C");
    editMenu->addAction("&Paste", "Ctrl+V");
    auto *viewMenu = sampleMenuBar->addMenu("&View");
    viewMenu->addAction("Zoom &In");
    viewMenu->addAction("Zoom &Out");
    page->addWidget(sampleMenuBar);

    // -- QTuiToolBar section --
    page->addWidget(sectionLabel("QTuiToolBar", page));
    auto *toolBar = new QTuiToolBar("Sample Toolbar", page);
    toolBar->addAction(new QTuiAction("Open", toolBar));
    toolBar->addAction(new QTuiAction("Save", toolBar));
    toolBar->addAction(new QTuiAction("Close", toolBar));
    page->addWidget(toolBar);

    // -- QTuiStatusBar section --
    page->addWidget(sectionLabel("QTuiStatusBar", page));
    auto *sampleStatusBar = new QTuiStatusBar(page);
    sampleStatusBar->showMessage("Permanent sample message");
    page->addWidget(sampleStatusBar);

    // -- QTuiDockWidget section --
    page->addWidget(sectionLabel("QTuiDockWidget", page));
    auto *dock = new QTuiDockWidget("Info Panel", page);
    dock->setWidget(new QTuiLabel("Dock widget content", dock));
    page->addWidget(dock);

    return page;
}

// ---------------------------------------------------------------------------
// Tab builder: Specialized
// ---------------------------------------------------------------------------
static QTuiWidget *createSpecializedTab(QTuiWidget *parent, QTuiStatusBar *statusBar)
{
    auto *page = new TabPage(parent);

    // -- QTuiCalendarWidget section --
    page->addWidget(sectionLabel("QTuiCalendarWidget", page));
    auto *calendar = new QTuiCalendarWidget(page);
    calendar->setSelectedDate(QDate::currentDate());
    QObject::connect(calendar, &QTuiCalendarWidget::selectionChanged, [statusBar, calendar]() {
        statusBar->showMessage(QStringLiteral("Date: %1").arg(calendar->selectedDate().toString(Qt::ISODate)));
    });
    page->addWidget(calendar);

    // -- QTuiUndoView section --
    page->addWidget(sectionLabel("QTuiUndoView", page));
    auto *undoStack = new QTuiUndoStack(page);
    undoStack->push(new SampleCommand("Add item"));
    undoStack->push(new SampleCommand("Change color"));
    auto *undoView = new QTuiUndoView(undoStack, page);
    page->addWidget(undoView, 1);

    // -- QTuiFocusFrame section --
    page->addWidget(sectionLabel("QTuiFocusFrame", page));
    auto *focusFrame = new QTuiFocusFrame(page);
    Q_UNUSED(focusFrame);
    page->addWidget(focusFrame);

    // -- QTuiSizeGrip section --
    page->addWidget(sectionLabel("QTuiSizeGrip", page));
    // The grip resizes the sub-window that it is in: drag it, or give it
    // the focus and use the arrow keys
    auto *gripArea = new QTuiMdiArea(page);
    auto *gripContent = new QTuiWidget;
    auto *gripColumn = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    gripColumn->addWidget(new QTuiLabel("Drag the grip", gripContent));
    gripColumn->addStretch(1);
    auto *gripRow = new QTuiWidget(gripContent);
    auto *gripRowLayout = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    gripRowLayout->addStretch(1);
    gripRowLayout->addWidget(new QTuiSizeGrip(gripRow));
    gripRow->setLayout(gripRowLayout);
    gripColumn->addWidget(gripRow);
    gripContent->setLayout(gripColumn);
    QTuiMdiSubWindow *gripWindow = gripArea->addSubWindow(gripContent);
    gripWindow->setWindowTitle("Resize me");
    gripWindow->setGeometry(QRect(0, 0, 20, 7));
    page->addWidget(gripArea, 1);

    return page;
}

// ---------------------------------------------------------------------------
// Builds the menu bar, the status bar and the 10 category tabs in window
// ---------------------------------------------------------------------------
void buildGallery(QTuiMainWindow &window)
{
    // Menu bar with File > Quit
    auto *menuBar = new QTuiMenuBar(&window);
    auto *fileMenu = menuBar->addMenu("&File");
    auto *openAction = fileMenu->addAction("&Open...", "Ctrl+O");
    auto *newAction = fileMenu->addAction("&New project...", "Ctrl+N");
    auto *quitAction = fileMenu->addAction("&Quit", "Ctrl+Q");
    QObject::connect(quitAction, &QTuiAction::triggered, qApp, &QCoreApplication::quit);
    // View > Light theme / Dark theme switch between the built-in light
    // theme and the gallery theme
    auto *viewMenu = menuBar->addMenu("&View");
    auto *lightAction = viewMenu->addAction("&Light theme");
    auto *darkAction = viewMenu->addAction("&Dark theme");
    QObject::connect(lightAction, &QTuiAction::triggered, []() {
        Theme_Engine::instance()->setActiveTheme("light");
    });
    QObject::connect(darkAction, &QTuiAction::triggered, []() {
        if (Theme_Engine::instance()->activeTheme() != QLatin1String("gallery"))
            applyGalleryTheme();
    });
    window.setMenuBar(menuBar);

    // Status bar
    auto *statusBar = new QTuiStatusBar(&window);
    const QString keyHints = QStringLiteral(" ←/→ category · Tab/Shift+Tab focus · F1 tip · Ctrl+Q quit");
    statusBar->showMessage(keyHints);
    window.setStatusBar(statusBar);

    // A two-step wizard; the name page is complete when the name is not empty
    QObject::connect(newAction, &QTuiAction::triggered, [statusBar]() {
        class NamePage : public QTuiWizardPage {
        public:
            NamePage()
            {
                setTitle("Project name");
                edit = new QTuiLineEdit(this);
                edit->setPlaceholderText("Type a name");
                auto *l = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
                l->addWidget(new QTuiLabel("Name of the new project:", this));
                l->addWidget(edit);
                setLayout(l);
                QObject::connect(edit, &QTuiLineEdit::textChanged, this, &QTuiWizardPage::completeChanged);
            }
            bool isComplete() const override { return !edit->text().trimmed().isEmpty(); }
            QTuiLineEdit *edit;
        };
        QTuiWizard wizard;
        wizard.setWindowTitle("QTuiWizard");
        auto *namePage = new NamePage;
        auto *donePage = new QTuiWizardPage;
        donePage->setTitle("Ready");
        auto *doneLayout = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
        doneLayout->addWidget(new QTuiLabel("Finish creates the project.", donePage));
        donePage->setLayout(doneLayout);
        wizard.addPage(namePage);
        wizard.addPage(donePage);
        const bool done = wizard.exec() == QTuiDialog::Accepted;
        statusBar->showMessage(done ? QStringLiteral("Created project %1").arg(namePage->edit->text())
                                    : QStringLiteral("New project canceled"));
    });

    QObject::connect(openAction, &QTuiAction::triggered, [statusBar]() {
        const QString path = QTuiFileDialog::getOpenFileName(nullptr, "Open a file");
        statusBar->showMessage(path.isEmpty() ? QStringLiteral("Open canceled")
                                              : QStringLiteral("Chosen: %1").arg(path));
    });

    // Central tab widget with 10 category tabs
    auto *tabs = new QTuiTabWidget(&window);
    tabs->addTab(createButtonsTab(&window, statusBar),         "Buttons");
    tabs->addTab(createTextInputTab(&window, statusBar),       "Text");
    tabs->addTab(createNumericInputTab(&window, statusBar),    "Numeric");
    tabs->addTab(createSelectionTab(&window, statusBar),       "Selection");
    tabs->addTab(createModelViewTab(&window, statusBar),       "Model/View");
    tabs->addTab(createContainersTab(&window, statusBar),      "Containers");
    tabs->addTab(createPanesTab(&window),                      "Panes");
    tabs->addTab(createLayoutsTab(&window, statusBar),         "Layouts");
    tabs->addTab(createMainWindowPartsTab(&window, statusBar), "Main Window");
    tabs->addTab(createSpecializedTab(&window, statusBar),     "Specialized");

    // Show the key hints again when the user switches tabs
    QObject::connect(tabs, &QTuiTabWidget::currentChanged, [statusBar, keyHints](int) {
        statusBar->showMessage(keyHints);
    });

    window.setCentralWidget(tabs);

    // Give the tab bar initial focus so arrow keys switch tabs
    tabs->tabBar()->setFocus();
}

#include "GalleryBuilder.moc"
