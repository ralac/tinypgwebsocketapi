#include <fcgi_stdio.h>
#include <sys/select.h>
#include <stdarg.h>

#include "websocket_parser.h"
#include "tpgwsapi_pg.h"

int ws_active = 0;
int echo = 1;
int trace = 0;

char *in_buf = NULL;
char *out_buf = NULL;
uint64_t remaining = 0;
uint64_t out_buf_to_send = 0;
char *reading_buffer_size_str = NULL;
char *fd_polling_timeout_str = NULL;
size_t reading_buffer_size = 0;
long fd_polling_timeout = 0;

static int tpgwsapi_trace(const char *format, ...) {
    va_list strs_;

    if (trace) {
        va_start( strs_, format );
        return FCGI_vfprintf(FCGI_stderr, format, strs_);
    } else {
        return 0;
    }
}

static int tpgwsapi_error(const char *format, ...){
    va_list strs_;

    va_start( strs_, format );
    return FCGI_vfprintf(FCGI_stderr, format, strs_);
}

static int tpgwsapi_output(const char *format, ...) {
    va_list strs_;

    va_start( strs_, format );
    return FCGI_vfprintf(FCGI_stdout, format, strs_);
}

static size_t tpgwsapi_get_content(char *content, size_t size) {
    return read(
        fileno(FCGI_stdin),
        content,
        size
    );
}

void websocket_send_message(void *msg, uint64_t len, unsigned char opcode) {
    out_buf_to_send = websocket_server_wrap(out_buf, msg, len, opcode, 1, 1, 0);
    if (write(
            fileno(FCGI_stdout),
            out_buf,
            out_buf_to_send
    ) < 0) {
        tpgwsapi_trace("Write() failed: %s\n", strerror(errno));
        ws_active = 0;
    }
}

static void websocket_on_unwrapped(void *udata, void *msg, uint64_t len,
                                   char first, char last, char opcode,
                                   unsigned char rsv){
    if (opcode == 1) { /* text */
        if (echo) {
            websocket_send_message(msg, len, 1);
            if (trace) {
                char *trace_msg = malloc(len + 1);
                memcpy(trace_msg, msg, len);
                *(trace_msg+len) = '\0';
                tpgwsapi_trace("received protocol message: %s\n", trace_msg);
                free(trace_msg);
            }
        }
    }
}

static void websocket_on_protocol_ping(void *udata, void *msg, uint64_t len){
    websocket_send_message(msg, len, 10);
    tpgwsapi_trace("received protocol PING: %s\n", msg);
}

static void websocket_on_protocol_pong(void *udata, void *msg, uint64_t len){
    tpgwsapi_trace("received protocol PONG: %s\n", msg);
}

static void websocket_on_protocol_close(void *udata) {
    ws_active = 0;
    tpgwsapi_trace("websocket protocol close\n");
}

static void websocket_on_protocol_error(void *udata) {
    ws_active = 0;
    tpgwsapi_trace("websocket protocol error\n");
}

int tpgwsapi_read_stdin() {
    tpgwsapi_trace("websocket_handler reading...\n");
    //consume
    uint64_t to_consume;
    uint64_t to_read;
    size_t num_blocks;
    to_read = reading_buffer_size - remaining;
    tpgwsapi_trace("websocket_handler to_read=%d\n", to_read);
    to_consume = read(
        fileno(FCGI_stdin),
        in_buf + remaining,
        (size_t)to_read
    );
    tpgwsapi_trace("websocket_handler to_consume=%d\n", to_consume);
    if (to_consume < 0) {
        tpgwsapi_trace("Read() failed: %s\n", strerror(errno));
        return -1;
    }
    if (to_consume > 0) {
        to_consume = to_consume + remaining;
        tpgwsapi_trace("websocket_handler before consume remaining=%d\n", remaining);
        remaining = websocket_consume(in_buf, to_consume, NULL, 1);
        tpgwsapi_trace("websocket_handler after consume  remaining=%d\n", remaining);
    }
    return 0;
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
        FD_SET(fileno(FCGI_stdin), &rfds);

        FD_ZERO(&efds);
        FD_SET(fileno(FCGI_stdin), &efds);

        retval = select(1, &rfds, NULL, &efds, &tv);
        /* Don't rely on the value of tv now! */

        if (retval < 0) {
            tpgwsapi_trace("Select() failed: %s\n", strerror(errno));
            break;
        } else if (retval) {
            if (FD_ISSET(fileno(FCGI_stdin), &rfds)) {
                tpgwsapi_read_stdin();
            } else if (FD_ISSET(fileno(FCGI_stdin), &efds)) {
                if (tpgwsapi_trace("File stdin error: %s\n", strerror(errno)) < 0)
                    break;
            }
        } else {
            char *my_ping_msg = "Trying to keep connection alive.";
            websocket_send_message(my_ping_msg, strlen(my_ping_msg)+1, 9);
        }
    }
    if (in_buf == NULL) {
        tpgwsapi_error("[tpgwsapi] Out of memory\n");
    } else {
        free(in_buf);
    }
    if (out_buf == NULL) {
        tpgwsapi_error("[tpgwsapi] Out of memory\n");
    } else {
        free(out_buf);
    }
}

int main()
{
    char *trace_env = getenv("TRACE");

    pg_url              = getenv("PG_URL");
    pg_function         = getenv("PG_FUNCTION");
    tpgwsapi_service    = getenv("TPGWSAPI_SERVICE");

    if (trace_env != NULL && !strcmp(trace_env, "enabled")) trace = 1;

    fprintf(stderr, "pg_connect pid: %d\n", getpid());
    fprintf(stderr, "pg_connect [TRACE=%s]\n", trace_env);
    fprintf(stderr, "pg_connect [TPGWSAPI=%s]\n", getenv("TPGWSAPI"));
    fprintf(stderr, "pg_connect [PG_URL=%s]\n", pg_url);
    fprintf(stderr, "pg_connect [PG_FUNCTION=%s]\n", pg_function);
    fprintf(stderr, "pg_connect [TPGWSAPI_SERVICE=%s]\n", tpgwsapi_service);
    fflush(stderr);

    pg_connect();
    while (FCGI_Accept() >= 0)
    {
        /**
        Websocket process must run as CGI because Lighttpd removes
        handshake header `Connection`.
        **/

        pg_setup_headers();
        pg_function_handler(trace);
        if (is_websocket) {
            setvbuf(FCGI_stdin, NULL, _IONBF, 0);
            setvbuf(FCGI_stdout, NULL, _IONBF, 0);
            setvbuf(FCGI_stderr, NULL, _IONBF, 0);
            websocket_handler();
        }
        pg_release_headers();
    }
    pg_disconnect();
    return 0;
}
