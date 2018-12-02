#ifndef TUITYPES_H
#define TUITYPES_H

#include <cstdint>

namespace tui
{
  template<class T> constexpr const T& min(const T& a, const T& b) { return (b < a) ? b : a; }
  template<class T> constexpr const T& max(const T& a, const T& b) { return (a < b) ? b : a; }

  struct point2d_t
  {
    uint16_t x;
    uint16_t y;
  };

  struct size2d_t
  {
    uint16_t width;
    uint16_t height;

    constexpr bool isEmpty(void) const noexcept { return !width || !height; }
    constexpr bool isNull (void) const noexcept { return !width && !height; }
    constexpr bool isValid(void) const noexcept { return  width &&  height; }

    constexpr size2d_t boundedTo(const size2d_t& other) const noexcept
      { return size2d_t { tui::min(width, other.width), min(height, other.height) }; }

    constexpr size2d_t expandedTo(const size2d_t& other) const noexcept
      { return size2d_t { tui::max(width, other.width), max(height, other.height) }; }
  };

  struct rect_t
  {
    point2d_t pos;
    size2d_t size;

    constexpr uint16_t x     (void) const noexcept { return pos.x      ; }
    constexpr uint16_t y     (void) const noexcept { return pos.y      ; }
    constexpr uint16_t width (void) const noexcept { return size.width ; }
    constexpr uint16_t height(void) const noexcept { return size.height; }
  };

  struct margins_t
  {
    uint16_t left;
    uint16_t top;
    uint16_t right;
    uint16_t bottom;
  };

  enum Orientation : uint8_t
  {
    Horizontal = 0x01,
    Vertical   = 0x02,
  };

  enum Align : uint16_t
  {
    Left      = 0x0001, // Aligns with the left edge.
    Right     = 0x0002, // Aligns with the right edge.
    HCenter   = 0x0004, // Centers horizontally in the available space.
    Justify   = 0x0008, // Justifies the text in the available space.
    Absolute  = 0x0010, // If the widget's layout direction is RightToLeft (instead of LeftToRight, the default), Left refers to the right edge and Right to the left edge. This is normally the desired behavior. If you want Left to always mean "left" and Right to always mean "right", combine the flag with Absolute.
    Top       = 0x0020, // Aligns with the top.
    Bottom    = 0x0040, // Aligns with the bottom.
    VCenter   = 0x0080, // Centers vertically in the available space.
    Baseline  = 0x0100, // Aligns with the baseline.
    Center    = VCenter | HCenter, // Centers in both dimensions.

    HorizontalMask = Left | Right  | HCenter | Justify | Absolute,
    VerticalMask   = Top  | Bottom | VCenter | Baseline,
  };

  enum Control : uint16_t
  {
    Unspecified = 0x0001, // The default type, when none is specified.
    ButtonBox   = 0x0002, // A QDialogButtonBox instance.
    CheckBox    = 0x0004, // A QCheckBox instance.
    ComboBox    = 0x0008, // A QComboBox instance.
    Frame       = 0x0010, // A QFrame instance.
    GroupBox    = 0x0020, // A QGroupBox instance.
    Label       = 0x0040, // A QLabel instance.
    Line        = 0x0080, // A QFrame instance with QFrame::HLine or QFrame::VLine.
    LineEdit    = 0x0100, // A QLineEdit instance.
    PushButton  = 0x0200, // A QPushButton instance.
    RadioButton = 0x0400, // A QRadioButton instance.
    Slider      = 0x0800, // A QAbstractSlider instance.
    SpinBox     = 0x1000, // A QAbstractSpinBox instance.
    TabWidget   = 0x2000, // A QTabWidget instance.
    ToolButton  = 0x4000, // A QToolButton instance.
  };

  enum class SizeConstraint
  {
    Default = 0, // The main widget's minimum size is set to minimumSize(), unless the widget already has a minimum size.
    None      , // The widget is not constrained.
    Minimum   , // The main widget's minimum size is set to minimumSize(); it cannot be smaller.
    Fixed     , // The main widget's size is set to sizeHint(); it cannot be resized at all.
    Maximum   , // The main widget's maximum size is set to maximumSize(); it cannot be larger.
    MinAndMax , // The main widget's minimum size is set to minimumSize() and its maximum size is set to maximumSize().
  };

  enum FocusPolicy
  {
    NoFocus     = 0x00,
    TabFocus    = 0x01,
    ClickFocus  = 0x02,
    StrongFocus = 0x08 | TabFocus | ClickFocus,
    WheelFocus  = 0x04 | StrongFocus,
  };
}
#endif // TUITYPES_H
