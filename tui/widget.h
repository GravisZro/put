#ifndef WIDGET_H
#define WIDGET_H

#include <object.h>

#include "tuiutils.h"
#include "layoutitem.h"
#include "layout.h"
#include "sizepolicy.h"
#include "event.h"

#include <cassert>

namespace tui
{
  class Widget : public LayoutItem,
                 public Object
  {
  public:
    Widget(void) noexcept;

// === Get and Set ===

    // simple get/set functions
    virtual Widget* widget(void) noexcept override { return this; }

    Layout* layout(void) const noexcept { return m_layout; }
    void    setLayout(Layout* layout) noexcept { m_layout = layout; }

    SizePolicy sizePolicy(void) const noexcept { return m_sizePolicy; }
    void setSizePolicy(SizePolicy policy) noexcept { m_sizePolicy = policy; }

    virtual rect_t    geometry(void) const noexcept override { return m_geometry; }
    virtual size2d_t  size    (void) const noexcept          { return m_geometry.size; }
    void setMinimumSize(const size2d_t& minsz) noexcept { m_minimumSize = minsz; }
    void setMaximumSize(const size2d_t& maxsz) noexcept { m_maximumSize = maxsz; }


    // complex get/set functions
    virtual void      setGeometry (const rect_t& r) noexcept override;
    virtual size2d_t  minimumSize (void) const noexcept override;
    virtual size2d_t  maximumSize (void) const noexcept override;
    virtual size2d_t  sizeHint       (void) const noexcept override;
            size2d_t  minimumSizeHint(void) const noexcept;

  protected:

// === Events ===
    virtual bool event(Event* event);


//            bool             focusNextChild()
//            bool             focusPreviousChild()
//    virtual bool             focusNextPrevChild(bool next);

    // Focus events
    virtual void             focusInEvent     (FocusEvent *event);
    virtual void             focusOutEvent    (FocusEvent *event);

    // Keyboard events
    virtual void             keyPressEvent    (KeyEvent *event);
    virtual void             keyReleaseEvent  (KeyEvent *event);

    // Mouse events
    // Mouse: drag and drop
    virtual void             dragEnterEvent   (DragEnterEvent *event);
    virtual void             dragLeaveEvent   (DragLeaveEvent *event);
    virtual void             dragMoveEvent    (DragMoveEvent *event);
    virtual void             dropEvent        (DropEvent *event);

    // Mouse: position
    virtual void             enterEvent       (Event *event);
    virtual void             leaveEvent       (Event *event);

    // Mouse: buttons
    virtual void             mouseDoubleClickEvent(MouseEvent *event);
    virtual void             mouseMoveEvent       (MouseEvent *event);
    virtual void             mousePressEvent      (MouseEvent *event);
    virtual void             mouseReleaseEvent    (MouseEvent *event);
    virtual void             wheelEvent           (WheelEvent *event);

    // Paint/Layout events
    virtual void             hideEvent        (HideEvent *event);
    virtual void             moveEvent        (MoveEvent *event);
    virtual void             paintEvent       (PaintEvent *event);
    virtual void             resizeEvent      (ResizeEvent *event);
    virtual void             showEvent        (ShowEvent *event);

    // Misc. events
    virtual void             actionEvent      (ActionEvent *event);
    virtual void             changeEvent      (Event *event);
    virtual void             closeEvent       (CloseEvent *event);
    virtual void             contextMenuEvent (ContextMenuEvent *event);

  private:
    virtual Layout* layout(void) noexcept override { assert(false); return nullptr; }
    virtual bool isEmpty (void) const noexcept override { return layout() == nullptr; }

    rect_t      m_geometry;
    Layout*     m_layout;
    SizePolicy  m_sizePolicy;
    size2d_t    m_minimumSize;
    size2d_t    m_maximumSize;
    size2d_t    m_sizeHint;
  };
}

#endif // WIDGET_H
