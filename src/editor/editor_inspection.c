#include "editor.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sysmacros.h>

int cmd_search(EditorState *state, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: s <texto>\n" COLOR_RESET);
        return 1;
    }

    if (state->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay ningún archivo abierto\n" COLOR_RESET);
        return 1;
    }

    const char *text = argv[1];

    struct stat st;

    if (strlen(text) == 0) {
        fprintf(stderr, COLOR_ERROR "El texto de búsqueda no puede estar vacio\n" COLOR_RESET);
        return 1;
    }

    if (fstat(state->fd, &st) == -1) {
        perror("fstat");
        return 1;
    }

    char *buffer = malloc(st.st_size + 1);

    if (buffer == NULL) {
        perror("malloc");
        return 1;
    }

    if (lseek(state->fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        free(buffer);
        return 1;
    }

    ssize_t bytes_read = read(state->fd, buffer, st.st_size);

    if (bytes_read == -1) {
        perror("read");
        free(buffer);
        return 1;
    }

    buffer[bytes_read] = '\0';

    char *current = buffer;
    int count = 0;

    while ((current = strstr(current, text)) != NULL) {
        count++;
        current += strlen(text);
    }

    if (count == 0) {
        printf(COLOR_INFO
               "No se encontraron coincidencias para '%s'\n"
               COLOR_RESET,
               text);
    } else {
        printf(COLOR_RESULT
               "Se encontraron %d coincidencias de '%s'\n"
               COLOR_RESET,
               count,
               text);
    }

    free(buffer);

    return 0;
}


int cmd_metadata(EditorState *state, int argc, char **argv) {
    (void)argv;
    struct stat st;

    if (argc != 1) {
        fprintf(stderr, COLOR_ERROR "Uso: m\n" COLOR_RESET);
        return 1;
    }

    if (state->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay ningún archivo abierto\n" COLOR_RESET);
        return 1;
    }

    /* 1. LLAMADA AL SISTEMA: fstat */
    int res = fstat(state->fd, &st);
    if (res == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }

    printf(COLOR_TITLE "--- Metadatos del Archivo (fstat) ---\n" COLOR_RESET);
    /* st_dev: ID del dispositivo físico que contiene el archivo */
    printf("  Dispositivo:    [%ld,%ld]\n", (long)major(st.st_dev), (long)minor(st.st_dev));
    /* st_ino: Número de inodo único del archivo dentro del dispositivo */
    printf("  Inodo:          " COLOR_PARAM "%ld" COLOR_RESET "\n", (long)st.st_ino);
    /* st_mode: Modos y permisos de acceso del archivo */
    printf("  Permisos (oct): " COLOR_PARAM "%o" COLOR_RESET "\n", st.st_mode & 0777);
    /* st_nlink: Número de enlaces físicos apuntando a este inodo */
    printf("  Enlaces (nlink):%ld\n", (long)st.st_nlink);
    /* st_uid y st_gid: IDs de dueño y grupo asignados */
    printf("  UID de Dueño:   %ld\n", (long)st.st_uid);
    printf("  GID de Grupo:   %ld\n", (long)st.st_gid);
    /* st_size: Tamaño real del archivo en bytes */
    printf("  Tamaño:         " COLOR_RESULT "%ld bytes" COLOR_RESET "\n", (long)st.st_size);
    /* st_blksize: Tamaño óptimo de bloque sugerido por el sistema de ficheros para E/S */
    printf("  Tamaño Bloque:  %ld bytes\n", (long)st.st_blksize);
    /* st_blocks: Número real de bloques de 512 bytes asignados en el disco */
    printf("  Bloques Ocup.:  %ld\n", (long)st.st_blocks);
    /* st_mtime: Fecha y hora de la última modificación del contenido */
    printf("  MTime (Modif):  %s", ctime(&st.st_mtime));
    printf(COLOR_TITLE "------------------------------------\n" COLOR_RESET);

    return 0;
}

/**
 * ====================================================================================
 * COMANDO: p (imprimir línea n, o todo el archivo si no se indica n)
 * ====================================================================================
 * Uso: p        -> imprime todo el archivo
 *      p <n>    -> imprime solo la línea n
 */
int cmd_print(EditorState *state, int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, COLOR_ERROR "Uso: p [n]\n" COLOR_RESET);
        return 1;
    }

    if (state->fd == -1) {
        fprintf(stderr, COLOR_ERROR "No hay ningún archivo abierto\n" COLOR_RESET);
        return 1;
    }

    int target_line = 0;
    if (argc == 2) {
        target_line = atoi(argv[1]);
        if (target_line < 1) {
            fprintf(stderr, COLOR_ERROR "El número de línea debe ser 1 o mayor\n" COLOR_RESET);
            return 1;
        }
    }

    struct stat st;
    LOG_SYSCALL("fstat", "fd=%d", state->fd);
    if (fstat(state->fd, &st) == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }
    LOG_SYSCALL_RESULT(st.st_size);

    off_t file_size = st.st_size;
    if (file_size == 0) {
        printf(COLOR_INFO "El archivo está vacío\n" COLOR_RESET);
        return 0;
    }

    char *buffer = malloc(file_size);
    if (buffer == NULL) {
        perror("malloc");
        return 1;
    }

    LOG_SYSCALL("lseek", "fd=%d, offset=0, whence=SEEK_SET", state->fd);
    lseek(state->fd, 0, SEEK_SET);
    LOG_SYSCALL_RESULT(0);

    LOG_SYSCALL("read", "fd=%d, buf, count=%ld", state->fd, (long)file_size);
    ssize_t bytes_read = read(state->fd, buffer, file_size);
    if (bytes_read == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        free(buffer);
        return 1;
    }
    LOG_SYSCALL_RESULT(bytes_read);

    int current_line = 1;
    off_t line_start = 0;
    int printed_something = 0;

    for (off_t i = 0; i < bytes_read; i++) {
        if (buffer[i] == '\n') {
            if (target_line == 0 || target_line == current_line) {
                off_t len = (i - line_start) + 1;
                LOG_SYSCALL("write", "fd=1, buf, count=%ld", (long)len);
                write(STDOUT_FILENO, buffer + line_start, len);
                LOG_SYSCALL_RESULT(len);
                printed_something = 1;
            }
            current_line++;
            line_start = i + 1;
        }
    }

    if (line_start < bytes_read && (target_line == 0 || target_line == current_line)) {
        off_t len = bytes_read - line_start;
        write(STDOUT_FILENO, buffer + line_start, len);
        write(STDOUT_FILENO, "\n", 1);
        printed_something = 1;
    }

    free(buffer);

    if (target_line != 0 && !printed_something) {
        fprintf(stderr, COLOR_ERROR "La línea %d no existe\n" COLOR_RESET, target_line);
        return 1;
    }

    return 0;
}