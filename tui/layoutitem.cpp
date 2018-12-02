#include "layoutitem.h"

#include "sizepolicy.h"


namespace tui
{

  size2d_t calculateMinSize(const size2d_t& sizeHint,
                            const size2d_t& minSizeHint,
                            const size2d_t& minSize,
                            const size2d_t& maxSize,
                            const SizePolicy& sizePolicy)
  {
      size2d_t s { 0, 0 };

      if (sizePolicy.horizontalPolicy() != SizePolicy::Policy::Ignored)
      {
        if(sizePolicy.horizontalPolicy() & SizePolicy::Flags::Shrink)
          s.width = minSizeHint.width;
        else
          s.width = tui::max(sizeHint.width, minSizeHint.width);
      }

      if (sizePolicy.verticalPolicy() != SizePolicy::Policy::Ignored)
      {
          if (sizePolicy.verticalPolicy() & SizePolicy::Flags::Shrink)
            s.height = minSizeHint.height;
          else
            s.height = tui::max(sizeHint.height, minSizeHint.height);
      }

      s = s.boundedTo(maxSize);
      if (minSize.width)
          s.width = minSize.width;
      if (minSize.height)
          s.height = minSize.height;

      return s.expandedTo(size2d_t { 0, 0 });
  }

  size2d_t calculateMaxSize(const size2d_t& sizeHint,
                            const size2d_t& minSize,
                            const size2d_t& maxSize,
                            const SizePolicy& sizePolicy,
                            Align alignment)
  {
    if(alignment & Align::HorizontalMask &&
       alignment & Align::VerticalMask)
      return size2d_t { UINT16_MAX, UINT16_MAX };

    size2d_t s = maxSize;
    size2d_t hint = sizeHint.expandedTo(minSize);
    if (s.width == UINT16_MAX && !(alignment & Align::HorizontalMask))
      if (!(sizePolicy.horizontalPolicy() & SizePolicy::Flags::Grow))
        s.width = hint.width;

    if(s.height == UINT16_MAX && !(alignment & Align::VerticalMask))
      if (!(sizePolicy.verticalPolicy() & SizePolicy::Flags::Grow))
        s.height = hint.height;

    if(alignment & Align::HorizontalMask)
      s.width = UINT16_MAX;
    if(alignment & Align::VerticalMask)
      s.height = UINT16_MAX;
    return s;
  }

/*
  size2d_t qSmartMinSize(const LayoutItem *w)
  {
    return qSmartMinSize(w->sizeHint(),
                         w->minimumSizeHint(),
                         w->minimumSize(),
                         w->maximumSize(),
                         w->sizePolicy());
  }

  size2d_t qSmartMaxSize(const LayoutItem* w, Align alignment)
  {
    return qSmartMaxSize(w->sizeHint().expandedTo(w->minimumSizeHint()),
                         w->minimumSize(),
                         w->maximumSize(),
                         w->sizePolicy(),
                         alignment);
  }
*/
  /*
  int qSmartSpacing(const QLayout *layout, QStyle::PixelMetric pm)
  {
      QObject *parent = layout->parent();
      if (!parent) {
          return -1;
      } else if (parent->isWidgetType()) {
          Widget *pw = static_cast<Widget *>(parent);
          return pw->style()->pixelMetric(pm, 0, pw);
      } else {
          return static_cast<QLayout *>(parent)->spacing();
      }
  }
  */

}
