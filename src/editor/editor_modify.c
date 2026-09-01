#include "editor.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

/**
 * ====================================================================================
 * COMANDO: a (añadir texto al final del archivo)
 * ====================================================================================
 * Uso: a <texto>
 *
 * 1. Se posiciona el cursor de lectura/escritura al FINAL del archivo con lseek().
 *    Esto es necesario porque cada llamada a write() escribe a partir de la
 *    posición actual del "cursor" del file descriptor, no siempre al final.
 * 2. Se escribe el texto seguido de un salto de línea '\n' con write().
 */
int cmd_add(EditorState *state, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, COLOR_ERROR "Uso: a <texto>\n" COLOR_RESET);
        return 1;
    }

    if (state->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay ningún archivo abierto\n" COLOR_RESET);
        return 1;
    }

    const char *text = argv[1];

    /* 1. LLAMADA AL SISTEMA: lseek -> mover el cursor al final del archivo */
    LOG_SYSCALL("lseek", "fd=%d, offset=0, whence=SEEK_END", state->fd);
    off_t new_pos = lseek(state->fd, 0, SEEK_END);

    if (new_pos == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(new_pos);

    /* Preparamos el texto agregándole un salto de línea al final */
    size_t text_len = strlen(text);
    char *line = malloc(text_len + 2); /* +1 para '\n', +1 para '\0' */

    if (line == NULL) {
        perror("malloc");
        return 1;
    }

    memcpy(line, text, text_len);
    line[text_len] = '\n';
    line[text_len + 1] = '\0';

    /* 2. LLAMADA AL SISTEMA: write -> escribir el texto en el archivo */
    LOG_SYSCALL("write", "fd=%d, buf=\"%s\", count=%zu", state->fd, text, text_len + 1);
    ssize_t bytes_written = write(state->fd, line, text_len + 1);

    if (bytes_written == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(line);
        return 1;
    }
    LOG_SYSCALL_RESULT(bytes_written);

    free(line);

    printf(COLOR_RESULT "Línea añadida correctamente al final del archivo\n" COLOR_RESET);
    return 0;
}