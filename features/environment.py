from behave import *
import subprocess
import socket
import sys
import os

def before_scenario(context, scenario):
    server_address = './pm.sock'

    # Make sure the socket does not already exist
    try:
        os.unlink(server_address)
    except OSError:
        if os.path.exists(server_address):
            raise

    context.pm = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    context.pm.bind(server_address)
    context.pm.listen(1)
    context.pm.settimeout(2.0)
