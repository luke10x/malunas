# tcpexecd

[![Build Status](https://travis-ci.org/normantas/tcpexecd.png)](https://travis-ci.org/normantas/tcpexecd)

A generic TCP server, that uses arbitrary command as request handler.

It waits for incoming TCP connections, when a socket is accepted,
it starts a custom process, piping stdin/stdout of that process to that socket.

This allows application developers, to focus on their domain specifics,
using simple standard IO interface, so that they don't have
to worry about network connections. At the same time it does not bind
the developers to a specific Application Layer protocol, like HTTP.

## Usage

```
    tcpexecd [-t] [-w NUMBER] [-v] PORT COMMAND
```

## Docker

To create a shell container:
```
    docker run -it -p10023:23 tcpexecd 23 sh
```
