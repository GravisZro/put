#include "widget.h"

namespace tui
{
  Widget::Widget(void) noexcept
    : m_geometry({{ 0, 0 }, { 0, 0 }}),
      m_layout(nullptr),
      m_sizePolicy(SizePolicy::Preferred, SizePolicy::Preferred),
      m_minimumSize({ 0, 0 }),
      m_maximumSize({ 0, 0 }),
      m_sizeHint({ 0, 0 }),
      m_focusPolicy(NoFocus)
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

  void Widget::repaint(void) noexcept
  {
    // TODO: invoke paintEvent() directly
  }

  void Widget::update(void) noexcept
  {
    Object::singleShot(this, &Widget::repaint);
  }

  void Widget::focusInEvent(FocusEvent* event) noexcept
  {
    if(focusPolicy() != NoFocus)
      update();
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::focusOutEvent(FocusEvent* event) noexcept
  {
    if(focusPolicy() != NoFocus)
      update();
    event->accept(); // TODO: verify in Qt source code
  }

// === Keyboard events ===
  void Widget::keyPressEvent(KeyEvent* event) noexcept
  {
    // TODO: verify in Qt source code
  }

  void Widget::keyReleaseEvent(KeyEvent* event) noexcept
  {
    // TODO: verify in Qt source code
  }

// === Mouse events ===
// Mouse: drag and drop
  void Widget::dragEnterEvent(DragEnterEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::dragLeaveEvent(DragLeaveEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::dragMoveEvent(DragMoveEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::dropEvent(DropEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

// Mouse: position
  void Widget::enterEvent(Event* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::leaveEvent(Event* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

// Mouse: buttons
  void Widget::mouseDoubleClickEvent(MouseEvent* event) noexcept
  {
    mousePressEvent(event); // verified correct
  }

  void Widget::mouseMoveEvent(MouseEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::mousePressEvent(MouseEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::mouseReleaseEvent(MouseEvent* event) noexcept
  {
    event->accept(); // TODO: verify in Qt source code
  }

  void Widget::wheelEvent(WheelEvent* event) noexcept
  {
    event->ignore(); // verified correct
  }

// Paint/Layout events
  void Widget::hideEvent(HideEvent* event) noexcept
  {
    // TODO!
  }

  void Widget::moveEvent(MoveEvent* event) noexcept
  {
    // TODO!
  }

  void Widget::paintEvent(PaintEvent* event) noexcept
  {
    // TODO!
  }

  void Widget::resizeEvent(ResizeEvent* event) noexcept
  {
    // TODO!
  }

  void Widget::showEvent(ShowEvent* event) noexcept
  {
    // TODO!
  }

// === Misc. events ===
  void Widget::actionEvent(ActionEvent* event) noexcept
  {
    // TODO!
  }

  void Widget::changeEvent(Event* event) noexcept
  {
    // TODO!
  }

  void Widget::closeEvent(CloseEvent* event) noexcept
  {
    // TODO!
  }

  void Widget::contextMenuEvent(ContextMenuEvent* event) noexcept
  {
    // TODO!
  }
}
