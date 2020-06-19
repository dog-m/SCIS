#include <iostream>
using namespace std;
class Main {

  public:
    inline static unsigned int __INSTRUM_ID = 1;

  public:
    inline static int a = - 10;
    inline static int b = - 20;

    static void something_else ()
    {
        if (a >= b) {
            if (a < 0) cout << "Fizz Buzz!" << endl;
        }
        if (a >= b) {
            if (a < 0) cout << "Fizz Buzz!" << endl;
        }
    }

    static void main ();
};

void Main :: main ()
{
    __INSTRUM_ID = 2;
    {
        {
            std :: cout << "[LOG] '" << __INSTRUM_ID << "before <if_statement> block in [Main :: main] method" << std :: endl;
            if (a >= b) {
                if (a < 0) cout << "Hello World!" << endl;
            }
        }
        if (a >= b) {
            if (a < 0) cout << "Hello World!" << endl;
        }
    }
}

int main ()
{
    Main :: main ();
}

