#!/usr/bin/python -u

import os
import sys
import psycopg2

# Try to connect

try:
    conn=psycopg2.connect("dbname='api_test' user='ralac'")
except:
    print "I am unable to connect to the database."

cur = conn.cursor()
try:
    test = """
('CGI_SERVICE', 'unique'), 
('CONTENT_LENGTH', '0'), 
('DOCUMENT_ROOT', '/srv/www/htdocs'), 
('FD_POLLING_TIMEOUT', '5'), 
('GATEWAY_INTERFACE', 'CGI/1.1'), 
('HTTP_ACCEPT', 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8'), 
('HTTP_ACCEPT_ENCODING', 'gzip, deflate, br'), 
('HTTP_ACCEPT_LANGUAGE', 'pt-PT,pt;q=0.9,en-US;q=0.8,en;q=0.7'), 
('HTTP_CACHE_CONTROL', 'max-age=0'), 
('HTTP_CONNECTION', 'Keep-Alive'), 
('HTTP_COOKIE', 'session_idxxx=588af49696ac11e88a9fe839dff3d4fd; session_id=63fbbd4c96ac11e88664e839dff3d4fd'), 
('HTTP_HOST', 'localhost:8090'), 
('HTTP_MAX_FORWARDS', '10'), 
('HTTP_UPGRADE_INSECURE_REQUESTS', '1'), 
('HTTP_USER_AGENT', 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36'), 
('HTTP_X_ARR_LOG_ID', '0ee76830-b2f7-46c0-8d0c-7cff9121fe6f'), 
('HTTP_X_ARR_SSL', '4096|256|C=US, O=Let''s Encrypt, CN=Let''s Encrypt Authority X3|CN=ralac.openchain.pt'), 
('HTTP_X_FORWARDED_FOR', '213.22.22.34:1025'), 
('HTTP_X_ORIGINAL_URL', '/local/app/api/tinypgwebsocketapi.py?a=1&b=2&c=dskjhhsd'), 
('PG_FUNCTION', 'api.handler'), 
('PG_URL', 'postgresql:///api_test?user=ralac&application_name=api_test'), 
('PYTHONUNBUFFERED', '1'), 
('QUERY_STRING', 'a=1&b=2&c=dskjhhsd'), 
('READING_BUFFER_SIZE', '16384'), 
('REDIRECT_STATUS', '200'), 
('REMOTE_ADDR', '127.0.0.1'), 
('REMOTE_PORT', '40575'), 
('REQUEST_METHOD', 'GET'), 
('REQUEST_SCHEME', 'http'), 
('REQUEST_URI', '/app/api/tinypgwebsocketapi.py?a=1&b=2&c=dskjhhsd'), 
('SCRIPT_FILENAME', '/srv/www/htdocs/app/api/tinypgwebsocketapi.py'), 
('SCRIPT_NAME', '/app/api/tinypgwebsocketapi.py'), 
('SERVER_ADDR', '127.0.0.1'), 
('SERVER_NAME', 'localhost'), 
('SERVER_PORT', '8090'), 
('SERVER_PROTOCOL', 'HTTP/1.1'), 
('SERVER_SOFTWARE', 'lighttpd/1.4.49-devel-lighttpd-1.4.49'), 
('SYSTEMROOT', 'C:\\WINDOWS'), 
('TRACE', 'enabled'), 
('TRACE_FILE', '/var/log/lighttpd/tinypgwebsocketapi.log'), 
('WINDIR', 'C:\\WINDOWS')
    """
    tt = unicode(tuple(sorted(os.environ.items()))).replace('(','(VALUES ',1)
    xx = '%s' % tt
    cur.execute( \
    """
SELECT * FROM api.handler(
    %d,
    array(SELECT ROW(key,value)::api.dict FROM (VALUES %s) AS T(key, value)),
    NULL,
    NULL
);
    """ % (
        os.getpid(),
        # unicode(sorted(os.environ.items())).decode('utf8')[1:-1]
        test
    ))
except:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    import traceback
    print ''.join(traceback.format_exception(exc_type, exc_value,exc_traceback))

rows = cur.fetchall()
for row in rows:
    index = 0
    for col in row:
        if col is None:
            print '%d is None' % index
        else:
            print '%d: %s' % (index, col)
        index += 1
