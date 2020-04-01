from logging import *

class Main:
    def __init__(self):
        self.a = 10
        self.b = 20

    def something_else(self, args):
        if self.a >= self.b:
          if a < 0:
            print "Fizz Buzz!"

        if a >= b:
          if a < 0:
            print "Fizz Buzz!"

    def main(self, args):
        if self.a >= self.b:
          if self.a < 0:
            print "Hello World!"

        if self.a >= self.b:
          if self.a < 0:
            print "Hello World!"
"""
        for row in keys:
            rowa = Frame(self, bg='gray40')
            rowb = Frame(self, bg='gray40')
            for p1, p2, p3, color, ktype, func in row:
                if ktype == FUN:
                    a = lambda s=self, a=func: s.evalAction(a)
                else:
                    a = lambda s=self, k=func: s.keyAction(k)
                SLabel(rowa, p2, p3)
                Key(rowb, text=p1, bg=color, command=a)
            rowa.pack(side=TOP, expand=YES, fill=BOTH)
            rowb.pack(side=TOP, expand=YES, fill=BOTH)            
"""

Main().main({})