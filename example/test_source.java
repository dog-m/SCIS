// javac test_source_instrumented.java
// java -cp .. example.Main
package example;

import java.util.logging.Level;
import java.util.logging.ConsoleHandler;

class Main {
  public static int a = -10;
  public static int b = -20;

  public static void something_else(String[] args) {
    if (a >= b) {
      if (a < 0)
        System.out.println("Fizz Buzz!");
    }

    if (a >= b) {
      if (a < 0)
        System.out.println("Fizz Buzz!");
    }
  }

  public static void main(String[] args) {
    if (a >= b) {
      if (a < 0)
        System.out.println("Hello World!");
    }

    if (a >= b) {
      if (a < 0)
        System.out.println("Hello World!");
    }
  }
}
