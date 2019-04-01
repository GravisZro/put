#ifndef PUT_SDL_H
#define PUT_SDL_H

// integration
#include <SDL2/SDL.h>

// PUT
#include <put/object.h>

static_assert(sizeof(SDL_EventType) == sizeof(uint32_t), "bad size");

namespace SDL
{
  enum class Button : uint8_t
  {
    A = SDL_CONTROLLER_BUTTON_A,
    B,
    X,
    Y,
    Back,
    Guide,
    Start,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
  };

  enum audio_device_t : uint8_t
  {
    output = 0,
    capture
  };

  typedef uint32_t windowid_t;

  class Events : public Object
  {
  public:
    Events(void) noexcept;
   ~Events(void) noexcept;

    signal<uint32_t, audio_device_t> AudioDeviceAdded; // device index, device type
    signal<uint32_t, audio_device_t> AudioDeviceRemoved; // device index, device type

    signal<SDL_JoystickID> ControllerDeviceAdded; // joystick id
    signal<SDL_JoystickID> ControllerDeviceRemoved; // joystick id
    signal<SDL_JoystickID> ControllerDeviceRemapped; // joystick id

    signal<SDL_JoystickID, uint8_t, int16_t> ControllerAxisMotion;   // joystick id, axis, value

    signal<SDL_JoystickID, Button> ControllerButtonPressed; // joystick id, button number
    signal<SDL_JoystickID, Button> ControllerButtonReleased; // joystick id, button number



    signal<SDL_JoystickID> JoystickDeviceAdded; // joystick id
    signal<SDL_JoystickID> JoystickDeviceRemoved; // joystick id

    signal<SDL_JoystickID, uint8_t, int16_t> JoystickAxisMotion;   // joystick id, axis, value

    signal<SDL_JoystickID, Button> JoystickButtonPressed; // joystick id, button number
    signal<SDL_JoystickID, Button> JoystickButtonReleased; // joystick id, button number



    signal<windowid_t, SDL_Keysym> KeyPressed; // window id, key identifier
    signal<windowid_t, SDL_Keysym> KeyReleased; // window id, key identifier

  private:
    void event_thread(void) noexcept;
  };

  class Window : public Object
  {
  public:
    Window(const char* title,
           int width,
           int height,
           int pos_x = SDL_WINDOWPOS_UNDEFINED,
           int pos_y = SDL_WINDOWPOS_UNDEFINED,
           SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE) noexcept;
   ~Window(void) noexcept;


    windowid_t id(void) const { return SDL_GetWindowID(m_pWindow); }

    signal<SDL_Keysym> KeyPressed; // key identifier
    signal<SDL_Keysym> KeyReleased; // key identifier

  private:
    SDL_Window* m_pWindow;
  };
}


#endif // PUT_SDL_H
