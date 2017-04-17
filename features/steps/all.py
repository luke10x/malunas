from behave import *
import subprocess
import socket
import sys

PORT = 10256

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
    cmd.extend([str(PORT), 'python', 'program.py'])

    context.server = subprocess.Popen(
        cmd,
        stdout = subprocess.PIPE,
        bufsize = 1)

@when(u'client connects')
def step_impl(context):
    context.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', PORT)
    context.client.connect(server_address)

@when(u'program handles the request')
def step_impl(context):
    context.controll_socket = context.program_manager.accept()

@when(u'program writes \'hello\'')
def step_impl(context):
    context.controll_socket.send('WRITE hello\n')

@when(u'program program flush output')
def step_impl(context):
    context.controll_socket.send('FLUSH\n')

@when(u'program exits')
@then(u'program exits')
def step_impl(context):
    context.controll_socket.send('EXIT\n')

@then(u'client receives \'hello\'')
def step_impl(context):

    sys.stdout.flush()
    line = context.client.recv(10) 
    assert("hello\n" == line), 'Message not received by client'
