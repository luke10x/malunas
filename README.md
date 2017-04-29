# Malunas: MITM debugging toolkit

[![Build Status](https://travis-ci.org/normantas/malunas.png)](https://travis-ci.org/normantas/malunas)

Replacement server for client debugging.

It waits for incoming TCP connections, when a socket is accepted,
it pipes the all data sent and received through that socket to
a backend module.

Currently supported backend modules are:
- exec: allows using an external command to handle the request; 
- proxy: forwards request to a real backend, while monitoring the data
  which passes through

## Usage

```
    malunas [-t] [-w <workers>] port module [args...] 
```

## Docker

To create a shell container:
```
    docker run -it -p10023:23 malunas 23 exec sh
```
