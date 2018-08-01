# Tiny PostgresSQL Websocket API

## Introduction
With this sample script you can make a lot of things. See demos:

1. [WebSocket communication](https://ralac.openchain.pt/local/app/api/tinypgwebsocketapi.py?wstest)
2. [CGI environment](https://ralac.openchain.pt/local/app/api/tinypgwebsocketapi.py?print_env)
2. [Trace file viewer](https://ralac.openchain.pt/local/app/api/tinypgwebsocketapi.py?trace_viewer)

## Dependencies
#### debian
```
sudo apt-get install python-psycopg2 python-ws4py
```

#### other
```
pip install ws4py
pip install psycopg2
```

## HTTP Server
This project uses patched **Lighttpd** web server and can run under Linux or Cygwin. See [Wiki](https://github.com/ralac/tinypgwebsocketapi/wiki)

## Configuration

- **READING_BUFFER_SIZE**: Maximum number of bytes asked in each read to CGI pipe.
- **FD_POLLING_TIMEOUT**: Time in seconds for file descriptor polling timeout. The script sends websocket ping control frame to client for each timout on file descriptor polling, and traces received pong control frame message.
- **TRACE**: When `True` enables tracing to **TRACE_FILE**.
- **TRACE_FILE**: Trace file name.
