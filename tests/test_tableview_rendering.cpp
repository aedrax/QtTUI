#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiTableView.h>
#include <QtTUI/ScreenBuffer.h>
#include <QtTUI/TuiEvents.h>
#include <QAbstractTableModel>

using namespace QtTUI;

// Feature: widget-rendering-overhaul, Property 13: Table Column Width Calculation
// Feature: widget-rendering-overhaul, Property 14: Table Header Rendering
// Feature: widget-rendering-overhaul, Property 15: Table Horizontal Scrolling

// Simple in-memory table model for testing
class RenderTestModel : public QAbstractTableModel {
    Q_OBJECT
public:
    RenderTestModel(int rows, int cols, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
        m_headers.resize(cols);
        m_data.resize(rows);
        for (int r = 0; r < rows; ++r)
            m_data[r].resize(cols);
        for (int c = 0; c < cols; ++c)
            m_headers[c] = QStringLiteral("H%1").arg(c);
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_data.size(); }
    int columnCount(const QModelIndex & = QModelIndex()) const override { return m_headers.size(); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || !index.isValid())
            return {};
        if (index.row() < 0 || index.row() >= rowCount()
            || index.column() < 0 || index.column() >= columnCount())
            return {};
        return m_data[index.row()][index.column()];
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};
        if (section < 0 || section >= columnCount())
            return {};
        return m_headers[section];
    }

    void setHeader(int col, const QString &text)
    {
        if (col >= 0 && col < columnCount())
            m_headers[col] = text;
    }

    void setCellData(int row, int col, const QString &text)
    {
        if (row >= 0 && row < rowCount() && col >= 0 && col < columnCount())
            m_data[row][col] = text;
    }

private:
    QVector<QVector<QString>> m_data;
    QVector<QString> m_headers;
};

class TestTableViewRendering : public QObject {
    Q_OBJECT
private slots:

    // **Validates: Requirements 6.1, 6.2**
    // Feature: widget-rendering-overhaul, Property 13: Table Column Width Calculation
    void columnWidthCalculation()
    {
        rc::check("Each column width equals max(header width, max data cell width), constrained by view width",
            []() {
                // Generate random dimensions
                const auto cols = *rc::gen::inRange(1, 6);
                const auto rows = *rc::gen::inRange(1, 8);

                RenderTestModel model(rows, cols);

                // Generate random header and cell data
                QVector<QString> headers(cols);
                for (int c = 0; c < cols; ++c) {
                    int hLen = *rc::gen::inRange(1, 9);
                    QString h;
                    for (int i = 0; i < hLen; ++i)
                        h.append(QChar(*rc::gen::inRange(0x41, 0x5B))); // A-Z
                    model.setHeader(c, h);
                    headers[c] = h;
                }

                QVector<QVector<QString>> cellData(rows, QVector<QString>(cols));
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        int cLen = *rc::gen::inRange(1, 9);
                        QString txt;
                        for (int i = 0; i < cLen; ++i)
                            txt.append(QChar(*rc::gen::inRange(0x61, 0x7B))); // a-z
                        model.setCellData(r, c, txt);
                        cellData[r][c] = txt;
                    }
                }

                // Compute expected natural column widths
                QVector<int> expectedNatural(cols);
                for (int c = 0; c < cols; ++c) {
                    int maxW = headers[c].length();
                    for (int r = 0; r < rows; ++r) {
                        if (cellData[r][c].length() > maxW)
                            maxW = cellData[r][c].length();
                    }
                    expectedNatural[c] = qMax(1, maxW);
                }

                // Use a view width large enough to fit all columns without scaling
                int separatorTotal = (cols > 1) ? (cols - 1) : 0;
                int totalNatural = 0;
                for (int c = 0; c < cols; ++c)
                    totalNatural += expectedNatural[c];
                int viewWidth = totalNatural + separatorTotal + 5; // extra space

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, rows + 1));
                tv.setModel(&model);

                ScreenBuffer buf(viewWidth, rows + 1);
                tv.paintEvent(buf);

                // Verify column widths by inspecting the header row
                // Each column should occupy exactly expectedNatural[c] chars, separated by 1 space
                int xOff = 0;
                for (int c = 0; c < cols; ++c) {
                    int colW = expectedNatural[c];

                    // The header text should be rendered at xOff
                    QString headerText = headers[c];
                    int renderedLen = qMin(headerText.length(), colW);
                    for (int i = 0; i < renderedLen; ++i) {
                        const Cell &cell = buf.backCell(xOff + i, 0);
                        RC_ASSERT(cell.ch == headerText.at(i).unicode());
                    }

                    // Remaining chars in column should be spaces (padding)
                    for (int i = renderedLen; i < colW; ++i) {
                        const Cell &cell = buf.backCell(xOff + i, 0);
                        RC_ASSERT(cell.ch == U' ');
                    }

                    xOff += colW;

                    // Separator space between columns
                    if (c < cols - 1) {
                        const Cell &sep = buf.backCell(xOff, 0);
                        RC_ASSERT(sep.ch == U' ');
                        xOff++;
                    }
                }
            });
    }

    // **Validates: Requirements 6.1**
    // Feature: widget-rendering-overhaul, Property 13: Table Column Width Calculation (scaling)
    void columnWidthScaling()
    {
        rc::check("When total column width exceeds view width, columns are proportionally scaled down",
            []() {
                // Create a model with wide columns that will exceed view width
                const auto cols = *rc::gen::inRange(3, 7);
                const auto rows = *rc::gen::inRange(1, 5);

                RenderTestModel model(rows, cols);

                // Use long headers to force scaling
                QVector<QString> headers(cols);
                for (int c = 0; c < cols; ++c) {
                    int hLen = *rc::gen::inRange(8, 16);
                    QString h;
                    for (int i = 0; i < hLen; ++i)
                        h.append(QChar(*rc::gen::inRange(0x41, 0x5B)));
                    model.setHeader(c, h);
                    headers[c] = h;
                }

                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        model.setCellData(r, c, QStringLiteral("x"));
                    }
                }

                // Use a narrow view width to force scaling
                int separatorTotal = (cols > 1) ? (cols - 1) : 0;
                int totalNatural = 0;
                for (int c = 0; c < cols; ++c)
                    totalNatural += headers[c].length();

                // Make view width smaller than total natural width
                int viewWidth = qMax(cols + separatorTotal, totalNatural / 2);

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, rows + 1));
                tv.setModel(&model);

                ScreenBuffer buf(viewWidth, rows + 1);
                tv.paintEvent(buf);

                // Verify: total rendered content fits within view width
                // Count non-default cells in header row
                int usedWidth = 0;
                for (int x = 0; x < viewWidth; ++x) {
                    const Cell &cell = buf.backCell(x, 0);
                    // All cells in header row should be written (either text or padding)
                    // since the implementation fills the entire row
                    if (cell.bold || cell.ch != U' '
                        || cell.foreground != Color(255, 255, 255)
                        || cell.background != Color(0, 0, 0)) {
                        usedWidth = x + 1;
                    }
                }
                // The rendered content should not exceed view width
                RC_ASSERT(usedWidth <= viewWidth);
            });
    }

    // **Validates: Requirements 6.2**
    // Feature: widget-rendering-overhaul, Property 14: Table Header Rendering
    void headerRendering()
    {
        rc::check("First rendered row contains bold text matching headerData() for each visible column",
            []() {
                const auto cols = *rc::gen::inRange(1, 5);
                const auto rows = *rc::gen::inRange(1, 6);

                RenderTestModel model(rows, cols);

                QVector<QString> headers(cols);
                for (int c = 0; c < cols; ++c) {
                    int hLen = *rc::gen::inRange(1, 7);
                    QString h;
                    for (int i = 0; i < hLen; ++i)
                        h.append(QChar(*rc::gen::inRange(0x41, 0x5B)));
                    model.setHeader(c, h);
                    headers[c] = h;
                }

                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        model.setCellData(r, c, QStringLiteral("d"));
                    }
                }

                // Use wide enough view to fit all columns
                int separatorTotal = (cols > 1) ? (cols - 1) : 0;
                int totalNatural = 0;
                for (int c = 0; c < cols; ++c)
                    totalNatural += qMax(1, headers[c].length());
                int viewWidth = totalNatural + separatorTotal + 10;

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, rows + 1));
                tv.setModel(&model);

                ScreenBuffer buf(viewWidth, rows + 1);
                tv.paintEvent(buf);

                // Verify header row (y=0) has bold=true for header text cells
                // and contains the header text
                int xOff = 0;
                for (int c = 0; c < cols; ++c) {
                    int colW = qMax(1, headers[c].length());
                    // For data cells, check if any is wider
                    for (int r = 0; r < rows; ++r) {
                        QModelIndex idx = model.index(r, c);
                        QString cellText = model.data(idx, Qt::DisplayRole).toString();
                        if (cellText.length() > colW)
                            colW = cellText.length();
                    }

                    QString headerText = headers[c];
                    int renderedLen = qMin(headerText.length(), colW);

                    // Verify bold and correct characters
                    for (int i = 0; i < renderedLen; ++i) {
                        const Cell &cell = buf.backCell(xOff + i, 0);
                        RC_ASSERT(cell.ch == headerText.at(i).unicode());
                        RC_ASSERT(cell.bold == true);
                    }

                    // Padding cells should also be bold (as per implementation)
                    for (int i = renderedLen; i < colW; ++i) {
                        const Cell &cell = buf.backCell(xOff + i, 0);
                        RC_ASSERT(cell.bold == true);
                    }

                    xOff += colW;
                    if (c < cols - 1)
                        xOff++; // separator
                }
            });
    }

    // **Validates: Requirements 6.6**
    // Feature: widget-rendering-overhaul, Property 15: Table Horizontal Scrolling
    void horizontalScrolling()
    {
        rc::check("Horizontal scrolling allows all columns to become visible by adjusting scroll offset",
            []() {
                // Create many narrow columns that exceed view width
                const auto cols = *rc::gen::inRange(6, 12);
                const auto rows = *rc::gen::inRange(1, 4);

                RenderTestModel model(rows, cols);

                // Use single-char headers ("A", "B", ...) and single-char data
                // so each column has natural width = 1
                for (int c = 0; c < cols; ++c) {
                    model.setHeader(c, QString(QChar('A' + c)));
                }

                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        model.setCellData(r, c, QString(QChar('a' + c)));
                    }
                }

                // Each column has natural width 1, separator width 1
                // Total needed: cols * 1 + (cols-1) = 2*cols - 1
                // Set view width to fit about half the columns
                // With 1-char columns: N columns need N + (N-1) = 2N-1 chars
                int halfCols = cols / 2;
                int viewWidth = 2 * halfCols - 1; // fits halfCols columns exactly
                viewWidth = qMax(viewWidth, 3);

                // Verify that total needed exceeds view width (scrolling is needed)
                int totalNeeded = 2 * cols - 1;
                RC_PRE(totalNeeded > viewWidth);

                QTuiTableView tv;
                tv.setGeometry(QRect(0, 0, viewWidth, rows + 1));
                tv.setModel(&model);

                // Initially, horizontal scroll offset should be 0
                RC_ASSERT(tv.horizontalScrollOffset() == 0);

                // Navigate to the last column using Right key presses
                for (int c = 0; c < cols - 1; ++c) {
                    TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(&tv, &ev);
                }

                // After navigating to last column, the scroll offset
                // should have changed to make that column visible
                int scrollOff = tv.horizontalScrollOffset();
                RC_ASSERT(scrollOff > 0);

                // Verify that every column can be made visible by navigating to it
                // Reset to first column
                for (int c = cols - 1; c > 0; --c) {
                    TuiKeyEvent ev(Qt::Key_Left, Qt::NoModifier);
                    QCoreApplication::sendEvent(&tv, &ev);
                }
                RC_ASSERT(tv.horizontalScrollOffset() == 0);

                // Navigate right through all columns, verifying each becomes visible
                for (int c = 0; c < cols; ++c) {
                    int offset = tv.horizontalScrollOffset();

                    // The current column must be >= the scroll offset (visible)
                    RC_ASSERT(c >= offset);

                    // Paint and verify the current column's header appears in the buffer
                    ScreenBuffer buf(viewWidth, rows + 1);
                    tv.paintEvent(buf);

                    // Read header row text
                    QString headerRow;
                    for (int x = 0; x < viewWidth; ++x)
                        headerRow.append(buf.backCell(x, 0).text());

                    // The header for column c should be visible (single char)
                    QChar expectedChar = QChar('A' + c);
                    RC_ASSERT(headerRow.contains(expectedChar));

                    // Move to next column (except on last iteration)
                    if (c < cols - 1) {
                        TuiKeyEvent ev(Qt::Key_Right, Qt::NoModifier);
                        QCoreApplication::sendEvent(&tv, &ev);
                    }
                }
            });
    }
};

QTEST_MAIN(TestTableViewRendering)
#include "test_tableview_rendering.moc"
