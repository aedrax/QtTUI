// Widget Gallery Demo, showcases every public QtTUI widget organized by category.
// Navigate tabs with arrow keys, widgets with Tab/Shift+Tab. Ctrl+Q to quit.

#include <QtTUI/QtTUI>
#include <QStringListModel>

using namespace QtTUI;

// ---------------------------------------------------------------------------
// TabPage: lightweight container widget with a vertical box layout.
// Used as the page widget for each category tab.
// ---------------------------------------------------------------------------
class TabPage : public QTuiWidget {
    Q_OBJECT
public:
    explicit TabPage(QTuiWidget *parent = nullptr)
        : QTuiWidget(parent)
        , m_layout(new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, this))
    {}

    QTuiBoxLayout *layout() const { return m_layout; }

    void addWidget(QTuiWidget *w, int stretch = 0) { m_layout->addWidget(w, stretch); }

    QSize minimumSize() const override { return QSize(0, 1); }

    void paintEvent(ScreenBuffer &buffer) override {
        if (!isVisible() || geometry().isEmpty())
            return;
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
// Section label helper: produces a QTuiLabel styled as a section header
// showing the widget class name (e.g. "── QTuiPushButton ──").
// ---------------------------------------------------------------------------
static QTuiLabel *sectionLabel(const QString &className, QTuiWidget *parent)
{
    auto *label = new QTuiLabel(QStringLiteral("── %1 ──").arg(className), parent);
    return label;
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
    QObject::connect(btn1, &QTuiPushButton::clicked, [statusBar, btn1]() {
        statusBar->showMessage(btn1->text());
    });
    QObject::connect(btn2, &QTuiPushButton::clicked, [statusBar, btn2]() {
        statusBar->showMessage(btn2->text());
    });
    page->addWidget(btn1);
    page->addWidget(btn2);

    // -- QTuiToolButton section --
    page->addWidget(sectionLabel("QTuiToolButton", page));
    auto *toolBtn = new QTuiToolButton("Tool", page);
    page->addWidget(toolBtn);

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
    lineEdit->setText("Type here...");
    QObject::connect(lineEdit, &QTuiLineEdit::textChanged, [statusBar](const QString &text) {
        statusBar->showMessage(QStringLiteral("Text length: %1").arg(text.length()));
    });
    page->addWidget(lineEdit);

    // -- QTuiTextEdit section --
    page->addWidget(sectionLabel("QTuiTextEdit", page));
    auto *textEdit = new QTuiTextEdit(page);
    textEdit->setPlainText("This is a multi-line text editor.\nEdit this sample text freely.");
    page->addWidget(textEdit, 1);

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
        if (role != Qt::DisplayRole || !idx.isValid())
            return {};
        return m_data.at(idx.row()).at(idx.column());
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

    // -- QTuiLabel section (styled text) --
    page->addWidget(sectionLabel("QTuiLabel", page));
    auto *styledLabel = new QTuiLabel(QStringLiteral("Styled label with sample text"), page);
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
    auto *tableModel = new SampleTableModel(page);
    auto *tableView = new QTuiTableView(page);
    tableView->setModel(tableModel);
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

    // -- QTuiSplitter section --
    page->addWidget(sectionLabel("QTuiSplitter", page));
    auto *splitter = new QTuiSplitter(Qt::Horizontal, page);
    splitter->addWidget(new QTuiLabel("Left pane", splitter));
    splitter->addWidget(new QTuiLabel("Right pane", splitter));
    page->addWidget(splitter);

    // -- QTuiScrollArea section --
    page->addWidget(sectionLabel("QTuiScrollArea", page));
    auto *scrollArea = new QTuiScrollArea(page);
    auto *scrollContent = new QTuiLabel(
        "Line 1\nLine 2\nLine 3\nLine 4\nLine 5\n"
        "Line 6\nLine 7\nLine 8\nLine 9\nLine 10",
        scrollArea);
    scrollArea->setWidget(scrollContent);
    page->addWidget(scrollArea);

    // -- QTuiMdiArea section --
    page->addWidget(sectionLabel("QTuiMdiArea", page));
    auto *mdiArea = new QTuiMdiArea(page);
    auto *sub1 = mdiArea->addSubWindow(new QTuiLabel("Sub-window 1", mdiArea));
    sub1->setWindowTitle("Window 1");
    auto *sub2 = mdiArea->addSubWindow(new QTuiLabel("Sub-window 2", mdiArea));
    sub2->setWindowTitle("Window 2");
    page->addWidget(mdiArea);

    // -- QTuiDockWidget section --
    page->addWidget(sectionLabel("QTuiDockWidget", page));
    auto *dock = new QTuiDockWidget("Sample Dock", page);
    dock->setWidget(new QTuiLabel("Dock content", dock));
    page->addWidget(dock);

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
    page->addWidget(undoView);

    // -- QTuiFocusFrame section --
    page->addWidget(sectionLabel("QTuiFocusFrame", page));
    auto *focusFrame = new QTuiFocusFrame(page);
    Q_UNUSED(focusFrame);
    page->addWidget(focusFrame);

    // -- QTuiSizeGrip section --
    page->addWidget(sectionLabel("QTuiSizeGrip", page));
    auto *sizeGrip = new QTuiSizeGrip(page);
    page->addWidget(sizeGrip);

    return page;
}

// ---------------------------------------------------------------------------
// Global event filter for Ctrl+Q quit shortcut.
// ---------------------------------------------------------------------------
struct GalleryFilter : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    bool eventFilter(QObject *, QEvent *ev) override {
        if (ev->type() != TuiEventTypes::keyEventType())
            return false;
        auto *ke = static_cast<TuiKeyEvent *>(ev);
        if (ke->key() == Qt::Key_Q && (ke->modifiers() & Qt::ControlModifier)) {
            QCoreApplication::quit();
            return true;
        }
        return false;
    }
};

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);

    QTuiMainWindow window;
    app.setMainWindow(&window);

    // Menu bar with File > Quit
    auto *menuBar = new QTuiMenuBar(&window);
    auto *fileMenu = menuBar->addMenu("&File");
    auto *quitAction = fileMenu->addAction("&Quit", "Ctrl+Q");
    QObject::connect(quitAction, &QTuiAction::triggered, &app, &QCoreApplication::quit);
    window.setMenuBar(menuBar);

    // Status bar
    auto *statusBar = new QTuiStatusBar(&window);
    statusBar->showMessage("Widget Gallery - Use Tab/Shift+Tab to navigate");
    window.setStatusBar(statusBar);

    // Central tab widget with 8 category tabs
    auto *tabs = new QTuiTabWidget(&window);
    tabs->addTab(createButtonsTab(&window, statusBar),         "Buttons & Toggles");
    tabs->addTab(createTextInputTab(&window, statusBar),        "Text Input");
    tabs->addTab(createNumericInputTab(&window, statusBar),    "Numeric Input");
    tabs->addTab(createSelectionTab(&window, statusBar),         "Selection & Display");
    tabs->addTab(createModelViewTab(&window, statusBar),        "Model/View");
    tabs->addTab(createContainersTab(&window, statusBar),        "Containers");
    tabs->addTab(createMainWindowPartsTab(&window, statusBar), "Main Window Parts");
    tabs->addTab(createSpecializedTab(&window, statusBar),      "Specialized");

    // Update status bar when switching tabs
    QObject::connect(tabs, &QTuiTabWidget::currentChanged, [statusBar, tabs](int idx) {
        statusBar->showMessage(QStringLiteral("Tab: %1").arg(tabs->tabBar()->tabText(idx)));
    });

    window.setCentralWidget(tabs);

    // Give the tab bar initial focus so arrow keys switch tabs
    tabs->tabBar()->setFocus();

    // Install global Ctrl+Q filter
    auto *filter = new GalleryFilter;
    filter->setParent(&app);
    app.installEventFilter(filter);

    window.show();
    app.triggerRender();

    return app.exec();
}

#include "gallery.moc"
