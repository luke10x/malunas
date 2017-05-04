# Malunas: TCP connection proxy toolkit

[![Build Status](https://travis-ci.org/normantas/malunas.png)](https://travis-ci.org/normantas/malunas)

Server that waits for incoming TCP connections, when a socket is accepted,
it pipes the all data sent and received through that socket to
a backend module.

## Usage

```
    malunas [-w <workers>] <port> <module> [args...] 
```
- -w, --workers: the number of workers;
- port: server listening TCP port;
- module: backend module;
- args: module arguments.

## Modules

Currently _malunas_ have these modules:

### Exec

Uses an external command to handle a request.
It maps input and output of the request to a process started by an arbitrary command.

Usage:
```
    malunas [opts...] <port> exec [-t] <command...>
```
- opts: general options;
- command: any executable followed by parameters;
- -t, --tty: specifies whether the program should be run in a separate TTY.

## Docker

To create a shell container:
```
    docker run -it -p10023:23 malunas 23 exec sh
```
