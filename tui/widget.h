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

    FocusPolicy focusPolicy(void) const noexcept { return m_focusPolicy; }
    void setFocusPolicy(FocusPolicy policy) noexcept { m_focusPolicy = policy; }

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

    void repaint(void) noexcept;
    void update(void) noexcept;
  protected:

// === Events ===
    virtual bool event(Event* event);


//            bool focusNextChild()
//            bool focusPreviousChild()
//    virtual bool focusNextPrevChild(bool next);

    // === Focus events ===
    virtual void focusInEvent   (FocusEvent* event) noexcept;
    virtual void focusOutEvent  (FocusEvent* event) noexcept;

    // === Keyboard events ===
    virtual void keyPressEvent  (KeyEvent* event) noexcept;
    virtual void keyReleaseEvent(KeyEvent* event) noexcept;

    // === Mouse events ===
    // Mouse: drag and drop
    virtual void dragEnterEvent (DragEnterEvent* event) noexcept;
    virtual void dragLeaveEvent (DragLeaveEvent* event) noexcept;
    virtual void dragMoveEvent  (DragMoveEvent*  event) noexcept;
    virtual void dropEvent      (DropEvent*      event) noexcept;

    // Mouse: position
    virtual void enterEvent     (Event* event) noexcept;
    virtual void leaveEvent     (Event* event) noexcept;

    // Mouse: buttons
    virtual void mouseDoubleClickEvent(MouseEvent* event) noexcept;
    virtual void mouseMoveEvent       (MouseEvent* event) noexcept;
    virtual void mousePressEvent      (MouseEvent* event) noexcept;
    virtual void mouseReleaseEvent    (MouseEvent* event) noexcept;
    virtual void wheelEvent           (WheelEvent* event) noexcept;

    // Paint/Layout events
    virtual void hideEvent  (HideEvent*   event) noexcept;
    virtual void moveEvent  (MoveEvent*   event) noexcept;
    virtual void paintEvent (PaintEvent*  event) noexcept;
    virtual void resizeEvent(ResizeEvent* event) noexcept;
    virtual void showEvent  (ShowEvent*   event) noexcept;

    // === Misc. events ===
    virtual void actionEvent      (ActionEvent*      event) noexcept;
    virtual void changeEvent      (Event*            event) noexcept;
    virtual void closeEvent       (CloseEvent*       event) noexcept;
    virtual void contextMenuEvent (ContextMenuEvent* event) noexcept;

  private:
    virtual Layout* layout(void) noexcept override { assert(false); return nullptr; }
    virtual bool isEmpty (void) const noexcept override { return layout() == nullptr; }

    rect_t      m_geometry;
    Layout*     m_layout;
    SizePolicy  m_sizePolicy;
    size2d_t    m_minimumSize;
    size2d_t    m_maximumSize;
    size2d_t    m_sizeHint;
    FocusPolicy m_focusPolicy;
  };
}

#endif // WIDGET_H
