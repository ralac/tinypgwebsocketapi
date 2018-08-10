#include "pg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpq-fe.h"

/* CGI 1.1 Standard environment variables       */
/*
    AUTH_TYPE
    CONTENT_LENGTH
    CONTENT_TYPE
    GATEWAY_INTERFACE
    HTTP_*
    PATH_INFO
    PATH_TRANSLATED
    QUERY_STRING
    REMOTE_ADDR
    REMOTE_HOST
    REMOTE_IDENT
    REMOTE_USER
    REQUEST_METHOD
    SCRIPT_NAME
    SERVER_NAME
    SERVER_PORT
    SERVER_PROTOCOL
    SERVER_SOFTWARE
--------------------------------------------------
** HTTP Headers commonly used                   **
--------------------------------------------------
    HTTP_ACCEPT
    HTTP_USER_AGENT
    HTTP_REFERER
    HTTP_COOKIE
--------------------------------------------------
** HTTP Headers for web sockets                 **
--------------------------------------------------
    HTTP_HOST
    HTTP_UPGRADE
    HTTP_CONNECTION
    HTTP_SEC_WEBSOCKET_KEY
    HTTP_ORIGIN
    HTTP_SEC_WEBSOCKET_PROTOCOL
    HTTP_SEC_WEBSOCKET_VERSION
--------------------------------------------------
** TPGWSAPI environment variables               **
--------------------------------------------------
    TRACE
    PG_URL
    PG_FUNCTION
    TPGWSAPI_SERVICE
    TPGWSAPI
--------------------------------------------------
** TPGWSAPI websockets environment variables    **
--------------------------------------------------
    READING_BUFFER_SIZE
    FD_POLLING_TIMEOUT
--------------------------------------------------
                                                */
char *trace;
char *pg_url;
char *pg_function;
char *tpgwsapi_service;

int main() {
char *
}
// Needs PGconn

		char	   *cmdbuf;
		char	   *bufptr;
		size_t		slen = strlen(comment_arg);

		cmdbuf = malloc(slen * 2 + 256);
		if (!cmdbuf)
			return fail_lo_xact("\\lo_import", own_transaction);
		sprintf(cmdbuf, "COMMENT ON LARGE OBJECT %u IS '", loid);
		bufptr = cmdbuf + strlen(cmdbuf);
		bufptr += PQescapeStringConn(pset.db, bufptr, comment_arg, slen, NULL);
		strcpy(bufptr, "'");


		free(cmdbuf);

// PQescapeByteaConn Needs PQfreemem(ptr) at the end
