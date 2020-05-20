#include <iostream>

using namespace std;

class Main {

  int a = 20;
  int b = 10;

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
