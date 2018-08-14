#include <fcgi_stdio.h>
#include <sys/select.h>

#include "websocket_parser.h"
#include "tpgwsapi.h"

char *reading_buffer_size_str;
char *fd_polling_timeout_str;
size_t reading_buffer_size = 0;
long fd_polling_timeout = 0;

int ws_active = 0;

char *in_buf = NULL;
char *out_buf = NULL;
uint64_t remaining = 0;
uint64_t out_buf_to_send = 0;


#ifndef TPGWSAPI_FCGI
void send_message(void *msg, uint64_t len, unsigned char opcode) {
    out_buf_to_send = websocket_server_wrap(out_buf, msg, len, opcode, 1, 1, 0);
    if (write(fileno(stdout), out_buf, out_buf_to_send) < 0) {
        if (trace)
            fprintf(stderr, "[tpgwsapi-trace] Write() failed: %s\n", strerror(errno));
        ws_active = 0;
    }
}

static void websocket_on_unwrapped(void *udata, void *msg, uint64_t len,
                                   char first, char last, char opcode,
                                   unsigned char rsv){
    if (opcode == 1) { /* text */
        send_message(msg, len, 1);
        if (trace) {
            char *trace_msg = malloc(len + 1);
            memcpy(trace_msg, msg, len);
            *(trace_msg+len) = '\0';
            fprintf(stderr, "[tpgwsapi-trace] received protocol message: %s\n", trace_msg);
            free(trace_msg);
        }
    }
}

static void websocket_on_protocol_ping(void *udata, void *msg, uint64_t len){
    send_message(msg, len, 10);
    if (trace)
        fprintf(stderr, "[tpgwsapi-trace] received protocol PING: %s\n", msg);
}

static void websocket_on_protocol_pong(void *udata, void *msg, uint64_t len){
    if (trace)
        fprintf(stderr, "[tpgwsapi-trace] received protocol PONG: %s\n", msg);
}

static void websocket_on_protocol_close(void *udata) {
    ws_active = 0;
    if (trace)
        fprintf(stderr, "[tpgwsapi-trace] websocket protocol close\n");
}

static void websocket_on_protocol_error(void *udata) {
    ws_active = 0;
    if (trace)
        fprintf(stderr, "[tpgwsapi-trace] websocket protocol error\n");
}

void websocket_handler() {
    reading_buffer_size_str = getenv("READING_BUFFER_SIZE");
    if (reading_buffer_size_str != NULL) {
        reading_buffer_size = atol(reading_buffer_size_str);
    }
    if (reading_buffer_size == 0) {
        reading_buffer_size = 65536;
    }
    fd_polling_timeout_str = getenv("FD_POLLING_TIMEOUT");
    if (fd_polling_timeout_str != NULL) {
        fd_polling_timeout = atol(fd_polling_timeout_str);
    }
    if (fd_polling_timeout == 0) {
        fd_polling_timeout = 5;
    }

    fd_set rfds;
    fd_set efds;
    struct timeval tv;
    int retval;

    /* Wait up to "FD_POLLING_TIMEOUT" seconds. */

    tv.tv_sec = fd_polling_timeout;
    tv.tv_usec = 0;

    in_buf = malloc(reading_buffer_size);
    out_buf = malloc(reading_buffer_size);
    ws_active = ( in_buf == NULL || out_buf == NULL ? 0 : 1);

    while (ws_active) {
        /* Watch stdin to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(fileno(stdin), &rfds);

        FD_ZERO(&efds);
        FD_SET(fileno(stdin), &efds);

        retval = select(1, &rfds, NULL, &efds, &tv);
        /* Don't rely on the value of tv now! */

        if (retval < 0) {
            if (trace)
                fprintf(stderr, "[tpgwsapi-trace] Select() failed: %s\n", strerror(errno));
            break;
        } else if (retval) {
            if (FD_ISSET(fileno(stdin), &rfds)) {
    fprintf(stderr, "OK reading...\n");
                //consume
                uint64_t to_consume;
                uint64_t to_read;
                to_read = reading_buffer_size - remaining;
    fprintf(stderr, "OK to_read=%d\n", to_read);
                to_consume = read(fileno(stdin), in_buf + remaining, to_read);
    fprintf(stderr, "OK to_consume=%d\n", to_consume);
                if (to_consume < 0) {
                    if (trace)
                        fprintf(stderr, "[tpgwsapi-trace] Read() failed: %s\n", strerror(errno));
                    break;
                }
                if (to_consume > 0) {
                    to_consume = to_consume + remaining;
    fprintf(stderr, "OK (before) remaining=%d\n", remaining);
                    remaining = websocket_consume(in_buf, to_consume, NULL, 1);
    fprintf(stderr, "OK (after) remaining=%d\n", remaining);
                }
            } else if (FD_ISSET(fileno(stdin), &efds)) {
                if (trace)
                    fprintf(stderr, "[tpgwsapi-trace] File stdin error: %s\n", strerror(errno));
                break;
            }
        } else {
            char *my_ping_msg = "Trying to keep connection alive.";
            send_message(my_ping_msg, strlen(my_ping_msg)+1, 9);
        }
    }
    if (in_buf == NULL) {
        fprintf(stderr, "[tpgwsapi] Out of memory\n");
    } else {
        free(in_buf);
    }
    if (out_buf == NULL) {
        fprintf(stderr, "[tpgwsapi] Out of memory\n");
    } else {
        free(out_buf);
    }
}

#endif // TPGWSAPI_FCGI

int main()
{
    setvbuf(stderr, NULL, _IONBF, 0);
#ifdef TPGWSAPI_FCGI
    pg_connect();
    while (FCGI_Accept() >= 0)
    {
        pg_function_handler();
    }
    pg_disconnect();
#else
    pg_connect();
    pg_function_handler();
    if (is_websocket) {
        setvbuf(stdin, NULL, _IONBF, 0);
        setvbuf(stdout, NULL, _IONBF, 0);
        websocket_handler();
    }
    pg_disconnect();
    return 0;
#endif // TPGWSAPI_FCGI
}
