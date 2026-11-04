#include <QtTUI/QTuiColorDialog.h>
#include <QtTUI/QTuiBoxLayout.h>
#include <QtTUI/QTuiLabel.h>
#include <QtTUI/QTuiLineEdit.h>
#include <QtTUI/QTuiPushButton.h>
#include <QtTUI/QTuiSlider.h>
#include <QtTUI/QTuiValidator.h>
#include <QtTUI/RenderUtils.h>
#include <QtTUI/TuiEvents.h>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace QtTUI {

namespace {

constexpr int kColumns = 8;
constexpr int kCellWidth = 3;

// Rows 1-2: the 16 basic colors. Rows 3-6: eight hues, from light to dark.
constexpr quint32 kPalette[] = {
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xC0C0C0,
    0x808080, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
    0xFCA5A5, 0xFDBA74, 0xFDE047, 0x86EFAC, 0x5EEAD4, 0x93C5FD, 0xC4B5FD, 0xF9A8D4,
    0xEF4444, 0xF97316, 0xEAB308, 0x22C55E, 0x14B8A6, 0x3B82F6, 0x8B5CF6, 0xEC4899,
    0xB91C1C, 0xC2410C, 0xA16207, 0x15803D, 0x0F766E, 0x1D4ED8, 0x6D28D9, 0xBE185D,
    0x7F1D1D, 0x7C2D12, 0x713F12, 0x14532D, 0x134E4A, 0x1E3A8A, 0x4C1D95, 0x831843,
};
constexpr int kCount = int(sizeof(kPalette) / sizeof(kPalette[0]));
constexpr int kRows = kCount / kColumns;

Color fromRgb(quint32 rgb)
{
    return Color(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb));
}

QString hexText(const Color &c)
{
    return QStringLiteral("#%1").arg((c.r << 16) | (c.g << 8) | c.b, 6, 16, QLatin1Char('0')).toUpper();
}

// Black or white, the one that is easier to read on c
Color contrastColor(const Color &c)
{
    return (299 * c.r + 587 * c.g + 114 * c.b) / 1000 > 127 ? Color(0, 0, 0) : Color(255, 255, 255);
}

// The grid of preset colors. Each cell shows its color as the background.
// The grid has no color of its own: it reads and sets the current color
// of the dialog.
class ColorGrid : public QTuiWidget {
public:
    explicit ColorGrid(QTuiColorDialog *dialog, QTuiWidget *parent)
        : QTuiWidget(parent)
        , m_dialog(dialog)
    {
        setFocusPolicy(Qt::StrongFocus);
    }

    QSize minimumSize() const override { return QSize(kColumns * kCellWidth, kRows); }

    void paintEvent(ScreenBuffer &buffer) override
    {
        const QRect geo = geometry();
        clearBackground(buffer, geo);
        const int current = cursor();
        for (int i = 0; i < kCount; ++i) {
            const Color color = fromRgb(kPalette[i]);
            const int x = geo.x() + (i % kColumns) * kCellWidth;
            const int y = geo.y() + i / kColumns;
            Cell style = cellStyle(contrastColor(color), color);
            fillRect(buffer, QRect(x, y, kCellWidth, 1), style);
            if (i != current)
                continue;
            // With focus, brackets mark the cell that the arrows move
            // from; without focus, a dot marks the current color
            if (hasFocus()) {
                style.bold = true;
                renderText(buffer, x, y, QStringLiteral("[ ]"), kCellWidth, style);
            } else if (color == m_dialog->currentColor()) {
                renderText(buffer, x, y, QStringLiteral(" • "), kCellWidth, style);
            }
        }
    }

protected:
    void keyPressEvent(TuiKeyEvent *event) override
    {
        int col = cursor() % kColumns;
        int row = cursor() / kColumns;
        switch (vimArrowKey(event)) {
        case Qt::Key_Left:  col = qMax(col - 1, 0); break;
        case Qt::Key_Right: col = qMin(col + 1, kColumns - 1); break;
        case Qt::Key_Up:    row = qMax(row - 1, 0); break;
        case Qt::Key_Down:  row = qMin(row + 1, kRows - 1); break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            m_dialog->accept();
            return;
        default:
            return;
        }
        select(row * kColumns + col);
    }

    void mousePressEvent(TuiMouseEvent *event) override
    {
        if (event->mouseType() != TuiMouseEvent::Press || event->button() != Qt::LeftButton)
            return;
        const int col = (event->x() - geometry().x()) / kCellWidth;
        const int row = event->y() - geometry().y();
        if (col >= 0 && col < kColumns && row >= 0 && row < kRows)
            select(row * kColumns + col);
    }

private:
    // The cell of the current color. If the current color is not a preset,
    // this is the last cell that the user selected.
    int cursor() const
    {
        const Color current = m_dialog->currentColor();
        for (int i = 0; i < kCount; ++i) {
            if (fromRgb(kPalette[i]) == current)
                return i;
        }
        return m_last;
    }

    void select(int index)
    {
        m_last = index;
        m_dialog->setCurrentColor(fromRgb(kPalette[index]));
        markDirty();
    }

    QTuiColorDialog *m_dialog;
    int m_last = 0;
};

// The preview: an area with the current color of the dialog
class ColorSwatch : public QTuiWidget {
public:
    explicit ColorSwatch(QTuiColorDialog *dialog, QTuiWidget *parent)
        : QTuiWidget(parent)
        , m_dialog(dialog)
    {
    }

    QSize minimumSize() const override { return QSize(14, 3); }

    void paintEvent(ScreenBuffer &buffer) override
    {
        clearBackground(buffer, geometry());
        const Color color = m_dialog->currentColor();
        fillRect(buffer, geometry(), cellStyle(contrastColor(color), color));
    }

private:
    QTuiColorDialog *m_dialog;
};

// A slider for one of red, green and blue. Page Up and Page Down step by
// 16, because 255 single steps are too many for the keyboard. Enter
// accepts the dialog.
class ChannelSlider : public QTuiSlider {
public:
    explicit ChannelSlider(QTuiColorDialog *dialog, QTuiWidget *parent)
        : QTuiSlider(Qt::Horizontal, parent)
        , m_dialog(dialog)
    {
        setRange(0, 255);
    }

protected:
    void keyPressEvent(TuiKeyEvent *event) override
    {
        switch (event->key()) {
        case Qt::Key_PageUp:   setValue(value() + 16); break;
        case Qt::Key_PageDown: setValue(value() - 16); break;
        case Qt::Key_Return:
        case Qt::Key_Enter:    m_dialog->accept(); break;
        default:               QTuiSlider::keyPressEvent(event); break;
        }
    }

private:
    QTuiColorDialog *m_dialog;
};

} // namespace

QTuiColorDialog::QTuiColorDialog(QTuiWidget *parent)
    : QTuiColorDialog(Color(), parent)
{
}

QTuiColorDialog::QTuiColorDialog(const Color &initial, QTuiWidget *parent)
    : QTuiDialog(parent)
    , m_current(initial)
{
    setWindowTitle(tr("Select Color"));

    auto *column = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);

    // Top row: the grid on the left; the preview and the hex text on the right
    auto *top = new QTuiWidget(this);
    auto *topRow = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    topRow->setSpacing(2);
    m_grid = new ColorGrid(this, top);
    topRow->addWidget(m_grid);
    auto *side = new QTuiWidget(top);
    auto *sideColumn = new QTuiBoxLayout(QTuiBoxLayout::TopToBottom);
    m_swatch = new ColorSwatch(this, side);
    sideColumn->addWidget(m_swatch, 1);
    sideColumn->addSpacing(1);
    m_hexEdit = new QTuiLineEdit(side);
    m_hexEdit->setValidator(new QTuiRegularExpressionValidator(
        QRegularExpression(QStringLiteral("#[0-9A-Fa-f]{6}")), m_hexEdit));
    sideColumn->addWidget(m_hexEdit);
    side->setLayout(sideColumn);
    topRow->addWidget(side);
    top->setLayout(topRow);
    column->addWidget(top);
    column->addSpacing(1);

    // One row for each of red, green and blue: "R <slider> 255"
    const QString names[3] = { tr("R"), tr("G"), tr("B") };
    for (int i = 0; i < 3; ++i) {
        auto *channel = new QTuiWidget(this);
        auto *row = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
        row->setSpacing(1);
        row->addWidget(new QTuiLabel(names[i], channel));
        m_sliders[i] = new ChannelSlider(this, channel);
        row->addWidget(m_sliders[i], 1);
        m_valueLabels[i] = new QTuiLabel(QString(), channel);
        row->addWidget(m_valueLabels[i]);
        channel->setLayout(row);
        column->addWidget(channel);
        connect(m_sliders[i], &QTuiSlider::valueChanged, this, [this]() {
            setCurrentColor(Color(uint8_t(m_sliders[0]->value()), uint8_t(m_sliders[1]->value()),
                                  uint8_t(m_sliders[2]->value())));
        });
    }
    column->addSpacing(1);

    auto *buttons = new QTuiWidget(this);
    auto *buttonRow = new QTuiBoxLayout(QTuiBoxLayout::LeftToRight);
    buttonRow->setSpacing(1);
    buttonRow->addStretch();
    auto *ok = new QTuiPushButton(tr("OK"), buttons);
    auto *cancel = new QTuiPushButton(tr("Cancel"), buttons);
    buttonRow->addWidget(ok);
    buttonRow->addWidget(cancel);
    buttons->setLayout(buttonRow);
    column->addWidget(buttons);
    setLayout(column);

    // A complete "#RRGGBB" text sets the color; other text waits
    connect(m_hexEdit, &QTuiLineEdit::textChanged, this, [this](const QString &text) {
        if (m_hexEdit->hasAcceptableInput())
            setCurrentColor(fromRgb(text.mid(1).toUInt(nullptr, 16)));
    });
    connect(m_hexEdit, &QTuiLineEdit::returnPressed, this, &QTuiDialog::accept);
    connect(ok, &QTuiPushButton::clicked, this, &QTuiDialog::accept);
    connect(cancel, &QTuiPushButton::clicked, this, &QTuiDialog::reject);
    connect(this, &QTuiDialog::accepted, this, [this]() {
        m_selected = m_current;
        emit colorSelected(m_selected);
    });

    syncParts();
}

Color QTuiColorDialog::currentColor() const
{
    return m_current;
}

void QTuiColorDialog::setCurrentColor(const Color &color)
{
    if (m_current == color)
        return;
    m_current = color;
    syncParts();
    emit currentColorChanged(m_current);
}

Color QTuiColorDialog::selectedColor() const
{
    return m_selected;
}

void QTuiColorDialog::syncParts()
{
    const int values[3] = { m_current.r, m_current.g, m_current.b };
    for (int i = 0; i < 3; ++i) {
        // No signal: a slider must not set a color from two old values
        const QSignalBlocker blocker(m_sliders[i]);
        m_sliders[i]->setValue(values[i]);
        // A constant width keeps the slider at one size
        m_valueLabels[i]->setText(QString::number(values[i]).rightJustified(3));
    }
    // Do not replace text that already gives this color: the cursor of the
    // line edit stays where the user types
    const QString hex = hexText(m_current);
    if (m_hexEdit->text().compare(hex, Qt::CaseInsensitive) != 0)
        m_hexEdit->setText(hex);
    m_grid->markDirty();
    m_swatch->markDirty();
}

Color QTuiColorDialog::getColor(const Color &initial, QTuiWidget *parent, const QString &title,
                                bool *ok)
{
    QTuiColorDialog dialog(initial, parent);
    if (!title.isEmpty())
        dialog.setWindowTitle(title);
    const bool accepted = dialog.exec() == Accepted;
    if (ok)
        *ok = accepted;
    return accepted ? dialog.selectedColor() : initial;
}

} // namespace QtTUI
