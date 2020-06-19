package example;

import java.util.logging.Level;

import java.util.logging.ConsoleHandler;

import java.util.logging.Logger;

import java.util.logging.Handler;

import java.lang.invoke.MethodHandles;

class Main {
    private static final Class iClass = MethodHandles.lookup ().lookupClass ();
    private static final Logger iLogger = Logger.getLogger (iClass.getName ());
    private static final Handler iHandler = new ConsoleHandler ();
    public static int a = - 10;
    public static int b = - 20;

    public static void something_else (String [] args) {
        if (a >= b) {
            if (a < 0) System.out.println ("Fizz Buzz!");

        }
        if (a >= b) {
            if (a < 0) System.out.println ("Fizz Buzz!");

        }
    }

    public static void main (String [] args) {
        iHandler.setLevel (Level.ALL);
        iLogger.addHandler (iHandler);
        iLogger.setLevel (Level.ALL);
        {
            {
                iLogger.log (Level.FINE, "before <if_statement> block in [Main] class, in {main} method");
                if (a >= b) {
                    if (a < 0) System.out.println ("Hello World!");

                }
            } if (a >= b) {
                if (a < 0) System.out.println ("Hello World!");

            }
        }}

}

