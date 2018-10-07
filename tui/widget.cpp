#include "widget.h"

namespace tui
{
  Widget::Widget(void) noexcept
    : m_geometry({{ 0, 0 }, { 0, 0 }}),
      m_layout(nullptr),
      m_sizePolicy(SizePolicy::Preferred, SizePolicy::Preferred),
      m_minimumSize({ 0, 0 }),
      m_maximumSize({ 0, 0 }),
      m_sizeHint({ 0, 0 })
  {
  }

  size2d_t Widget::minimumSize(void) const noexcept
  {
    if (isEmpty())
      return size2d_t { 0, 0 };
    return calculateMinSize(m_sizeHint, minimumSizeHint(), m_minimumSize, m_maximumSize, sizePolicy());
  }

  size2d_t Widget::maximumSize(void) const noexcept
  {
    if (isEmpty())
      return size2d_t { 0, 0 };
    return calculateMaxSize(m_sizeHint, m_minimumSize, m_maximumSize, sizePolicy(), alignment());
  }

  size2d_t Widget::minimumSizeHint(void) const noexcept
  {
    if (isEmpty())
      return size2d_t { 0, 0 };
    return layout()->minimumSize();
  }

  size2d_t Widget::sizeHint(void) const noexcept
  {
    if (isEmpty())
      return size2d_t { 0, 0 };

    size2d_t sz = m_sizeHint
                  .expandedTo(minimumSizeHint())
                  .boundedTo(m_maximumSize)
                  .expandedTo(m_minimumSize);

    if (sizePolicy().horizontalPolicy() == SizePolicy::Policy::Ignored)
      sz.width = 0;
    if (sizePolicy().verticalPolicy() == SizePolicy::Policy::Ignored)
      sz.height = 0;
    return sz;
  }

  void Widget::setGeometry(const rect_t& rect) noexcept
  {
    if (isEmpty())
      return;

    size2d_t& sz = m_geometry.size;
    uint16_t& x = m_geometry.pos.x;
    uint16_t& y = m_geometry.pos.y;

    sz = rect.size.boundedTo(maximumSize());
    x = rect.x();
    y = rect.y();

    if (alignment() & (Align::HorizontalMask | Align::VerticalMask))
    {
      size2d_t pref(sizeHint());
      SizePolicy sp = sizePolicy();

      if (sp.horizontalPolicy() == SizePolicy::Policy::Ignored)
        pref.width = m_sizeHint.expandedTo(m_minimumSize).width;
      if (sp.verticalPolicy() == SizePolicy::Policy::Ignored)
        pref.height = m_sizeHint.expandedTo(m_minimumSize).height;

      if (alignment() & Align::HorizontalMask)
        sz.width  = tui::min(sz.width , pref.width);
      if (alignment() & Align::VerticalMask)
        sz.height = tui::min(sz.height, pref.height);
    }

    Align alignHoriz = alignment();// QStyle::visualAlignment(wid->layoutDirection(), alignment());
    if (alignHoriz & Align::Right) // right
      x += rect.width() - sz.width;
    else if (!(alignHoriz & Align::Left)) // left
      x += (rect.width() - sz.width) / 2;

    if (alignment() & Align::Bottom) // bottom
      y += rect.height() - sz.height;
    else if (!(alignment() & Align::Top)) // middle
      y += (rect.height() - sz.height) / 2;

    //      wid->setGeometry(x, y, s.width, s.height);
  }
}
