#!/usr/bin/python -u

README_MARKDOWN="""
See new repe (here)[https://github.com/ralac/tinypgwebsocketapi]
"""

# Configuration
READING_BUFFER_SIZE = 16384
FD_POLLING_TIMEOUT=5
TRACE=True
TRACE_FILE='tinypgwebapi.log'

import os
import sys
import json
import psycopg2
import subprocess
self_pid=os.getpid()

SYS_PAGE_SIZE=-1
def memory_usage_ps():
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
        trace_file = open(TRACE_FILE, 'a')
        trace_file.write(msg+'\n')
        trace_file.flush()
        trace_file.close()


# log_message('memory (common):'+memory_usage_ps())

def websocket():
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
    except (Cookie.CookieError, KeyError):
        session_id = str(uuid.uuid1()).replace('-', '')
        print 'Set-Cookie: session_id=%s' % session_id


def eventsource_trace():
    print 'Content-type: text/event-stream'
    print
    proc = subprocess.Popen(['tail', '-f', '-n', '128', TRACE_FILE], stdout=subprocess.PIPE)
    id = 0
    while True:
        try:
            line = proc.stdout.readline()
            id += 1
            print 'data: ' + line.rstrip()
            print 'id: ' + str(id)
            print 'retry: %d' % (10 * 60 * 60 * 100)  # 10 hours
            print
            sys.stdout.flush()
        except:
            break


def trace_viewer():
    import cgi
    print 'Content-Type: text/html'
    print
    print """
<!DOCTYPE html>
<html>
    <head>
        <title>Tiny PostgresSQL Web API</title>
        <style>
            h1 {
                margin-left: 20px;
            }
            html, body {
                height: 100%;
                margin: 0px;
                padding: 0px;
            }
            .container {
                height: 100%;
                width: 100%;
                margin: 0px;
                padding: 0px;
                table-layout:fixed;
            }
            .container tr, .container td {
                margin: 0px;
                padding: 0px;
                border-width: 0px;
            }
            .row1px {
                height: 1px;
            }
            .rowauto {
                height: auto;
            }
            textarea {
                display: block;
                margin: 0px auto;
                padding: 0px;
                height: 95%;
                width: 100%;
            }
        </style>
        <script>
            var eventSource = null;

            function addToLog(data) {
                var currentdate = new Date();

                var str_datetime =
                    currentdate.getFullYear() +
                    '-' +
                    String(currentdate.getMonth()+1+100).substr(-2) +
                    '-' +
                    String(currentdate.getDate()+100).substr(-2) +
                    ' ' +
                    String(currentdate.getHours()+100).substr(-2) +
                    ':' +
                    String(currentdate.getMinutes()+100).substr(-2) +
                    ':' +
                    String(currentdate.getSeconds()+100).substr(-2);

                logBox.value = '*[' + str_datetime + '] ' + data + '\\n' + logBox.value.substr(0, 65535);
                logBox.scrollTop = 0;
            }

            document.addEventListener('DOMContentLoaded', function(){
                logBox = document.getElementById('log');

                eventSource  = new EventSource(location.href.replace('?trace_viewer', '?trace'));
                eventSource.onopen = function() {
                    logBox.value = "";
                    addToLog('onopen (readyState = ' + eventSource.readyState + ')');
                }
                eventSource.onmessage = function(event) {
                    addToLog(event.data);
                }
                eventSource.onerror = function(event) {
                    addToLog('onerror (readyState = ' + eventSource.readyState + ')');
                }
            });

        </script>
    </head>
    <body style="margin: 0px auto; max-width: 800px;">
        <table class="container">
            <tr class="row1px">
                <td>
                    <h1>Trave Viewer</h1>
                </td>
            </tr>
            <tr class="rowauto">
                <td>
                    <textarea id="log" readonly>
                    </textarea>
                </td>
            </tr>
            <tr class="row1px">
                <td style="font-size=xx-small; padding-bottom: 15px;">
                    *Browser receiving time.
                </td>
            </tr>
        </table>
    </body>
</html>
    """
    print


def print_env():
    import cgi
    print 'Content-Type: text/html'
    print
    print """
<!DOCTYPE html>
<html>
    <head>
        <title>Tiny PostgresSQL Web API</title>
        <style>
            h3 {
                margin-left: 20px;
            }
            dl {
                margin: 0.5em;
                font-size: small;
            }
            dt {
                float: left;
                clear: left;
                width: 300px;
                text-align: right;
                font-weight: bold;
                word-wrap: break-word;
                font-size: small;
                min-height: 18px;
            }
            dd {
                margin: 0 0 0 300px;
                padding: 0 0 0 0.5em;
                word-wrap: break-word;
                font-size: small;
                min-height: 18px;
                &:empty {
                    content: 'Empty';
                }
            }
            dt:nth-child(4n+1), dt:nth-child(4n+1) + dd {
                background-color: #f2f2f2;
            }
            dt::empty {
                content: "&lt;empty&ngt;";
            }
        </style>
    </head>
    <body style="margin: 0 auto; max-width: 800px;">
    """
    cgi.print_environ()
    print """
    </body>
</html>
    """
    print


def print_wstest():
    print 'Content-Type: text/html'
    print
    print """
<!DOCTYPE html>
<html>
    <head>
        <title>Tiny PostgresSQL Web API</title>
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


def print_readme():
    import markdown
    readme_extensions = [
        'markdown.extensions.extra',
        'markdown.extensions.nl2br',
        'markdown.extensions.codehilite',
        'markdown.extensions.toc',
    ]
    readme_extensions_configs = {
        'markdown.extensions.extra': {},
        'markdown.extensions.nl2br': {},
        'markdown.extensions.codehilite': {
            'pygments_style': 'autumn'
        },
        'markdown.extensions.toc': {
            'baselevel': 2
        },
    }
    source_str=''
    source_str+='```\n'
    source_str+=':::python\n'
    skip=False
    with open(__file__, 'r') as f:
        for line in f:
            if skip:
                if line.rstrip() == '"""':
                    skip = False
            else:
                source_str += line.rstrip()+'\n'
                if line.rstrip() == 'README_MARKDOWN="""':
                    source_str += '... README_MARKDOWN variable source bellow\n"""\n'
                    skip = True
        f.close()
    source_str+='```\n'
    outstr=markdown.markdown(README_MARKDOWN+source_str, extensions=readme_extensions, extension_configs=readme_extensions_configs)
    print 'Content-Type: text/html'
    print
    print """
<!DOCTYPE html>
<html>
    <head>
        <title>Tiny PostgresSQL Web API</title>
        <link rel="stylesheet" href="%s">
        <style>
    """ % 'https://cdnjs.cloudflare.com/ajax/libs/github-markdown-css/2.10.0/github-markdown.min.css'
    print subprocess.check_output(['pygmentize', '-S', 'autumn', '-f', 'html'])
    print """
        </style>
    </head>
    <body style="margin: 0 auto; max-width: 800px;"><article class="markdown-body">
    <h2><br/>Tiny PostgresSQL Web API</h2>
    """
    print
    print outstr
    source_str='\n###README_MARKDOWN variable source\n'
    source_str+='```\n'
    source_str+=':::md\n'
    source_str+=README_MARKDOWN.replace('```','\\`\\`\\`')+'\n'
    source_str+='```\n'
    outstr=markdown.markdown(source_str, extensions=readme_extensions, extension_configs=readme_extensions_configs).replace('\\`\\`\\`','```')
    # print outstr
    print """
    </article></body>
</html>
    """
    print


conn = None
def pg_datetime():
    try:
        global conn
        if conn is None:
            conn = psycopg2.connect(dbname='postgres', user='ralac')
            conn.autocommit = True
        cur = conn.cursor()
        cur.execute(
        """
SELECT to_char(current_timestamp, 'YYYY-MM-DD HH24:MI:SS');
        """
        )
        rows = cur.fetchall()
        cur.close()
        # conn.rollback()
        for row in rows:
            result = row[0]
        return result
    except psycopg2.OperationalError as e:
        return str(e)
    except e:
        return str(e)


cgi_qs = []
try:
    session_id()
    if os.environ.__contains__('QUERY_STRING'):
        cgi_qs = os.environ['QUERY_STRING'].split('&')
    if os.environ['REQUEST_METHOD'] == "GET":
        if cgi_qs.__contains__('ws'):
            websocket()
        elif cgi_qs.__contains__('trace'):
            eventsource_trace()
        elif cgi_qs.__contains__('trace_viewer'):
            trace_viewer()
        elif cgi_qs.__contains__('print_env'):
            print_env()
        elif cgi_qs.__contains__('wstest'):
            print_wstest()
        elif cgi_qs.__contains__('readme'):
            print_readme()
        else:
            print_readme()
except:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    import traceback
    log_message(''.join(traceback.format_exception(exc_type, exc_value,exc_traceback)))
