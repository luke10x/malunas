# EasySrv

Is a generic server, primarily designed to facilitate quick creation of mocks and prototypes.
EasySrv listens for incoming TCP connections, when a new connection is accepted,
it pipes received data from the socket to the stdin of a handling process,
meanwhile all output of the handling process is sent to the accepted connection session.
A new external handling process is forked for each accepted connection,
running a specified system command. When the process terminates, the connection is closed.

