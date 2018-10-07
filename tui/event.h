#ifndef EVENT_H
#define EVENT_H


class Event
{
public:
  enum class Type
  {
    None,
    ActionAdded,
    ActionChanged,
    ActionRemoved,
    ActivationChange,
    ApplicationActivate,
    ApplicationActivated,
    ApplicationDeactivate,
    ApplicationFontChange,
    ApplicationLayoutDirectionChange,
    ApplicationPaletteChange,
    ApplicationStateChange,
    ApplicationWindowIconChange,
    ChildAdded,
    ChildPolished,
    ChildRemoved,
    Clipboard,
    Close,
    CloseSoftwareInputPanel,
    ContentsRectChange,
    ContextMenu,
    CursorChange,
    DeferredDelete,

    DynamicPropertyChange,
    EnabledChange,
    Enter,
    EnterEditFocus,
    EnterWhatsThisMode,
    Expose,
    FileOpen,
    FocusIn,
    FocusOut,
    FocusAboutToChange,
    FontChange,
    Gesture,
    GestureOverride,
    GrabKeyboard,
    GrabMouse,
    Hide,
    HideToParent,
    HoverEnter,
    HoverLeave,
    HoverMove,
    IconDrag,
    IconTextChange,
    InputMethod,
    InputMethodQuery,
    KeyboardLayoutChange,
    KeyPress,
    KeyRelease,
    LanguageChange,
    LayoutDirectionChange,
    LayoutRequest,
    Leave,
    LeaveEditFocus,
    LeaveWhatsThisMode,
    LocaleChange,
    MetaCall,
    ModifiedChange,
    Move,
    OrientationChange,
    Paint,
    PaletteChange,
    ParentAboutToChange,
    ParentChange,
    PlatformPanel,
    PlatformSurface,
    Polish,
    PolishRequest,
    QueryWhatsThis,
    ReadOnlyChange,
    RequestSoftwareInputPanel,
    Resize,
    ScrollPrepare,
    Scroll,
    Shortcut,
    ShortcutOverride,
    Show,
    ShowToParent,
    SockAct,
    StatusTip,
    StyleChange,
    ThreadChange,
    Timer,
    ToolBarChange,
    ToolTip,
    ToolTipChange,
    TouchBegin,
    TouchCancel,
    TouchEnd,
    TouchUpdate,
    UngrabKeyboard,
    UngrabMouse,
    UpdateLater,
    UpdateRequest,
    WhatsThis,
    WhatsThisClicked,
    Wheel,
    WinIdChange,
    ZOrderChange,


    MouseButtonDblClick,
    MouseButtonPress,
    MouseButtonRelease,
    MouseMove,
    MouseTrackingChange,

    DragEnter,
    DragLeave,
    DragMove,
    Drop,


    WindowActivate,
    WindowBlocked,
    WindowDeactivate,
    WindowIconChange,
    WindowStateChange,
    WindowTitleChange,
    WindowUnblocked,

  };

  Event(Type type) : m_type(type), m_accepted(false) { }
  virtual ~Event(void) = default;

  void accept(void) { setAccepted(true ); }
  void ignore(void) { setAccepted(false); }

  bool isAccepted() const { return m_accepted; }
  void setAccepted(bool accepted) { m_accepted = accepted; }

  Type type(void) const { return m_type; }

private:
  Type m_type;
  bool m_accepted;
};


class FocusEvent : public Event
{
public:
  enum class FocusReason
  {
    MouseFocusReason,         // A mouse action occurred.
    TabFocusReason,           // The Tab key was pressed.
    BacktabFocusReason,       // A Backtab occurred. The input for this may include the Shift or Control keys; e.g. Shift+Tab.
    ActiveWindowFocusReason,  // The window system made this window either active or inactive.
    PopupFocusReason,         // The application opened/closed a pop-up that grabbed/released the keyboard focus.
    ShortcutFocusReason,      // The user typed a label's buddy shortcut
    MenuBarFocusReason,       // The menu bar took focus.
    OtherFocusReason,         // Another reason, usually application-specific.
  };

  FocusEvent(Event::Type _type, FocusReason _reason)
    : Event(_type),
      m_reason(_reason) { }

  bool gotFocus (void) const noexcept { return type() == Type::FocusIn; }
  bool lostFocus(void) const noexcept { return type() == Type::FocusOut; }

  FocusReason reason(void) const noexcept { return m_reason; }
private:
  FocusReason m_reason;
};

class KeyEvent : public Event
{

};

class DragEnterEvent : public Event
{

};

class DragLeaveEvent : public Event
{

};

class DragMoveEvent : public Event
{

};

class DropEvent : public Event
{

};

class MouseEvent : public Event
{

};

class WheelEvent : public Event
{

};

class HideEvent : public Event
{

};

class MoveEvent : public Event
{

};

class PaintEvent : public Event
{

};

class ResizeEvent : public Event
{

};

class ShowEvent : public Event
{

};

class ActionEvent : public Event
{

};

class CloseEvent : public Event
{

};

class ContextMenuEvent : public Event
{

};


#endif // EVENT_H
