#include "object.h"

#include <csignal>
#include <iostream>


#include <tui/keyboard.h>

void exiting(void)
{
  std::cout << "exiting" << std::endl;
}

void printKey(uint8_t key)
{
  if(key > 0x20 && key < 0x7F)
    printf(" %c ", key);
  else if (key)
    printf(" %02x", key);
}

void keyOutput(uint64_t key) noexcept
{
  printf("key: %016lx -", key);

  for(int i = 0; i < 8; ++i)
    printKey(reinterpret_cast<uint8_t*>(&key)[i]);
  printf("\n");
}

int main(int argc, char* argv[])
{
  Application app;
  std::atexit(exiting);
  std::signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, [](int){ Application::quit(0); });

  tui::Keyboard kb;
  Object::connect(kb.keyPressed, keyOutput);

  return app.exec();
}

/*
==== BUILD COMMAND ==== 
g++ inputexample.cpp application.cpp specialized/eventbackend.cpp tui/keyboard.cpp -I. -std=c++14 -Os -fno-exceptions -fno-rtti -o inputexample

*/
