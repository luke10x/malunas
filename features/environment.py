from behave import *
import subprocess
import socket
import sys
import os

class ProgramManager:

    def __init__(self):
        self.program_handles =  []
        server_address = './pm.sock'

        # Make sure the socket does not already exist
        try:
            os.unlink(server_address)
        except OSError:
            if os.path.exists(server_address):
                raise

        self.servsock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.servsock.bind(server_address)
        self.servsock.listen(1)

    def accept(self):
        connection, client_address = self.servsock.accept()
        return connection

def before_scenario(context, scenario):
   context.program_manager = ProgramManager()
