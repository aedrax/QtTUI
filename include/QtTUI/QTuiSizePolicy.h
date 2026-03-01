#ifndef QTTUI_QTUISIZEPOLICY_H
#define QTTUI_QTUISIZEPOLICY_H

#include <QtTUI/QtTUIGlobal.h>

namespace QtTUI {

class QTuiSizePolicy {
public:
    enum Policy {
        Fixed = 0,
        Minimum = 1,
        Maximum = 2,
        Preferred = 3,
        Expanding = 4,
        MinimumExpanding = 5,
        Ignored = 6
    };

    QTuiSizePolicy() = default;
    QTuiSizePolicy(Policy horizontal, Policy vertical);

    Policy horizontalPolicy() const;
    void setHorizontalPolicy(Policy policy);

    Policy verticalPolicy() const;
    void setVerticalPolicy(Policy policy);

    int horizontalStretch() const;
    void setHorizontalStretch(int stretch);

    int verticalStretch() const;
    void setVerticalStretch(int stretch);

private:
    Policy m_horizontal = Preferred;
    Policy m_vertical = Preferred;
    int m_hStretch = 0;
    int m_vStretch = 0;
};

} // namespace QtTUI

#endif // QTTUI_QTUISIZEPOLICY_H
