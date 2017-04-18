from behave import *
import subprocess
import socket
import sys
import os
import signal
import random

PORT = 10257

@given(u'number of workers is {number}')
def step_impl(context, number):
    context.workers = number

@given(u'TTY mode is off')
def step_impl(context):
    context.tty = False

@given(u'TTY mode is on')
def step_impl(context):
    context.tty = True

@given(u'verbose mode is on')
def step_impl(context):
    context.verbose = True

@given(u'verbose mode is off')
def step_impl(context):
    context.verbose = False

@given(u'server is started')
def step_impl(context):
    cmd = ['./tcpexecd']
    if context.tty:
        cmd.append('-t')
    if context.verbose:
        cmd.append('-v')
    cmd.extend(['-w', context.workers])
    context.port = PORT + random.randint(1, 1000)
    cmd.extend([str(context.port), 'python', 'program.py'])
    context.server = None 
    context.server = subprocess.Popen(
        cmd,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        bufsize = 1)
    assert(context.server.poll() is None), 'server died: %s' % context.server.stderr.read(100) 

@when(u'client connects')
def step_impl(context):
    context.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', context.port)
    context.client.connect(server_address)

@when(u'program handles the request')
def step_impl(context):
    context.controll_socket = context.program_manager.accept()

@when(u'program writes \'hello\'')
def step_impl(context):
    context.controll_socket.send('WRITE hello\n')

@when(u'program flushes output')
def step_impl(context):
    context.controll_socket.send('FLUSH\n')

@when(u'program exits')
@then(u'program exits')
def step_impl(context):
    context.controll_socket.send('EXIT\n')

@then(u'client does not receive anything')
def step_impl(context):
    context.client.settimeout(2.0)
    try:
        line = context.client.recv(10) 
    except socket.timeout:
        return
    assert(None == line), 'Message some output was unexpectedly read from socket: %s' % line

@then(u'client receives \'hello\'')
def step_impl(context):
    #sys.stdout.flush() # XXX not quite sure why it is here
    context.client.settimeout(2.0)
    line = context.client.recv(10)
    assert("hello" == line.strip()), "Message not received by client, but received: '%s'" % line

@then(u'program recognizes that it runs in TTY')
def step_impl(context):
    context.controll_socket.send('ISTTY?\n')
    line = context.controll_socket.recv(10) 
    assert("TTY=1" == line.strip()), 'program cannot confirm that it runs in a TTY'

@then(u'program recognizes that it does not run in TTY')
def step_impl(context):
    context.controll_socket.send('ISTTY?\n')
    line = context.controll_socket.recv(10) 
    assert("TTY=0" == line.strip()), 'program cannot confirm that it does not run in a TTY'
