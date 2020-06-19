import os
# python test_source_instrumented.py
import logging

class Main :
    def __init__(self) :
        self.a = - 10
        self.b = - 20

    def something_else(self, args) :
        if self.a >= self.b :
            if self.a < 0 :
                print "Fizz Buzz!"

        if self.a >= self.b :
            if self.a < 0 :
                print "Fizz Buzz!"

    def main(self, args) :
        logging.basicConfig()
        self.iLogger = logging.getLogger("simpleExample")
        self.iLogger.setLevel(logging.DEBUG)
        self.iLogger.info("before <if_statement> block in [Main] class, in {main} method")
        if self.a >= self.b :
            if self.a < 0 :
                print "Hello World!"

        if self.a >= self.b :
            if self.a < 0 :
                print "Hello World!"

Main().main({})
