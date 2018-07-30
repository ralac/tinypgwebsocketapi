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
The script is beeing tested with **lighttpd-1.4.49** and **Tiny Tiny HTTPD** web servers.

### Lighttpd (lighty)
Download source code for **lighttpd-1.4.49** and extract all files in a folder of your choice.
```
:::bash
cd path-do-folder
wget https://download.lighttpd.net/lighttpd/releases-1.4.x/lighttpd-1.4.49.tar.gz
tar -xvzf lighttpd-1.4.49.tar.gz
cd lighttpd-1.4.49
```

In order to prevent **lighty** from close **CGI** connection after websocket upgrade, we need to fix file `src/mod_cgi.c`.
```
:::diff
*** lighttpd-1.4.49.original/src/mod_cgi.c        2018-03-12 00:52:20.000000000 +0000
--- lighttpd-1.4.49.new/src/mod_cgi.c        2018-07-28 18:06:40.546179700 +0100
***************
*** 797,803 ****

                cgi_pid_add(p, hctx->pid, hctx);

!               if (0 == con->request.content_length) {
                        close(to_cgi_fds[1]);
                } else {
                        /* there is content to send */
--- 797,803 ----

                cgi_pid_add(p, hctx->pid, hctx);

!               if (0 == con->request.content_length && !hctx->conf.upgrade) {
                        close(to_cgi_fds[1]);
                } else {
                        /* there is content to send */

```
#####Build on Linux
```
:::bash
./configure --with-openssl
make
sudo make install
```
#####Build on Cygwin
```
:::bash
./configure --build=i686-pc-cygwin --with-openssl
make
make install
```
####Configuration
To run **lighty** on Cygwin, modify `lighttpd.cong` as follows:
```
:::diff
*** path-do-folder/lighttpd-1.4.49/doc/config/lighttpd.conf    2018-03-12 00:52:20.000000000 +0000
--- /var/lib/lighttpd/lighttpd.conf     2018-07-29 09:30:06.092363800 +0100
***************
*** 85,96 ****
  ##  Basic Configuration
  ## ---------------------
  ##
! server.port = 80

  ##
  ## Use IPv6?
  ##
! server.use-ipv6 = "enable"

  ##
  ## bind to a specific IP
--- 85,96 ----
  ##  Basic Configuration
  ## ---------------------
  ##
! server.port = 8090

  ##
  ## Use IPv6?
  ##
! #server.use-ipv6 = "enable"

  ##
  ## bind to a specific IP
***************
*** 178,184 ****
  ##
  ## linux-sysepoll is recommended on kernel 2.6.
  ##
! server.event-handler = "linux-sysepoll"

  ##
  ## The basic network interface for all platforms at the syscalls read()
--- 178,184 ----
  ##
  ## linux-sysepoll is recommended on kernel 2.6.
  ##
! #server.event-handler = "linux-sysepoll"

  ##
  ## The basic network interface for all platforms at the syscalls read()
```

To enable **CGI**, modify `modules.conf` as follows:
```
:::diff
*** path-do-folder/lighttpd-1.4.49/doc/config/modules.conf     2018-03-12 00:52:20.000000000 +0000
--- /var/lib/lighttpd/modules.conf      2018-07-29 09:08:42.262075900 +0100
***************
*** 140,146 ****
  ##
  ## plain old CGI (mod_cgi)
  ##
! #include "conf.d/cgi.conf"

  ##
  #######################################################################
--- 140,146 ----
  ##
  ## plain old CGI (mod_cgi)
  ##
! include "conf.d/cgi.conf"

  ##
  #######################################################################
```

To enable **CGI** websocket upgrade and correct buffering, modify `cgi.conf` as follows:
```
:::diff
*** path-do-folder/lighttpd-1.4.49/doc/config/conf.d/cgi.conf  2018-03-12 00:52:20.000000000 +0000
--- /var/lib/lighttpd/conf.d/cgi.conf   2018-07-29 10:56:14.585175400 +0100
***************
*** 29,33 ****
--- 29,51 ----
  #   cgi.assign = ( "" => "" )
  #}

+ $HTTP["url"] =~ "\/app\/api\/.*\.log" {
+      url.access-deny = ("")
+ }
+
+ $HTTP["url"] =~ "\/app\/api\/tinypgwebapi.cgi" {
+     ##
+     ## Enable CGI upgrade
+     cgi.upgrade = "enable"
+
+     ##
+     ## Avoid buffer entire request body before connecting to backend
+     server.stream-request-body = 2
+
+     ##
+     ## Avoid buffer entire response body before sending to client
+     server.stream-response-body = 2
+ }
+
  ##
  #######################################################################
```
####Running
```
:::bash
/usr/local/sbin/lighttpd -f /var/lib/lighttpd/lighttpd.conf
```
###Tiny Tiny HTTPD
#####Build
```
:::bash
git clone https://github.com/mattn/tinytinyhttpd
cd tinytinyhttpd
./configure
make
sudo make install
```
####Running
```
/usr/local/sbin/tthttpd -p 8090 -d /srv/www/htdocs/ &
```

##Script
###Configuration

- **READING_BUFFER_SIZE**: Maximum number of bytes asked in each read to CGI pipe.
- **FD_POLLING_TIMEOUT**: Time in seconds for file descriptor polling timeout. The script sends websocket ping control frame to client for each timout on file descriptor polling, and traces received pong control frame message.
- **TRACE**: When `True` enables tracing to **TRACE_FILE**.
- **TRACE_FILE**: Trace file name.

###Source
