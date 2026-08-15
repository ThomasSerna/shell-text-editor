#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "editor.h"

int cmd_open(EditorState *state, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: o <archivo>\n" COLOR_RESET);
        return 1;
    }
    const char *filename = argv[1];

    if (state->fd != -1) {
        close(state->fd);
        state->fd = -1;
    }

    /* 1. LLAMADA AL SISTEMA: open */
    state->fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (state->fd == -1) {
        /* Si falla open, retorna -1 y asigna el código en errno */
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }

    printf(COLOR_INFO "Se ha abierto el archivo %s\n" COLOR_RESET, filename);
    return 0;
}

int cmd_quit(EditorState *state, int argc, char **argv)
{
    (void) argc;
    (void) argv;

    if (state->fd == -1)
    {
        printf(COLOR_INFO "No habia ningún archivo abierto\n" COLOR_RESET);
        return 0;
    }

    int res = close(state->fd);
    if (res == -1)
    {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    state->fd = -1;

    printf(COLOR_INFO "Se ha cerrado el file descriptor\n" COLOR_RESET);
    return 0;
}
