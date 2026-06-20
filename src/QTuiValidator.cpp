#include <QtTUI/QTuiValidator.h>
#include <climits>

namespace QtTUI {

QTuiValidator::QTuiValidator(QObject *parent)
    : QObject(parent)
{
}

void QTuiValidator::fixup(QString &) const
{
}

QTuiIntValidator::QTuiIntValidator(QObject *parent)
    : QTuiIntValidator(INT_MIN, INT_MAX, parent)
{
}

QTuiIntValidator::QTuiIntValidator(int bottom, int top, QObject *parent)
    : QTuiValidator(parent)
    , m_bottom(bottom)
    , m_top(top)
{
}

int QTuiIntValidator::bottom() const
{
    return m_bottom;
}

int QTuiIntValidator::top() const
{
    return m_top;
}

void QTuiIntValidator::setRange(int bottom, int top)
{
    m_bottom = bottom;
    m_top = top;
}

// Number of digits of the absolute value
static int digitCount(qint64 value)
{
    return QString::number(qAbs(value)).length();
}

QTuiValidator::State QTuiIntValidator::validate(QString &input, int &) const
{
    const QString text = input.trimmed();
    if (text.isEmpty())
        return Intermediate;
    if (text == QLatin1String("-") || text == QLatin1String("+"))
        return (text == QLatin1String("-") ? m_bottom < 0 : m_top > 0) ? Intermediate : Invalid;

    bool ok = false;
    const qint64 value = text.toLongLong(&ok);
    if (!ok)
        return Invalid;
    if (value >= m_bottom && value <= m_top)
        return Acceptable;
    if (value < 0 && m_bottom >= 0)
        return Invalid;
    if (value > 0 && m_top <= 0)
        return Invalid;

    // Out of range: more digits can still help only while the number is
    // shorter than the bound on its side
    const qint64 bound = value < 0 ? m_bottom : m_top;
    return digitCount(value) < digitCount(bound) ? Intermediate : Invalid;
}

QTuiRegularExpressionValidator::QTuiRegularExpressionValidator(QObject *parent)
    : QTuiValidator(parent)
{
}

QTuiRegularExpressionValidator::QTuiRegularExpressionValidator(const QRegularExpression &re,
                                                               QObject *parent)
    : QTuiValidator(parent)
    , m_re(re)
{
}

QRegularExpression QTuiRegularExpressionValidator::regularExpression() const
{
    return m_re;
}

void QTuiRegularExpressionValidator::setRegularExpression(const QRegularExpression &re)
{
    m_re = re;
}

QTuiValidator::State QTuiRegularExpressionValidator::validate(QString &input, int &) const
{
    // The whole input must match; a prefix of a match is Intermediate
    const QRegularExpression anchored(QRegularExpression::anchoredPattern(m_re.pattern()),
                                      m_re.patternOptions());
    const QRegularExpressionMatch match =
        anchored.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
    if (match.hasMatch())
        return Acceptable;
    if (match.hasPartialMatch() || input.isEmpty())
        return Intermediate;
    return Invalid;
}

} // namespace QtTUI
