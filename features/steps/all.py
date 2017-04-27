from behave import *
import subprocess
import socket
import sys
import os
import re 
import signal
import random
import select

TIMEOUT = 5.0

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
    cmd.extend(['0', 'exec', 'python', 'program.py'])
    context.server = None 
    context.server = subprocess.Popen(
        cmd,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        bufsize = 1)
    poller = select.poll()
    poller.register(context.server.stderr, select.POLLIN)
    poll_result = poller.poll(TIMEOUT)
    if poll_result:
        line = context.server.stderr.readline()
        print(line)
        m = re.compile(r'^tcpexecd: Listening on 0.0.0.0:(?P<port>\d+)').match(line) 
        context.port = int(m.group('port'))
    assert(context.server.poll() is None), 'server died: %s' % context.server.stderr.read(100) 

@when(u'client connects')
def step_impl(context):
    context.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', context.port)
    context.client.connect(server_address)

@when(u'program handles the request')
def step_impl(context):
    connection, client_address = context.pm.accept()
    context.control_socket = connection

@when(u'program writes \'hello\'')
def step_impl(context):
    context.control_socket.send('WRITE hello\n')

@when(u'program flushes output')
def step_impl(context):
    context.control_socket.send('FLUSH\n')

@when(u'program exits')
def step_impl(context):
    context.control_socket.send('EXIT\n')

@then(u'client does not receive anything')
def step_impl(context):
    context.client.settimeout(TIMEOUT)
    try:
        line = context.client.recv(10) 
    except socket.timeout:
        return
    assert(None == line), 'Message some output was unexpectedly read from socket: %s' % line

@then(u'client receives \'hello\'')
def step_impl(context):
    context.client.settimeout(TIMEOUT)
    line = context.client.recv(10)
    assert("hello" == line.strip()), "Message not received by client, but received: '%s'" % line

@then(u'program recognizes that it runs in TTY')
def step_impl(context):
    context.control_socket.send('ISTTY?\n')
    line = context.control_socket.recv(10) 
    assert("TTY=1" == line.strip()), 'program cannot confirm that it runs in a TTY'

@then(u'program recognizes that it does not run in TTY')
def step_impl(context):
    context.control_socket.send('ISTTY?\n')
    line = context.control_socket.recv(10) 
    assert("TTY=0" == line.strip()), 'program cannot confirm that it does not run in a TTY'

@when(u'second client connects')
def step_impl(context):
    context.second_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', context.port)
    context.second_client.connect(server_address)

@then(u'program does not handle request')
def step_impl(context):
    try:
        connection, client_address = context.pm.accept()
        context.control_socket = connection
    except socket.timeout:
        return
    assert(False), 'A program handles a request but that is not expected'

@when(u'second program handles the request')
def step_impl(context):
    connection, client_address = context.pm.accept()
    context.second_control_socket = connection

@when(u'second program writes \'hello\'')
def step_impl(context):
    context.second_control_socket.send('WRITE hello\n')

@when(u'second program flushes')
def step_impl(context):
    context.second_control_socket.send('FLUSH\n')

@then(u'second client receives \'hello\'')
def step_impl(context):
    context.second_client.settimeout(TIMEOUT)
    line = context.second_client.recv(10)
    assert("hello" == line.strip()), "Message not received by client, but received: '%s'" % line
