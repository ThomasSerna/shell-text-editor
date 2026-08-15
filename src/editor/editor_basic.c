#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/sysmacros.h>

#include "editor.h"

int cmd_open(EditorState *state, int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: o <archivo>\n" COLOR_RESET);
        return 1;
    }
    const char *filename = argv[1];

    // System call: open()
    int new_fd = open(filename, O_RDWR | O_CREAT, 0644);

    if (new_fd == -1) {
        LOG_SYSCALL_ERROR(strerror(errno));
        return 1;
    }

    if (state->fd != -1) {
        close(state->fd);
    }

    state->fd = new_fd;

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

int cmd_metadata(EditorState *state, int argc, char **argv) {
    (void)argc;
    (void)argv;
    struct stat st;

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

    printf(COLOR_TITLE "--- Metadatos del Archivo (stat) ---\n" COLOR_RESET);
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
