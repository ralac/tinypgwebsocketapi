#include <fcgi_stdio.h>
#include "pg.h"

char *tpgwsapi;

int main(int argc, char *argv[], char *envp[])
{
    pg_url              = getenv("PG_URL");
    pg_function         = getenv("PG_FUNCTION");
    tpgwsapi_service    = getenv("TPGWSAPI_SERVICE");
    tpgwsapi            = getenv("TPGWSAPI");

    pg_connect();
    if (!strcmp(tpgwsapi,"FCGI")) {
        while (FCGI_Accept() >= 0)
        {
            call_handler_function(envp);
        }
    }
    if (!strcmp(tpgwsapi,"CGI")) {
            call_handler_function(envp);
    }
    pg_disconnect();

    return 0;
}
