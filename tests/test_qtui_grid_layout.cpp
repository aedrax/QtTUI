#include <QtTest/QtTest>
#include <rapidcheck.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiGridLayout.h>
#include <QtTUI/QTuiWidget.h>
#include <QtTUI/ScreenBuffer.h>

using namespace QtTUI;

class GridTestWidget : public QTuiWidget {
    Q_OBJECT
public:
    explicit GridTestWidget(QSize minSize = QSize(0, 0), QTuiWidget *parent = nullptr)
        : QTuiWidget(parent), m_minSize(minSize) {}
    void paintEvent(ScreenBuffer &) override {}
    QSize minimumSize() const override { return m_minSize; }
    void setMinSize(QSize s) { m_minSize = s; }
private:
    QSize m_minSize;
};

class TestQTuiGridLayout : public QObject {
    Q_OBJECT
private slots:

    // Req 30.1: inherits from QTuiLayout
    void inheritsFromQTuiLayout()
    {
        QTuiGridLayout grid;
        QTuiLayout *base = &grid;
        QVERIFY(base != nullptr);
    }

    // Req 30.2: addWidget updates row/column count
    void addWidgetUpdatesRowColumnCount()
    {
        QTuiGridLayout grid;
        QCOMPARE(grid.rowCount(), 0);
        QCOMPARE(grid.columnCount(), 0);

        auto *w1 = new GridTestWidget();
        grid.addWidget(w1, 0, 0);
        QCOMPARE(grid.rowCount(), 1);
        QCOMPARE(grid.columnCount(), 1);

        auto *w2 = new GridTestWidget();
        grid.addWidget(w2, 2, 3);
        QCOMPARE(grid.rowCount(), 3);
        QCOMPARE(grid.columnCount(), 4);

        delete w1;
        delete w2;
    }

    // Req 30.2, 30.6: 2x2 grid with equal stretch positions widgets correctly
    void applyPositionsWidgetsInGrid()
    {
        QTuiGridLayout grid;
        auto *w00 = new GridTestWidget();
        auto *w01 = new GridTestWidget();
        auto *w10 = new GridTestWidget();
        auto *w11 = new GridTestWidget();

        grid.addWidget(w00, 0, 0);
        grid.addWidget(w01, 0, 1);
        grid.addWidget(w10, 1, 0);
        grid.addWidget(w11, 1, 1);

        // No stretch set → equal distribution
        grid.apply(QRect(0, 0, 40, 20));

        // Each cell should be ~20x10
        QCOMPARE(w00->geometry().x(), 0);
        QCOMPARE(w00->geometry().y(), 0);
        QCOMPARE(w00->geometry().width(), 20);
        QCOMPARE(w00->geometry().height(), 10);

        QCOMPARE(w01->geometry().x(), 20);
        QCOMPARE(w01->geometry().y(), 0);
        QCOMPARE(w01->geometry().width(), 20);
        QCOMPARE(w01->geometry().height(), 10);

        QCOMPARE(w10->geometry().x(), 0);
        QCOMPARE(w10->geometry().y(), 10);
        QCOMPARE(w10->geometry().width(), 20);
        QCOMPARE(w10->geometry().height(), 10);

        QCOMPARE(w11->geometry().x(), 20);
        QCOMPARE(w11->geometry().y(), 10);
        QCOMPARE(w11->geometry().width(), 20);
        QCOMPARE(w11->geometry().height(), 10);

        delete w00; delete w01; delete w10; delete w11;
    }

    // Req 30.4: stretch factor distribution
    void applyWithStretchFactors()
    {
        QTuiGridLayout grid;
        auto *w0 = new GridTestWidget();
        auto *w1 = new GridTestWidget();

        grid.addWidget(w0, 0, 0);
        grid.addWidget(w1, 0, 1);
        grid.setColumnStretch(0, 1);
        grid.setColumnStretch(1, 3);

        grid.apply(QRect(0, 0, 40, 20));

        // col0 stretch=1, col1 stretch=3 → col0=10, col1=30
        QCOMPARE(w0->geometry().width(), 10);
        QCOMPARE(w1->geometry().width(), 30);
        QCOMPARE(w1->geometry().x(), 10);

        delete w0; delete w1;
    }

    // Req 30.5: minimum sizes are respected
    void applyWithMinimumSizes()
    {
        QTuiGridLayout grid;
        auto *w0 = new GridTestWidget();
        auto *w1 = new GridTestWidget();

        grid.addWidget(w0, 0, 0);
        grid.addWidget(w1, 0, 1);
        grid.setColumnMinimumWidth(0, 15);
        grid.setRowMinimumHeight(0, 8);

        grid.apply(QRect(0, 0, 40, 20));

        // Column 0 must be at least 15 wide
        QVERIFY(w0->geometry().width() >= 15);
        // Row 0 must be at least 8 tall
        QVERIFY(w0->geometry().height() >= 8);

        delete w0; delete w1;
    }

    // Req 30.7: widget spanning 2 columns gets combined width
    void applyWithCellSpanning()
    {
        QTuiGridLayout grid;
        auto *wTop = new GridTestWidget();    // spans 2 columns
        auto *wBL = new GridTestWidget();
        auto *wBR = new GridTestWidget();

        grid.addWidget(wTop, 0, 0, 1, 2);  // row 0, col 0, span 1 row, 2 cols
        grid.addWidget(wBL, 1, 0);
        grid.addWidget(wBR, 1, 1);

        grid.apply(QRect(0, 0, 40, 20));

        // Spanning widget should get the full width of both columns
        QCOMPARE(wTop->geometry().width(), wBL->geometry().width() + wBR->geometry().width());
        QCOMPARE(wTop->geometry().x(), 0);

        delete wTop; delete wBL; delete wBR;
    }

    // Req 30.7: widget spanning 2 rows gets combined height
    void applyWithRowSpanning()
    {
        QTuiGridLayout grid;
        auto *wLeft = new GridTestWidget();   // spans 2 rows
        auto *wTR = new GridTestWidget();
        auto *wBR = new GridTestWidget();

        grid.addWidget(wLeft, 0, 0, 2, 1);  // row 0, col 0, span 2 rows, 1 col
        grid.addWidget(wTR, 0, 1);
        grid.addWidget(wBR, 1, 1);

        grid.apply(QRect(0, 0, 40, 20));

        // Spanning widget should get the full height of both rows
        QCOMPARE(wLeft->geometry().height(), wTR->geometry().height() + wBR->geometry().height());
        QCOMPARE(wLeft->geometry().y(), 0);

        delete wLeft; delete wTR; delete wBR;
    }

    void applyEmptyLayoutDoesNotCrash()
    {
        QTuiGridLayout grid;
        grid.apply(QRect(0, 0, 40, 20));
        QCOMPARE(grid.rowCount(), 0);
        QCOMPARE(grid.columnCount(), 0);
    }

    void applyRespectsContentsMargins()
    {
        QTuiGridLayout grid;
        grid.setContentsMargins(2, 3, 4, 5);

        auto *w = new GridTestWidget();
        grid.addWidget(w, 0, 0);

        grid.apply(QRect(0, 0, 40, 20));

        // Widget should be offset by margins
        QCOMPARE(w->geometry().x(), 2);
        QCOMPARE(w->geometry().y(), 3);
        // Width = 40 - 2 - 4 = 34, Height = 20 - 3 - 5 = 12
        QCOMPARE(w->geometry().width(), 34);
        QCOMPARE(w->geometry().height(), 12);

        delete w;
    }

    // A nested layout added via addLayout must actually lay out its children
    // when the grid assigns it a cell (wrapper->setGeometry → nested apply).
    void addLayoutLaysOutNestedChildren()
    {
        QTuiGridLayout grid;
        auto *w0 = new GridTestWidget(QSize(5, 2));
        grid.addWidget(w0, 0, 0);

        auto *nested = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, &grid);
        auto *n1 = new GridTestWidget();
        auto *n2 = new GridTestWidget();
        nested->addWidget(n1, 1);
        nested->addWidget(n2, 1);
        grid.addLayout(nested, 0, 1);

        grid.apply(QRect(0, 0, 40, 20));

        // The nested layout's children fill column 1, stacked vertically
        const int col1x = w0->geometry().width();
        QVERIFY(col1x > 0);
        QCOMPARE(n1->geometry().x(), col1x);
        QCOMPARE(n2->geometry().x(), col1x);
        QCOMPARE(n1->geometry().width(), 40 - col1x);
        QCOMPARE(n2->geometry().width(), 40 - col1x);
        QCOMPARE(n1->geometry().y(), 0);
        QCOMPARE(n2->geometry().y(), n1->geometry().y() + n1->geometry().height());
        QCOMPARE(n1->geometry().height() + n2->geometry().height(), 20);

        delete w0;
        delete n1;
        delete n2;
    }

    // The nested layout's aggregated minimum size must constrain the grid's
    // column measurement, exactly like a widget's minimumSize does.
    void addLayoutMinimumSizeConstrainsColumn()
    {
        QTuiGridLayout grid;
        auto *w0 = new GridTestWidget(QSize(5, 2));
        grid.addWidget(w0, 0, 0);

        auto *nested = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom, &grid);
        auto *n1 = new GridTestWidget(QSize(12, 3));
        auto *n2 = new GridTestWidget(QSize(8, 2));
        nested->addWidget(n1, 0);
        nested->addWidget(n2, 0);
        grid.addLayout(nested, 0, 1);

        // Box aggregate: width = max(12, 8) = 12, height = 3 + 2 = 5
        QCOMPARE(nested->minimumSize(), QSize(12, 5));
        QCOMPARE(grid.minimumSize(), QSize(5 + 12, 5));

        // Tight area: column 1 still gets at least the nested minimum width
        grid.apply(QRect(0, 0, 17, 5));
        QCOMPARE(n1->geometry().width(), 12);

        delete w0;
        delete n1;
        delete n2;
    }

    // RapidCheck property: stretch factor distribution sums to available width
    void stretchFactorDistributionProperty()
    {
        rc::check("total allocated column widths equals available width",
            []() {
                // Generate 2-5 columns with random stretch factors 1-10
                const int cols = *rc::gen::inRange(2, 6);
                const int availWidth = *rc::gen::inRange(20, 200);

                QTuiGridLayout grid;
                QList<GridTestWidget *> widgets;
                QList<int> stretches;

                for (int c = 0; c < cols; ++c) {
                    auto *w = new GridTestWidget();
                    widgets.append(w);
                    grid.addWidget(w, 0, c);
                    int s = *rc::gen::inRange(1, 11);
                    stretches.append(s);
                    grid.setColumnStretch(c, s);
                }

                grid.apply(QRect(0, 0, availWidth, 10));

                // Total width of all columns should equal available width
                int totalWidth = 0;
                for (auto *w : widgets)
                    totalWidth += w->geometry().width();
                RC_ASSERT(totalWidth == availWidth);

                // Proportions should be within ±1 of expected
                int totalStretch = 0;
                for (int s : stretches)
                    totalStretch += s;

                for (int c = 0; c < cols; ++c) {
                    int expected = availWidth * stretches[c] / totalStretch;
                    int actual = widgets[c]->geometry().width();
                    RC_ASSERT(std::abs(actual - expected) <= 1);
                }

                qDeleteAll(widgets);
            });
    }
};

QTEST_MAIN(TestQTuiGridLayout)
#include "test_qtui_grid_layout.moc"
