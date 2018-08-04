#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Most of the C compilers support a third parameter to main which
// store all environment variables
int main()
{
    char *line = NULL;
    size_t line_length = 0;
    ssize_t nread;
    char *http_header = "Cache-Control: no-cache;\nContent-Type: text/event-stream\nTransfer-Encoding: chunked\n\n";
    char *str_chunk_size;
    str_chunk_size = malloc(32);

    if (fwrite(http_header, strlen(http_header), 1, stdout) == 1)
    {
        line = malloc(32);
        while ((nread = getline(&line, &line_length, stdin)) != -1)
        {
            if (sprintf(str_chunk_size, "%04x\r\n", nread) < 0)
                break;
            if (fwrite(str_chunk_size, strlen(str_chunk_size), 1, stdout) != 1)
                break;
            if (fwrite(line, nread, 1, stdout) != 1)
                break;
            if (fwrite("\r\n", 2, 1, stdout) != 1)
                break;
            fflush(stdout);
            line_length = 0;
        }
        free(line);
    }
}
