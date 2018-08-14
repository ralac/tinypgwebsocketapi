#include "../config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "libpq-fe.h"

#define PG_SELECT_MASK "\n\
SELECT *\n\
  FROM %s(\n\
    %d,\n\
    array(\n\
      SELECT\n\
          ROW(key,value)::api.dict\n\
        FROM (VALUES%s\n\
             ) AS T(key, value)\n\
    ),\n\
    %s%s%s,\n\
    NULL,\
    %s)"

static int tpgwsapi_trace(const char *format, ...);
static int tpgwsapi_error(const char *format, ...);
static int tpgwsapi_output(const char *format, ...);
static size_t tpgwsapi_get_content(char *content, size_t size);

char *pg_url;
char *pg_function;
char *tpgwsapi_service;

PGconn *conn = NULL;
char *sql_cmd = NULL;

int client_id;
int is_eventsource;
int is_websocket;

#ifndef HAVE_ASPRINTF
int vasprintf(char **ret, const char *format, va_list ap)
{
    va_list ap2;
    int len= 100;        /* First guess at the size */
    if ((*ret= (char *)malloc(len)) == NULL) return -1;
    while (1)
    {
        int nchar;
        va_copy(ap2, ap);
        nchar= vsnprintf(*ret, len, format, ap2);
        if (nchar > -1 && nchar < len) return nchar;
        if (nchar > len)
            len= nchar+1;
        else
            len*= 2;
        if ((*ret= (char *)realloc(*ret, len)) == NULL)
        {
            free(*ret);
            return -1;
        }
    }
}

int asprintf(char **ret, const char *format, ...)
{
    va_list ap;
    int nc;
    va_start (ap, format);
    nc= vasprintf(ret, format, ap);
    va_end(ap);
    return nc;
}
#endif /*HAVE_ASPRINTF*/

int pg_connect() {
    PGresult   *res;

    pg_url              = getenv("PG_URL");
    pg_function         = getenv("PG_FUNCTION");
    tpgwsapi_service    = getenv("TPGWSAPI_SERVICE");

    tpgwsapi_trace("pg_connect [PG_URL=%s]\n", pg_url);
    tpgwsapi_trace("pg_connect [PG_FUNCTION=%s]\n", pg_function);
    tpgwsapi_trace("pg_connect [TPGWSAPI_SERVICE=%s]\n", tpgwsapi_service);

    if (conn == NULL && pg_url != NULL)
    {
        /* Make a connection to the database */
        conn = PQconnectdb(pg_url);

        /* Check to see that the backend connection was successfully made */
        if (PQstatus(conn) != CONNECTION_OK)
        {
            tpgwsapi_error("[tpgwsapi] Connection to database failed: %s\n", PQerrorMessage(conn));
            PQfinish(conn);
            return -1;
        }

        /* Set always-secure search path, so malicous users can't take control. */
        res = PQexec(conn, "SELECT pg_catalog.set_config('search_path', '', false);");
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            tpgwsapi_error("[tpgwsapi] SET failed: %s\n", PQerrorMessage(conn));
            PQclear(res);
            PQfinish(conn);
            return -2;
        }

        /*
         * Should PQclear PGresult whenever it is no longer needed to avoid memory
         * leaks
         */
        PQclear(res);
        return 0;
    }
}

void pg_disconnect() {
    if (conn != NULL)
    {
        /* close the connection to the database and cleanup */
        PQfinish(conn);
    }
}

void pg_env_item(char **sql_str, char *item_name) {
    char    *item_value;
    char	*item_escaped;
    char    *item_empty = "";
    char    *sql_str_new = NULL;
    char    *mask;
    size_t	item_value_len;
    size_t  item_escaped_len;
    int     error;

    item_value = getenv(item_name);
    if (item_value == NULL) {
        if (*sql_str == NULL)
            if (asprintf(&sql_str_new, "\n('%s', NULL)", item_name) < 0) {
                tpgwsapi_error("[tpgwsapi] asprintf failed\n");
            } else {
                *sql_str = sql_str_new;
            }
        else {
            if (asprintf(&sql_str_new, "%s, \n('%s', NULL)", *sql_str, item_name) < 0) {
                tpgwsapi_error("[tpgwsapi] asprintf failed\n");
            } else {
                free(*sql_str);
                *sql_str = sql_str_new;
            }
        }
    } else {
        item_value_len = strlen(item_value);
        if (item_value_len) {
            item_escaped = malloc(item_value_len * 2 + 256);
            if (!item_escaped) {
                tpgwsapi_error("[tpgwsapi] Out of memory\n");
                exit(EXIT_FAILURE);
            }
            item_escaped_len = PQescapeStringConn(conn, item_escaped, item_value, item_value_len, &error);
            if (error != 0) {
                tpgwsapi_error("[tpgwsapi] PQescapeStringConn failed: %s\n", PQerrorMessage(conn));
            }
        } else {
            item_escaped = "";
        }
        if (*sql_str == NULL)
            if (asprintf(&sql_str_new, "\n('%s', '%s')", item_name, item_escaped) < 0) {
                tpgwsapi_error("[tpgwsapi] asprintf failed\n");
            } else {
                *sql_str = sql_str_new;
            }
        else {
            if (asprintf(&sql_str_new, "%s, \n('%s', '%s')", *sql_str, item_name, item_escaped) < 0) {
                tpgwsapi_error("[tpgwsapi] asprintf failed\n");
            } else {
                free(*sql_str);
                *sql_str = sql_str_new;
            }
        }

        if (item_escaped != NULL && item_value_len)
            free(item_escaped);
    }
}

void pg_function_handler(int with_trace) {
    PGresult *res;
    char *cgi_environment = NULL;
    unsigned char *request_body = NULL;
    size_t content_length = 0;
    size_t request_body_length = 0;
    char *method;
    char *content_length_env;
    char *content = NULL;
    int i;

    tpgwsapi_trace("New pg_function_handler call\n");

    /* CGI 1.1 Standard environment variables       */
    pg_env_item(&cgi_environment, "AUTH_TYPE");
    pg_env_item(&cgi_environment, "CONTENT_LENGTH");
    pg_env_item(&cgi_environment, "CONTENT_TYPE");
    pg_env_item(&cgi_environment, "GATEWAY_INTERFACE");
    pg_env_item(&cgi_environment, "HTTP_*");
    pg_env_item(&cgi_environment, "PATH_INFO");
    pg_env_item(&cgi_environment, "PATH_TRANSLATED");
    pg_env_item(&cgi_environment, "QUERY_STRING");
    pg_env_item(&cgi_environment, "REMOTE_ADDR");
    pg_env_item(&cgi_environment, "REMOTE_HOST");
    pg_env_item(&cgi_environment, "REMOTE_IDENT");
    pg_env_item(&cgi_environment, "REMOTE_USER");
    pg_env_item(&cgi_environment, "REQUEST_METHOD");
    pg_env_item(&cgi_environment, "SCRIPT_NAME");
    pg_env_item(&cgi_environment, "SERVER_NAME");
    pg_env_item(&cgi_environment, "SERVER_PORT");
    pg_env_item(&cgi_environment, "SERVER_PROTOCOL");
    pg_env_item(&cgi_environment, "SERVER_SOFTWARE");
    /*
    --------------------------------------------------
    ** HTTP Headers commonly used                   **
    --------------------------------------------------
                                                    */
    pg_env_item(&cgi_environment, "HTTP_ACCEPT");
    pg_env_item(&cgi_environment, "HTTP_USER_AGENT");
    pg_env_item(&cgi_environment, "HTTP_REFERER");
    pg_env_item(&cgi_environment, "HTTP_COOKIE");
    /*
    --------------------------------------------------
    ** HTTP Headers for web sockets                 **
    --------------------------------------------------
                                                    */
    pg_env_item(&cgi_environment, "HTTP_HOST");
    pg_env_item(&cgi_environment, "HTTP_UPGRADE");
    pg_env_item(&cgi_environment, "HTTP_CONNECTION");
    pg_env_item(&cgi_environment, "HTTP_SEC_WEBSOCKET_KEY");
    pg_env_item(&cgi_environment, "HTTP_ORIGIN");
    pg_env_item(&cgi_environment, "HTTP_SEC_WEBSOCKET_PROTOCOL");
    pg_env_item(&cgi_environment, "HTTP_SEC_WEBSOCKET_VERSION");
    /*
    --------------------------------------------------
    ** TPGWSAPI environment variables               **
    --------------------------------------------------
                                                    */
    pg_env_item(&cgi_environment, "TRACE");
    pg_env_item(&cgi_environment, "PG_URL");
    pg_env_item(&cgi_environment, "PG_FUNCTION");
    pg_env_item(&cgi_environment, "TPGWSAPI_SERVICE");
    pg_env_item(&cgi_environment, "TPGWSAPI");
    /*
    --------------------------------------------------
    ** TPGWSAPI websockets environment variables    **
    --------------------------------------------------
                                                    */
    pg_env_item(&cgi_environment, "READING_BUFFER_SIZE");
    pg_env_item(&cgi_environment, "FD_POLLING_TIMEOUT");

    method = getenv("REQUEST_METHOD");
    if (method != NULL && !strcmp(method, "POST")) {
        content_length_env = getenv("CONTENT_LENGTH");
        if (content_length_env != NULL) {
            content_length = strtol(content_length_env, NULL, 10);
        }
        if (content_length) {
            content = malloc(content_length + 1);
            if (!content) {
                tpgwsapi_error("[tpgwsapi] Out of memory\n");
                exit(EXIT_FAILURE);
            }
            if (tpgwsapi_get_content(content, content_length) < content_length) {
                tpgwsapi_error("[tpgwsapi] PQescapeByteaConn failed: %s\n", PQerrorMessage(conn));
            } else {
                *(content + content_length) = '\0';
                request_body = PQescapeByteaConn(conn, content, content_length, &request_body_length);
                if (request_body == NULL) {
                    tpgwsapi_error("[tpgwsapi] PQescapeByteaConn failed: %s\n", PQerrorMessage(conn));
                }
            }
        }
    }

    if (asprintf(
                &sql_cmd,
                PG_SELECT_MASK,
                pg_function,
                getpid(),
                (cgi_environment == NULL ? "NULL" : cgi_environment),
                (request_body == NULL ? "" : "'"),
                (request_body == NULL ? "NULL" : (char *)request_body),
                (request_body == NULL ? "" : "'::bytea"),
                (with_trace ? "true" : "false")
            ) < 0) {
        tpgwsapi_error("[tpgwsapi] asprintf failed\n");
    } else {
        tpgwsapi_trace("sql_cmd=%s\n", sql_cmd);

        res = PQexec(conn, sql_cmd);
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            tpgwsapi_error("[tpgwsapi] SELECT failed: %s\n", PQerrorMessage(conn));
            PQclear(res);
        } else {
            if (PQntuples(res) == 1) {
                client_id = 0;
                is_eventsource = 0;
                is_websocket = 0;

                int col_client_id           = PQfnumber(res, "client_id");
                int col_response_status     = PQfnumber(res, "response_status");
                int col_response_headers    = PQfnumber(res, "response_headers");
                int col_response_body       = PQfnumber(res, "response_body");
                int col_eventsource         = PQfnumber(res, "eventsource");
                int col_websocket           = PQfnumber(res, "websocket");
                int col_trace               = PQfnumber(res, "trace");

                if (PQgetisnull(res, 0, col_client_id) != 1)
                    client_id = atoi(PQgetvalue(res, 0, col_client_id));
                if (PQgetisnull(res, 0, col_response_status) != 1)
                    tpgwsapi_output("%s\r\n", PQgetvalue(res, 0, col_response_status));
                if (PQgetisnull(res, 0, col_response_headers) != 1) {
                    int bytea_array_size = PQgetlength(res, 0, col_response_headers);
                    unsigned char *response_headers;
                    response_headers = PQgetvalue(res, 0, col_response_headers);
                    if (
                        bytea_array_size > 4 &&
                        bytea_array_size < 16384 &&
                        response_headers[0] == '{' &&
                        response_headers[bytea_array_size - 1] == '}'
                    ) {
                        unsigned char *header;
                        unsigned char *ptr;
                        unsigned char *header_start = NULL;
                        size_t to_length = 0;
                        ptr = response_headers;

                        while (*ptr) {
                            ptr++;
                            if (*ptr == '"') {
                                if (header_start == NULL)
                                    header_start = ptr+1;
                                else {
                                    *ptr = '\0';
                                    header = PQunescapeBytea(header_start+1, &to_length);
                                    if (header == NULL) {
                                        tpgwsapi_trace("PQunescapeBytea failed: %s\n", PQerrorMessage(conn));
                                    } else {
                                        *(header+to_length) = '\0';
                                        tpgwsapi_output("%s\r\n", header);
                                        PQfreemem(header);
                                    }
                                    header_start = NULL;
                                    *ptr = '"';
                                }

                            }
                        }
                    }
                }
                tpgwsapi_output("\r\n");
                if (PQgetisnull(res, 0, col_response_body) != 1) {
                    char *response_content;
                    size_t response_content_len;
                    response_content = PQunescapeBytea(PQgetvalue(res, 0, col_response_body), &response_content_len);
                    if (response_content == NULL) {
                        tpgwsapi_trace("PQunescapeBytea failed: %s\n", PQerrorMessage(conn));
                    } else {
                        char *content_buf = malloc(response_content_len + 1);
                        memcpy(content_buf, response_content, response_content_len);
                        PQfreemem(response_content);
                        *(content_buf+response_content_len) = '\0';
                        tpgwsapi_output("%s", content_buf);
                        free(content_buf);
                    }
                }
                if (PQgetisnull(res, 0, col_eventsource) != 1)
                    is_eventsource = ( !strcmp(PQgetvalue(res, 0, col_eventsource),"t") ? 1 : 0 );
                if (PQgetisnull(res, 0, col_websocket) != 1)
                    is_websocket = ( !strcmp(PQgetvalue(res, 0, col_websocket),"t") ? 1 : 0 );
                if (!is_websocket && !is_eventsource)
                    tpgwsapi_output("\r\n");
                if (with_trace && PQgetisnull(res, 0, col_trace) != 1) {
                    tpgwsapi_trace(\
                        "------------------------------------------------------------ \n" \
                        "postgres trace: \n%s\n", PQgetvalue(res, 0, col_trace));
                }
            } else {
                tpgwsapi_error("[tpgwsapi] PQntuples(res) != 1\n");
            }
            PQclear(res);
        }
    }
    if (content != NULL)
        free(content);
    if (request_body != NULL)
        PQfreemem(request_body);
}
