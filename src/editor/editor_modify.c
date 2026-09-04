#include "editor.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

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

/**
 * ====================================================================================
 * COMANDO: d (borrar la línea n)
 * ====================================================================================
 * Uso: d <n>
 *
 * Como no se puede "sacar" un pedazo del medio de un archivo, la estrategia es:
 * 1. Averiguar el tamaño del archivo con fstat().
 * 2. Leer TODO el archivo a un buffer en memoria con read().
 * 3. Recorrer ese buffer contando saltos de línea '\n', copiando todo a un
 *    segundo buffer EXCEPTO los bytes que pertenecen a la línea n.
 * 4. Volver al inicio del archivo con lseek() y reescribirlo con write().
 * 5. Cortar el sobrante del final con ftruncate(), porque el archivo nuevo
 *    es más corto que el original.
 */
int cmd_delete(EditorState *state, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, COLOR_ERROR "Uso: d <n>\n" COLOR_RESET);
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

    /* 1. LLAMADA AL SISTEMA: fstat -> saber el tamaño actual del archivo */
    struct stat st;
    LOG_SYSCALL("fstat", "fd=%d", state->fd);
    if (fstat(state->fd, &st) == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(st.st_size);

    off_t file_size = st.st_size;

    if (file_size == 0) {
        fprintf(stderr, COLOR_ERROR "El archivo está vacío\n" COLOR_RESET);
        return 1;
    }

    char *original = malloc(file_size);
    char *result   = malloc(file_size); /* el resultado nunca puede ser más grande */

    if (original == NULL || result == NULL) {
        perror("malloc");
        free(original);
        free(result);
        return 1;
    }

    /* Nos aseguramos de leer desde el inicio del archivo */
    LOG_SYSCALL("lseek", "fd=%d, offset=0, whence=SEEK_SET", state->fd);
    lseek(state->fd, 0, SEEK_SET);
    LOG_SYSCALL_RESULT(0);

    /* 2. LLAMADA AL SISTEMA: read -> traer todo el archivo a memoria */
    LOG_SYSCALL("read", "fd=%d, buf, count=%ld", state->fd, (long)file_size);
    ssize_t bytes_read = read(state->fd, original, file_size);
    if (bytes_read == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(original);
        free(result);
        return 1;
    }
    LOG_SYSCALL_RESULT(bytes_read);

    /* 3. Recorremos byte por byte, contando líneas, y copiamos todo
       menos la línea que se quiere borrar */
    int current_line = 1;
    off_t result_len = 0;
    int line_found = 0;

    for (off_t i = 0; i < bytes_read; i++) {
        char c = original[i];

        if (current_line != target_line) {
            result[result_len++] = c;
        } else {
            line_found = 1;
        }

        if (c == '\n') {
            current_line++;
        }
    }

    if (!line_found) {
        fprintf(stderr, COLOR_ERROR "La línea %d no existe\n" COLOR_RESET, target_line);
        free(original);
        free(result);
        return 1;
    }

    /* Volvemos al inicio para reescribir el archivo con el contenido nuevo */
    LOG_SYSCALL("lseek", "fd=%d, offset=0, whence=SEEK_SET", state->fd);
    lseek(state->fd, 0, SEEK_SET);
    LOG_SYSCALL_RESULT(0);

    /* 4. LLAMADA AL SISTEMA: write -> reescribir el archivo sin la línea borrada */
    LOG_SYSCALL("write", "fd=%d, buf, count=%ld", state->fd, (long)result_len);
    ssize_t bytes_written = write(state->fd, result, result_len);
    if (bytes_written == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(original);
        free(result);
        return 1;
    }
    LOG_SYSCALL_RESULT(bytes_written);

    /* 5. LLAMADA AL SISTEMA: ftruncate -> cortar los bytes sobrantes del final */
    LOG_SYSCALL("ftruncate", "fd=%d, length=%ld", state->fd, (long)result_len);
    if (ftruncate(state->fd, result_len) == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(original);
        free(result);
        return 1;
    }
    LOG_SYSCALL_RESULT(0);

    free(original);
    free(result);

    printf(COLOR_RESULT "Línea %d borrada correctamente\n" COLOR_RESET, target_line);
    return 0;
}