# Malunas: server debugging toolkit

[![Build Status](https://travis-ci.org/normantas/malunas.png)](https://travis-ci.org/normantas/malunas)

Replacement server for client debugging.

It waits for incoming TCP connections, when a socket is accepted,
it pipes the all data sent and received through that socket to
a backend module.

## Usage

```
    malunas [-w <workers>] port module [args...] 
```

## Modules

Currently _malunas_ have these modules:

### Exec

Uses an external command to handle a request.
It maps input and output of the request to a process started by an arbitrary command.

Usage:
```
    malunas <port> exec [-t] <command...>
```
Here command can be any executable followed by parameters.
-t, --tty specifies whether the program should be run in a separate TTY.

## Docker

To create a shell container:
```
    docker run -it -p10023:23 malunas 23 exec sh
```
