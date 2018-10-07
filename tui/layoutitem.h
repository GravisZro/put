#ifndef LAYOUTITEM_H
#define LAYOUTITEM_H

#include "tuitypes.h"

namespace tui
{
  class Layout;
  class Widget;
  class SpacerItem;
  class SizePolicy;

  class LayoutItem
  {
  public:
    LayoutItem(Align alignment = Align::Left) noexcept : m_parent(nullptr), m_alignment(alignment) { }
    virtual ~LayoutItem(void) noexcept = default;

    LayoutItem* parent      (void) const noexcept { return m_parent; }
    void        setParent   (LayoutItem* parent) noexcept { m_parent = parent; }

    Align       alignment   (void) const noexcept { return m_alignment; }
    void        setAlignment(Align alignment) noexcept { m_alignment = alignment; }

//    virtual Control   controlTypes(void) const noexcept { return Control::Unspecified; }

    virtual rect_t    geometry    (void) const noexcept = 0;
    virtual bool      isEmpty     (void) const noexcept = 0;
    virtual size2d_t  maximumSize (void) const noexcept = 0;
    virtual size2d_t  minimumSize (void) const noexcept = 0;
    virtual void      setGeometry (const rect_t& r) noexcept = 0;
    virtual size2d_t  sizeHint    (void) const noexcept = 0;
//    virtual Orientation expandingDirections(void) const noexcept = 0;


    virtual SpacerItem* spacerItem(void) noexcept { return nullptr; }
    virtual Widget*     widget    (void) noexcept { return nullptr; }
    virtual Layout*     layout    (void) noexcept { return nullptr; }
  private:
    LayoutItem* m_parent;
    Align m_alignment;
  };



  size2d_t calculateMinSize(const size2d_t& sizeHint,
                            const size2d_t& minSizeHint,
                            const size2d_t& minSize,
                            const size2d_t& maxSize,
                            const SizePolicy& sizePolicy);

  size2d_t calculateMaxSize(const size2d_t& sizeHint,
                            const size2d_t& minSize,
                            const size2d_t& maxSize,
                            const SizePolicy& sizePolicy,
                            Align alignment);
}

#endif // LAYOUTITEM_H
