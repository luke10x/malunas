"""
Script that creates a controllable proces.

It can be controlled through unix socket: `pm.sock`
This script connects to the socket as a client,
and reacts to commands sent from a server.
A command is a line of text, terminated by \n.
Available commands are defined in `exec_cmd`, and
they can tell to process executing this script to:
- print something to stdout;
- read from stout;
- flush output;
- exit.

The script is executed running command:
```
    python program.py
```

To demo how the script reacts to server commands:
```
    printf "WRITE 444\nREAD\nEXIT\n" | nc -U "./pm.sock" -l
```
"""
import socket
import sys
import os
import re

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server_address = './pm.sock'
sock.connect(server_address)

def read_line(sock):
    chars = []
    while True:
        a = sock.recv(1)
        if a == "\n":# or a == "":
            yield "".join(chars)
            chars = []
        else:
            chars.append(a)     
        if a == "":
            return

def exec_cmd(line, sock):
    m = re.compile(r'^WRITE (?P<msg>.+)').match(line) 
    if m:
        print("%s" % m.group('msg'))
        return

    if re.compile(r'^READ').match(line): 
        read = raw_input("")
        sock.send("%s\n" % read)
        return

    if re.compile(r'^FLUSH').match(line): 
        sys.stdout.flush()
        return

    if re.compile(r'^EXIT').match(line): 
        exit(0)
        return

    sock.send("UNKNOWN COMMAND: '%s'\n" % line)
    
for line in read_line(sock):
    exec_cmd(line, sock)
