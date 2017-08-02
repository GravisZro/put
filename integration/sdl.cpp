#include "sdl.h"

#include <chrono>
#include <thread>
#include <map>

// POSIX++
#include <cstdlib> // for exit

#ifdef SDL_DEBUG
#include <iostream>
#endif

#include <cxxutils/error_helpers.h>

static std::thread sdl_thread;
static std::map<SDL_Window*, SDL::Window*> window_lookup;

SDL::Window* sdl_window_lookup(uint32_t id)
{
  SDL_Window* ptr = SDL_GetWindowFromID(id);
  if(ptr == nullptr)
    return nullptr;
  auto pos = window_lookup.find(ptr);
  if(pos == window_lookup.end())
    return nullptr;
  return pos->second;
}

int sdl_event_filter(SDL::Events* obj, SDL_Event* event) noexcept
{
  SDL::Window* win = nullptr;
  switch(event->type)
  {
    case SDL_AUDIODEVICEADDED:
      Object::enqueue(obj->AudioDeviceAdded,
                      *reinterpret_cast<uint32_t*>(&event->adevice.which),
                      *reinterpret_cast<SDL::audio_device_t*>(&event->adevice.iscapture));
      break;
    case SDL_AUDIODEVICEREMOVED:
      Object::enqueue(obj->AudioDeviceRemoved,
                      *reinterpret_cast<uint32_t*>(&event->adevice.which),
                      *reinterpret_cast<SDL::audio_device_t*>(&event->adevice.iscapture));
      break;
// CONTROLLER
    case SDL_CONTROLLERDEVICEADDED:
      Object::enqueue(obj->ControllerDeviceAdded,
                   event->cdevice.which);
      break;
    case SDL_CONTROLLERDEVICEREMOVED:
      Object::enqueue(obj->ControllerDeviceRemoved,
                   event->cdevice.which);
      break;
    case SDL_CONTROLLERDEVICEREMAPPED:
      Object::enqueue(obj->ControllerDeviceRemapped,
                   event->cdevice.which);
      break;

    case SDL_CONTROLLERAXISMOTION:
      Object::enqueue(obj->ControllerAxisMotion,
                      event->caxis.which,
                      *reinterpret_cast<uint8_t*>(&event->caxis.axis),
                      *reinterpret_cast<int16_t*>(&event->caxis.value));
      break;

    case SDL_CONTROLLERBUTTONDOWN:
      Object::enqueue(obj->ControllerButtonPressed,
                      event->cbutton.which,
                      *reinterpret_cast<SDL::Button*>(&event->cbutton.button));
      break;

    case SDL_CONTROLLERBUTTONUP:
      Object::enqueue(obj->ControllerButtonReleased,
                      event->cbutton.which,
                      *reinterpret_cast<SDL::Button*>(&event->cbutton.button));
      break;
// JOYSTICK

    case SDL_JOYDEVICEADDED:
      Object::enqueue(obj->JoystickDeviceAdded,
                   event->jdevice.which);
      break;
    case SDL_JOYDEVICEREMOVED:
      Object::enqueue(obj->JoystickDeviceRemoved,
                   event->jdevice.which);
      break;

    case SDL_JOYAXISMOTION:
      Object::enqueue(obj->JoystickAxisMotion,
                      event->jaxis.which,
                      *reinterpret_cast<uint8_t*>(&event->jaxis.axis),
                      *reinterpret_cast<int16_t*>(&event->jaxis.value));
      break;

    case SDL_JOYBUTTONDOWN:
      Object::enqueue(obj->JoystickButtonPressed,
                      event->jbutton.which,
                      *reinterpret_cast<SDL::Button*>(&event->jbutton.button));
      break;

    case SDL_JOYBUTTONUP:
      Object::enqueue(obj->JoystickButtonReleased,
                      event->jbutton.which,
                      *reinterpret_cast<SDL::Button*>(&event->jbutton.button));
      break;

// KEYBOARD
    case SDL_KEYDOWN:
      win = sdl_window_lookup(event->key.windowID);
      if(win != nullptr)
        Object::enqueue(win->KeyPressed, event->key.keysym);
      Object::enqueue(obj->KeyPressed, event->key.windowID, event->key.keysym);
      break;

    case SDL_KEYUP:
      win = sdl_window_lookup(event->key.windowID);
      if(win != nullptr)
        Object::enqueue(win->KeyReleased, event->key.keysym);
      Object::enqueue(obj->KeyReleased, event->key.windowID, event->key.keysym);
      break;

    default:
#ifdef SDL_DEBUG
      std::cout << "event type: " << std::hex << event->type << std::endl << std::flush;
#endif
      break;
  }
  return 0;
}

namespace SDL
{
  Events::Events(void) noexcept
  {
    if(!sdl_thread.joinable()) // if thread already active
      sdl_thread = std::thread(&Events::event_thread , this); // create a new thread
  }

  Events::~Events(void) noexcept
  {
    SDL_Event sdlevent;
    sdlevent.type = SDL_QUIT;
    SDL_PushEvent(&sdlevent); // trick SDL into thinking it's quiting
    sdl_thread.detach(); // detach because thread will self terminate
  }

  void Events::event_thread(void) noexcept
  {
    SDL_SetMainReady();
    flaw(SDL_Init(SDL_INIT_EVERYTHING) != posix::success_response, posix::critical, , std::exit(4),
         "SDL could not initialize.")
    SDL_SetEventFilter(reinterpret_cast<SDL_EventFilter>(sdl_event_filter), static_cast<void*>(this));
    while(SDL_WaitEvent(nullptr) == 0); // run until there is an error
  }

  Window::Window(const char* title,
                 int width,
                 int height,
                 int pos_x,
                 int pos_y,
                 SDL_WindowFlags flags) noexcept
  {
    while(!(SDL_WasInit(0) & SDL_INIT_VIDEO))
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_pWindow = SDL_CreateWindow(title, pos_x, pos_y, width, height, flags);

    window_lookup.emplace(m_pWindow, this);
  }

  Window::~Window(void) noexcept
  {
    window_lookup.erase(m_pWindow);
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
  }

}
