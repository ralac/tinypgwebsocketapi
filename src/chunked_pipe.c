#include <stdio.h>
#include <stdlib.h>

// Most of the C compilers support a third parameter to main which
// store all environment variables
int main()
{
    char *line = NULL;
    size_t line_length = 0;
    ssize_t nread;
    char *http_header = "Content-Type: text/event-stream\nTransfer-Encoding: chunked\n\n";
    char *str_chunk_size = "0000000\n";

    line = malloc(32);
    while ((nread = getline(&line, &line_length, stdin)) != -1)
    {
        if (fwrite(http_header, sizeof(http_header), 1, stdout) != 1)
            break;

        if (sprintf(str_chunk_size,"%d\n", line_length) < 0)
            break;

        if (fwrite(str_chunk_size, sizeof(str_chunk_size), 1, stdout) != 1)
            break;

        if (fwrite(line, nread, 1, stdout) != 1)
            break;

        line_length = 0;
    }
    free(line);
}