import time
import sys
import os
def myprint(stri):
     #print >> sys.stderr, stri
     print stri
     #sys.stdout.flush()


myprint("stding fileno: %d " % sys.stdin.fileno())
myprint("is a tty?: %d"% os.isatty(sys.stdin.fileno()))
myprint("This is not a test")
name = raw_input("What's your name? ")
myprint("give me a break:")
time.sleep(2) 
myprint("Nice to meet you " + name + "!")

name = raw_input("What's your name (again)? ")
myprint("give me a break:")
time.sleep(2) 
myprint("Nice to meet you " + name + "!")
