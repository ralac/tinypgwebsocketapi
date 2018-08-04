#!/bin/bash
echo "Cache-Control: no-cache"
echo "Connection: keep-alive"
echo "Content-Type: text/event-stream"
#
# ---> chunked is not supported
# echo "Transfer-Encoding: chunked"
#
echo ""
stdbuf -o 0 /usr/bin/tail -f -n 512 "$TRACE_FILE" | stdbuf -i 0 -o 0 /usr/bin/awk '{
    system("");
    print "data: " $0;
    print "id: " NR;
    print "retry: 3600000";
    print "";
    fflush(stdout);
    system("");
}'