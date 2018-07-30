# Tiny PostgresSQL Web API

## Table of contents
[TOC]

## Introduction
With this sample script you can make a lot of things:

1. [WebSocket communication](tinypgwebapi.cgi?wstest)
2. [CGI environment](tinypgwebapi.cgi?print_env)
2. [Trace file viewer](tinypgwebapi.cgi?trace_viewer)
3. [README](tinypgwebapi.cgi?readme)

## Dependencies
### Installation
#### debian
```
:::bash
sudo apt-get install python-psycopg2 python-ws4py python-pygments python-markdown
```
#### other
```
:::bash
pip install ws4py
pip install psycopg2
pip install pygments
pip install markdown
```
## HTTP Server
The script is beeing tested with **lighttpd-1.4.49** and **Tiny Tiny HTTPD** web servers. See (Wiki)[Wiki/Home]

## Configuration

- **READING_BUFFER_SIZE**: Maximum number of bytes asked in each read to CGI pipe.
- **FD_POLLING_TIMEOUT**: Time in seconds for file descriptor polling timeout. The script sends websocket ping control frame to client for each timout on file descriptor polling, and traces received pong control frame message.
- **TRACE**: When `True` enables tracing to **TRACE_FILE**.
- **TRACE_FILE**: Trace file name.
