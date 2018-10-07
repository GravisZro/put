#ifndef LAYOUT_H
#define LAYOUT_H

#include <object.h>

#include "layoutitem.h"
#include "sizepolicy.h"

namespace tui
{
  class Layout : public LayoutItem
  {
  public:
    Layout(void) noexcept;
    virtual Layout* layout(void) noexcept override { return this; }

    Widget* parentWidget(void) noexcept;




    virtual void    addItem(LayoutItem *item) = 0;
    virtual void    removeItem(LayoutItem *item) = 0;

    virtual rect_t  contentsRect(void) const = 0;
    virtual int     count(void) const = 0;

    //virtual int indexOf(LayoutItem *widget) const = 0;
    //virtual LayoutItem* itemAt(int index) const = 0;
    //virtual LayoutItem* takeAt(int index) = 0;


    margins_t       contentsMargins(void) const noexcept { return m_margins; }
    void            setContentsMargins(const margins_t &margins) noexcept { m_margins = margins; }

    LayoutItem*     menuBar(void) const noexcept { return m_menuBar; }
    void            setMenuBar(LayoutItem* menuBar) noexcept { m_menuBar = menuBar; }


    bool            isEnabled(void) const noexcept { return m_enabled; }
    void            setEnabled(bool enable) noexcept { m_enabled = enable; }

    SizeConstraint  sizeConstraint(void) const noexcept { return m_sizeConstraint; }
    void            setSizeConstraint(SizeConstraint sc) noexcept { m_sizeConstraint = sc; }

    virtual int     spacing(void) const noexcept { return m_spacing; }
    virtual void    setSpacing(int spacing) noexcept { m_spacing = spacing; }

    virtual rect_t  geometry(void) const noexcept override;// { return m_geometry; }
    virtual void    setGeometry(const rect_t& geometry) noexcept override { m_geometry = geometry; }


//    void            update(void) noexcept;


    //virtual Control controlTypes() const noexcept override;
    //virtual Qt::Orientations     expandingDirections() const noexcept override;
//    virtual void     invalidate() noexcept override;

    virtual bool      isEmpty(void) const noexcept override { return count() > 0; }
    virtual size2d_t  maximumSize(void) const noexcept override { return size2d_t{ UINT16_MAX, UINT16_MAX }; }
    virtual size2d_t  minimumSize(void) const noexcept override { return size2d_t{ 0, 0 }; }

  private:
    margins_t   m_margins;
    LayoutItem* m_menuBar;
    bool m_enabled;
    SizeConstraint m_sizeConstraint;
    rect_t m_geometry;
    int m_spacing;
  };
}

#endif // LAYOUT_H
