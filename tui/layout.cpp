#include "layout.h"

namespace tui
{
  Layout::Layout(void) noexcept
    : m_margins({ 0, 0, 0, 0 }),
      m_menuBar(nullptr),
      m_enabled(true),
      m_sizeConstraint(SizeConstraint::Default),
      m_geometry({ {0, 0}, {0, 0} }),
      m_spacing(1)
  {
  }

  Widget* Layout::parentWidget(void) noexcept
  {
    LayoutItem* l = parent();

    while(l != nullptr &&
          l->widget() == nullptr)
      l = l->parent();
    if(l != nullptr)
      return l->widget();
    return nullptr;
  }

  rect_t Layout::geometry(void) const noexcept
  {
    rect_t geo = m_geometry;
    size2d_t szhint = sizeHint();
    //size2d_t contents = contentsRect().size;

    if(!geo.size.height)
      geo.size.height = szhint.height;
    if(!geo.size.width)
      geo.size.width = szhint.width;

    switch(sizeConstraint())
    {
      case SizeConstraint::Default  : // The main widget's minimum size is set to minimumSize(), unless the widget already has a minimum size.
        geo.size.expandedTo(minimumSize());
        break;
      case SizeConstraint::None     : // The widget is not constrained.
        break;
      case SizeConstraint::Minimum  : // The main widget's minimum size is set to minimumSize(); it cannot be smaller.
        geo.size.expandedTo(minimumSize());
        break;
      case SizeConstraint::Fixed    : // The main widget's size is set to sizeHint(); it cannot be resized at all.
        geo.size = szhint;
        break;
      case SizeConstraint::Maximum  : // The main widget's maximum size is set to maximumSize(); it cannot be larger.
        geo.size.boundedTo(maximumSize());
        break;
      case SizeConstraint::MinAndMax: // The main widget's minimum size is set to minimumSize() and its maximum size is set to maximumSize().
        geo.size.expandedTo(minimumSize());
        geo.size.boundedTo(maximumSize());
        break;
    }
    return geo;
  }

}
