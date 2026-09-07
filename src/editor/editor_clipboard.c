#include "editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * ====================================================================================
 * COMANDO: y (copiar línea)
 * ====================================================================================
 * Uso: y <n>
 * Copia la línea n al portapapeles
 */
int cmd_copy(EditorState *state, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: y <n>\n" COLOR_RESET);
        return 1;
    }

    if (state->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay ningún archivo abierto\n" COLOR_RESET);
        return 1;
    }

    int target_line = atoi(argv[1]);

    if (target_line < 1) {
        fprintf(stderr, COLOR_ERROR "El número de línea debe ser 1 o mayor\n" COLOR_RESET);
        return 1;
    }

    /* Obtener tamaño del archivo */
    struct stat st;

    /* --- SYSCALL: fstat --- */
    LOG_SYSCALL("fstat", "fd=%d", state->fd);
    if (fstat(state->fd, &st) == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(st.st_size);

    if (st.st_size == 0) {
        fprintf(stderr, COLOR_ERROR "El archivo está vacío\n" COLOR_RESET);
        return 1;
    }

    /* Leer archivo completo */
    char *buffer = malloc(st.st_size);

    if (buffer == NULL) {
        perror("malloc"); // Se deja perror porque malloc no es syscall POSIX (es de libc)
        return 1;
    }

    /* --- SYSCALL: lseek --- */
    LOG_SYSCALL("lseek", "fd=%d, offset=0, whence=SEEK_SET", state->fd);
    if (lseek(state->fd, 0, SEEK_SET) == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(buffer);
        return 1;
    }
    LOG_SYSCALL_RESULT(0);

    /* --- SYSCALL: read --- */
    LOG_SYSCALL("read", "fd=%d, buf, count=%ld", state->fd, (long)st.st_size);
    ssize_t bytes_read = read(state->fd, buffer, st.st_size);

    if (bytes_read == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(buffer);
        return 1;
    }
    LOG_SYSCALL_RESULT(bytes_read);


    /* Buscar dónde empieza la línea */
    off_t line_start = 0;
    int current_line = 1;

    if (target_line > 1) {

        int found = 0;

        for (off_t i = 0; i < bytes_read; i++) {

            if (buffer[i] == '\n') {
                current_line++;

                if (current_line == target_line) {
                    line_start = i + 1;
                    found = 1;
                    break;
                }
            }
        }

        if (!found || line_start >= bytes_read) {
            fprintf(stderr,
                    COLOR_ERROR "La línea %d no existe\n" COLOR_RESET,
                    target_line);

            free(buffer);
            return 1;
        }
    }


    /* Buscar dónde termina la línea */
    off_t line_end = line_start;

    while (line_end < bytes_read && buffer[line_end] != '\n') {
        line_end++;
    }

    size_t line_length = line_end - line_start;


    /* Crear el nuevo portapapeles */
    char *new_clipboard = malloc(line_length + 1);

    if (new_clipboard == NULL) {
        perror("malloc");
        free(buffer);
        return 1;
    }

    memcpy(new_clipboard, buffer + line_start, line_length);

    new_clipboard[line_length] = '\0';

    /* Reemplazar lo que hubiera copiado anteriormente */
    free(state->clipboard);

    state->clipboard = new_clipboard;

    free(buffer);

    printf(
        COLOR_RESULT
        "Línea %d copiada al portapapeles\n"
        COLOR_RESET,
        target_line
    );

    return 0;
}


/**
 * ====================================================================================
 * COMANDO: x (pegar línea)
 * ====================================================================================
 * Uso: x <n>
 * Pega el contenido del portapapeles en la línea n
 */
int cmd_paste(EditorState *state, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: x <n>\n" COLOR_RESET);
        return 1;
    }

    if (state->clipboard == NULL) {
        fprintf(stderr,
                COLOR_ERROR "No hay nada en el portapapeles\n"
                COLOR_RESET);

        return 1;
    }

    char *insert_argv[] = {
        "i",
        argv[1],
        state->clipboard
    };

    return cmd_insert(state, 3, insert_argv);
}