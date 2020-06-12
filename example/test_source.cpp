// g++ test_source_instrumented.cpp -std=c++17
// a
#include <iostream>

using namespace std;

class Main {
public:
  inline static int a = -10;
  inline static int b = -20;

  static void something_else() {
    if (a >= b) {
      if (a < 0)
        cout << "Fizz Buzz!" << endl;
    }

    if (a >= b) {
      if (a < 0)
        cout << "Fizz Buzz!" << endl;
    }
  }

  static void main();
};

void Main::main() {
  if (a >= b) {
    if (a < 0)
      cout << "Hello World!" << endl;
  }

  if (a >= b) {
    if (a < 0)
      cout << "Hello World!" << endl;
  }
}

int main() {
  Main::main();
}
