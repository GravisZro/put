#ifndef SIZEPOLICY_H
#define SIZEPOLICY_H

#include "tuitypes.h"

namespace tui
{
  class SizePolicy
  {
  public:
    enum Flags : uint8_t
    {
      Grow    = 0x01,
      Shrink  = 0x02,
      Expand  = 0x04,
      Ignore  = 0x08,
    };

    enum Policy : uint8_t
    {
      Fixed             = 0,
      Minimum           = Flags::Grow,
      Maximum           = Flags::Shrink,
      Preferred         = Flags::Grow | Flags::Shrink,
      Expanding         = Flags::Grow | Flags::Shrink | Flags::Expand,
      MinimumExpanding  = Flags::Grow | Flags::Expand,
      Ignored           = Flags::Grow | Flags::Shrink | Flags::Ignore,
    };

    SizePolicy(Policy horizontal, Policy vertical, Control type = Control::Unspecified)
      : m_horizontal_policy(horizontal),
        m_horizontal_strech(1),
        m_vertical_policy(vertical),
        m_vertical_strech(1),
        m_control(type) { }
    SizePolicy(void) : SizePolicy(Preferred, Preferred) { }

    Policy      horizontalPolicy() const noexcept { return m_horizontal_policy; }
    void        setHorizontalPolicy(Policy policy) noexcept { m_horizontal_policy = policy; }

    int         horizontalStretch() const noexcept { return m_horizontal_strech; }
    void        setHorizontalStretch(int stretchFactor) { m_horizontal_strech = stretchFactor; }

    Policy      verticalPolicy() const noexcept { return m_vertical_policy; }
    void        setVerticalPolicy(Policy policy) { m_vertical_policy = policy; }

    int         verticalStretch() const noexcept { return m_vertical_strech; }
    void        setVerticalStretch(int stretchFactor) { m_vertical_strech = stretchFactor; }

    Control     controlType(void) const noexcept { return m_control; }
    void        setControlType(Control type) { m_control = type; }

  private:
    Policy  m_horizontal_policy;
    int     m_horizontal_strech;
    Policy  m_vertical_policy;
    int     m_vertical_strech;
    Control m_control;
  };
}

#endif // SIZEPOLICY_H
