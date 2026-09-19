#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/sysmacros.h>

#include "editor.h"

/**
 * ====================================================================================
 * COMANDO: o (abrir archivo)
 * ====================================================================================
 * Uso: o <archivo>
 * Abre un archivo en disco. Si no existe, lo crea con los permisos adecuados.
 */
int cmd_open(EditorState *state, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: o <archivo>\n" COLOR_RESET);
        return 1;
    }
    const char *filename = argv[1];

    /* --- SYSCALL: open --- */
    LOG_SYSCALL("open", "pathname=\"%s\", flags=O_RDWR|O_CREAT, mode=0644", filename);
    int new_fd = open(filename, O_RDWR | O_CREAT, 0644);

    if (new_fd == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(new_fd);

    // Si ya había un archivo abierto, lo cerramos
    if (state->fd != -1) {
        /* --- SYSCALL: close (archivo anterior) --- */
        LOG_SYSCALL("close", "fd=%d", state->fd);
        int res = close(state->fd);

        if (res == -1) {
            LOG_SYSCALL_ERROR(strerror(errno));
            close(new_fd); // Cerramos el nuevo para evitar fugas
            return 1;
        }
        LOG_SYSCALL_RESULT(res);
    }

    state->fd = new_fd;

    printf(COLOR_INFO "Se ha abierto el archivo %s\n" COLOR_RESET, filename);
    return 0;
}

/**
 * ====================================================================================
 * COMANDO: q (salir / cerrar)
 * ====================================================================================
 * Uso: q
 * Cierra el File Descriptor y sale de la aplicación sin dejar fugas de memoria.
 */
int cmd_quit(EditorState *state, int argc, char **argv)
{
    (void) argc;
    (void) argv;

    if (state->clipboard != NULL)
    {
        free(state->clipboard);
        state->clipboard = NULL;
    }

    if (state->fd == -1)
    {
        printf(COLOR_INFO "No habia ningún archivo abierto\n" COLOR_RESET);
        return 0;
    }

    /* --- SYSCALL: close --- */
    LOG_SYSCALL("close", "fd=%d", state->fd);
    int res = close(state->fd);

    if (res == -1)
    {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(res);

    state->fd = -1;

    printf(COLOR_INFO "Se ha cerrado el file descriptor\n" COLOR_RESET);
    return 0;
}