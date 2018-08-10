#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libpq-fe.h"

char *trace;
char *pg_url;
char *pg_function;
char *tpgwsapi_service;

char *sql_cmd;

void pg_connect() {
    PGresult   *res;

    pg_url             = getenv("PG_URL");

    if (conn == NULL && pg_url != NULL)
    {
        /* Make a connection to the database */
        conn = PQconnectdb(conninfo);

        /* Check to see that the backend connection was successfully made */
        if (PQstatus(conn) != CONNECTION_OK)
        {
            fprintf(stderr, "[auth] Connection to database failed: %s",
                    PQerrorMessage(conn));
            return false;
        }

        /* Set always-secure search path, so malicous users can't take control. */
        res = PQexec(conn, "SELECT pg_catalog.set_config('search_path', '', false)");
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "[auth] SET failed: %s", PQerrorMessage(conn));
            PQclear(res);
            exit_nicely(conn);
        }

        /*
         * Should PQclear PGresult whenever it is no longer needed to avoid memory
         * leaks
         */
        PQclear(res);

    }
}

void pg_disconnect() {
    if (conn != NULL)
    {
        /* close the connection to the database and cleanup */
        PQfinish(conn);
    }
}

void call_handler_function(char *envp[]) {
    PGresult *res;
    long int content_length = 0;
    char *method;
    char *content_length_env;
    char *content = NULL;
    int i;

    content_length_env = getenv("CONTENT_LENGTH");
    if (content_length_env != NULL) {
        content_length = strtol(content_length_env, NULL, 10);
    }

    method = getenv("REQUEST_METHOD");
    if (!strcmp(method, "POST") && content_length) {
        content = malloc(content_length + 1);
        if (!content) {
            fprintf(stderr, "[auth] Out of memory");
            exit(EXIT_FAILURE);
        }
        fgets(content, content_length + 1, stdin);
    }
    PQescapeBytea
    PQunescapeBytea


    for (i = 0; envp[i] != NULL; i++)
        printf("\n%s", envp[i]);
        PQescapeStringConn

    if (
        sprintf(
            sql_cmd,
            "SELECT * FROM %s('%s', '%s',  '%s', NULL);",
            pg_function,
            getpid(),
            cgi_environment,
            request_body
        ) < 0
    ) {
        fprintf(stderr, "sprintf failed");
    } else {
        res = PQexec(conn, sql_cmd);
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "[auth] SELECT failed: %s", PQerrorMessage(conn));
            PQclear(res);
        } else {
            /* handle result */
        }
    }

    if (content_length != NULL)
        free(content_length);
}

