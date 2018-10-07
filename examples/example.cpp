#include "object.h"

#include <csignal>
#include <iostream>

class ObjectDemo : public Object
{
public:

  ObjectDemo(void)
  {
    connect(test_signal, this, &ObjectDemo::activated);
  }

  void trigger(int num)
  {
    std::cout << "executing trigger" << std::endl;
    int num2 = 99;
    enqueue(test_signal, num, num2);
  }

  void activated(int num, int num2)
  {
    std::cout << "num: " << num << std::endl;
    std::cout << "num2: " << num2 << std::endl;
  }

  signal<int, int> test_signal;
};

void exiting(void)
{
  std::cout << "exiting" << std::endl;
}

int main(int argc, char* argv[])
{
  Application app;
  std::atexit(exiting);
  std::signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, [](int){ Application::quit(0); });

  ObjectDemo demo;
  std::cout << "before trigger" << std::endl;
  demo.trigger(7);
  std::cout << "after trigger" << std::endl;

  std::cout << "process signals" << std::endl;
  return app.exec();
}

/*

==== BUILD COMMAND ==== 
g++ example.cpp application.cpp specialized/eventbackend.cpp -I. -std=c++14 -Os -fno-exceptions -fno-rtti -o example

==== OUTPUT ====
before trigger
executing trigger
after trigger
process signals
num: 7
num2: 99

*/
