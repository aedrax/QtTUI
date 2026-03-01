#include <QtTUI/QTuiSizePolicy.h>
#include <algorithm>

namespace QtTUI {

QTuiSizePolicy::QTuiSizePolicy(Policy horizontal, Policy vertical)
    : m_horizontal(horizontal)
    , m_vertical(vertical)
{
}

QTuiSizePolicy::Policy QTuiSizePolicy::horizontalPolicy() const
{
    return m_horizontal;
}

void QTuiSizePolicy::setHorizontalPolicy(Policy policy)
{
    m_horizontal = policy;
}

QTuiSizePolicy::Policy QTuiSizePolicy::verticalPolicy() const
{
    return m_vertical;
}

void QTuiSizePolicy::setVerticalPolicy(Policy policy)
{
    m_vertical = policy;
}

int QTuiSizePolicy::horizontalStretch() const
{
    return m_hStretch;
}

void QTuiSizePolicy::setHorizontalStretch(int stretch)
{
    m_hStretch = std::clamp(stretch, 0, 255);
}

int QTuiSizePolicy::verticalStretch() const
{
    return m_vStretch;
}

void QTuiSizePolicy::setVerticalStretch(int stretch)
{
    m_vStretch = std::clamp(stretch, 0, 255);
}

} // namespace QtTUI
