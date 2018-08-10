#!/usr/bin/python

import os
import sys
import json
import psycopg2

# Configuration
READING_BUFFER_SIZE = os.environ['READING_BUFFER_SIZE']
FD_POLLING_TIMEOUT  = float(os.environ['FD_POLLING_TIMEOUT'])
TRACE               = bool(os.environ['TRACE']=='enabled')

self_pid=os.getpid()

SYS_PAGE_SIZE=-1
def memory_usage_ps():
    import subprocess
    global SYS_PAGE_SIZE
    if SYS_PAGE_SIZE == -1:
        SYS_PAGE_SIZE = int(subprocess.check_output(["getconf", "PAGE_SIZE"]).strip())
    statm_file = open('/proc/%d/statm' % self_pid, 'r')
    data = statm_file.readline()
    statm_file.close()
    mem_values = data.rstrip().split(' ')
    return str(int(mem_values[5])*SYS_PAGE_SIZE/1024)+ ' Kb'


def log_message(msg):
    if TRACE:
        sys.stderr.write(msg+'\n')
        sys.stderr.flush()


def websocket_test():
    import time
    import hashlib
    import base64
    import select
    import traceback
    import ws4py.websocket

    class stdin_as_socket(object):
        active=True
        def getsockname():
            return (os.environ['SERVER_ADDR'], os.environ['SERVER_PORT'])
        def getpeername():
            return (os.environ['REMOTE_ADDR'], os.environ['REMOTE_PORT'])
        def shutdown(flag):
            active = False
        def close():
            active = False
        def sendall(fd, b):
            sys.stdout.write(b)
            sys.stdout.flush()
        def pending():
            return READING_BUFFER_SIZE
        def recv(fd, numbytes):
            return sys.stdin.read(numbytes)
        def setblocking(value):
            import fcntl
            if not value:
                fd = sys.stdin.fileno()
                fl = fcntl.fcntl(fd, fcntl.F_GETFL)
                fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    def echo_cb(received_message):
        #global ws
        senging_message = {
            'time': time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()),
            'message':
                '<span style="color: black;">memory usage: ' + \
                memory_usage_ps() + \
                ' </span>' + \
                str(received_message)
        }
        log_message('received_message: '+str(received_message))
        ws.send(json.dumps(senging_message))

    def pong_cb(pong):
        log_message('received PONG: ' + str(pong))

    # Websocket handshake
    combined = os.environ['HTTP_SEC_WEBSOCKET_KEY'] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    accept = base64.b64encode(hashlib.sha1(combined).digest())
    sys.stdout.write("HTTP/1.1 101 Switching Protocols\r\n")
    sys.stdout.write("upgrade: WebSocket\r\n")
    sys.stdout.write("Connection: Upgrade\r\n")
    sys.stdout.write("Sec-WebSocket-Accept: %(acceptstring)s\r\n" % { 'acceptstring' : accept })
    sys.stdout.write("Server: %(server)s\n" % { 'server': os.environ['SERVER_NAME'] })
    sys.stdout.write("Access-Control-Allow-Origin: %(origin)s\r\n" % { 'origin': os.environ['HTTP_ORIGIN']})
    sys.stdout.write("Access-Control-Allow-Credentials: true\r\n\r\n")
    sys.stdout.flush()

    # Websocket run
    stdin_sock = stdin_as_socket()
    ws = ws4py.websocket.WebSocket(stdin_sock)
    ws.ponged = pong_cb
    ws.received_message = echo_cb
    ws.opened()

    while stdin_sock.active and not ws.terminated:
        fds = [ sys.stdin, ]
        inputready, outputready, exceptready = select.select(fds, [], fds, FD_POLLING_TIMEOUT)
        if not inputready:
            ws.ping(u"Trying to keep connection alive.")
        else:
            for fd in inputready:
                if fd not in exceptready and fd == sys.stdin:
                    stdin_sock.active = ws.once()
                else:
                    #handle the postgreSQL socket
                    a="notify"


def session_id():
    try:
        import Cookie
        import uuid
        cookie = Cookie.SimpleCookie(os.environ["HTTP_COOKIE"])
        session_id = cookie["session_id"].value
    except:
        session_id = str(uuid.uuid1()).replace('-', '')
        print 'Set-Cookie: session_id=%s' % session_id


def print_wstest():
    print 'Content-Type: text/html'
    print
    print """
<!DOCTYPE html>
<html>
    <head>
        <title>Tiny PostgresSQL Websocket API</title>
        <style>
            h1 {
                margin-left: 20px;
            }
            .time { color: green }
            .message { color: blue; font-weight: bold }
        </style>
    </head>
    <body style="margin: 0 auto; max-width: 800px;">
<h1>WebSocket Test</h1>
<input id="message" class="input" type="text"/><input id="post" class="input" type="button" value="Send" />
<section id="content"><br/></section>
<script>
    document.getElementById('message').setAttribute('disabled', 'disabled')
    var ws = new WebSocket(location.href.replace(/^http/, 'ws')+'&ws');
    ws.onerror = function() { ws.close(); }
    ws.onclose = function() { document.getElementById('message').setAttribute('disabled', 'disabled') }
    ws.onopen = function() {
        var message_input=document.getElementById('message')
        message_input.removeAttribute('disabled');
        ws.onmessage = function(msg) {
            if (msg.data.length) {
                var data = JSON.parse(msg.data);
                var content_section = document.getElementById('content');
                template = document.createElement('div');
                template.innerHTML =
                    '<span class="time">'+data.time+':&nbsp;</span>'+
                    '<span class="message">'+data.message+'</span><br/>';
                content_section.prepend(template);
            }
        }
        message_input.onkeydown = function(e) {
            if (e.keyCode == 13) document.getElementById('post').click();
        };
        document.getElementById('post').onclick = function() {
            var message_input=document.getElementById('message');
            ws.send(message_input.value);
            message_input.value='';
        };
    };
</script>    """
    print """
    </body>
</html>
    """
    print


def print_page():
    print 'Content-Type: text/html'
    print
    print """
<!DOCTYPE html>
<html>
    <head>
        <title>Tiny PostgresSQL Websocket API</title>
    </head>
    <body style="margin: 0 auto; max-width: 800px;">
        <h2><br/>Tiny PostgresSQL Websocket API</h2>
        <a href="tinypgwebsocketapi.py?wstest">WebSocket communication</a>
        <br/>
        <a href="printenv">CGI environment</a>
        <br/>
        <a href="traceviewer.html">Trace file viewer</a>
        <br/>
        <br/>
        <div style="background-color: #f2f2f2; padding: 15px;">
        <code>
        %s
        </code>
        </div>
        <br/>
        <br/>
    </body>
</html>
    """ % unicode(sorted(os.environ.items())).decode('utf8').replace('(','<br/>(')[6:-1]
    print


def pg_datetime():
    try:
        global pg_conn
        if pg_conn is None:
            pg_conn = psycopg2.connect(dbname='postgres', user='ralac')
            pg_conn.autocommit = True
        cur = pg_conn.cursor()
        cur.execute(
        """
SELECT to_char(current_timestamp, 'YYYY-MM-DD HH24:MI:SS');
        """
        )
        rows = cur.fetchall()
        cur.close()
        # pg_conn.rollback()
        for row in rows:
            result = row[0]
        return result
    except psycopg2.OperationalError as e:
        return str(e)
    except e:
        return str(e)


pg_conn = None
cgi_qs = []
try:
    session_id()
    if os.environ.__contains__('QUERY_STRING'):
        cgi_qs = os.environ['QUERY_STRING'].split('&')
    if os.environ['REQUEST_METHOD'] == "GET":
        if cgi_qs.__contains__('ws'):
            websocket_test()
        elif cgi_qs.__contains__('trace'):
            eventsource_trace()
        elif cgi_qs.__contains__('print_env'):
            print_env()
        elif cgi_qs.__contains__('wstest'):
            print_wstest()
        else:
            print_page()
except:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    import traceback
    log_message(''.join(traceback.format_exception(exc_type, exc_value,exc_traceback)))
