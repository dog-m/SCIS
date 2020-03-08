public class Foo {

  public static void main(String[] args) {

    class Bar {
      public void test() { System.out.println("Hello test"); }
    }
    
    Bar b = new Bar();
    b.test();
  }
}