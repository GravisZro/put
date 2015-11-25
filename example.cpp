#include "pdtk.h"

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
    queue(test_signal, num, 99);
  }

  void activated(int num, int num2)
  {
    std::cout << "num: " << num << std::endl;
    std::cout << "num2: " << num2 << std::endl;
  }

  signal<int, int> test_signal;
};

int main(int argc, char* argv[])
{
  Application app;

  ObjectDemo demo;
  std::cout << "before trigger" << std::endl;
  demo.trigger(7);
  std::cout << "after trigger" << std::endl;

  std::cout << "process signals" << std::endl;
  return app.exec();
}

/*

==== BUILD COMMAND ==== 
g++ example.cpp object.cpp -std=c++14 -o example

==== OUTPUT ====
before trigger
executing trigger
after trigger
process signals
num: 7
num2: 99

*/
