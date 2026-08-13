#include "shell.h"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>


/**
 * ====================================================================================
 * COMANDO: saludar
 * ====================================================================================
 */
int cmd_saludar(int argc, char **argv) {
    (void)argc;
    (void)argv;

    /* 1. LLAMADA AL SISTEMA: getuid */
    LOG_SYSCALL("getuid", "");
    uid_t uid = getuid();
    LOG_SYSCALL_RESULT(uid); /* Devuelve el ID numérico del usuario actual */

    /* Consultar base de datos del sistema /etc/passwd */
    struct passwd *pw = getpwuid(uid);
    const char *username = pw ? pw->pw_name : "usuario desconocido";

    printf(COLOR_RESULT "¡Hola, %s! Bienvenido al Shell editor de texto.\n" COLOR_RESET, username);
    return 0;
}

/*
 * ========
 * Despedir
 * ========
 */

int cmd_despedir(int argc, char **argv) {
    (void)argc;
    (void)argv;

    /* 1. LLAMADA AL SISTEMA: getuid */
    LOG_SYSCALL("getuid", "");
    uid_t uid = getuid();
    LOG_SYSCALL_RESULT(uid); /* Devuelve el ID numérico del usuario actual */

    /* Consultar base de datos del sistema /etc/passwd */
    struct passwd *pw = getpwuid(uid);
    const char *username = pw ? pw->pw_name : "usuario desconocido";

    printf(COLOR_RESULT "¡Chao, %s!\n" COLOR_RESET, username);
    return 0;
}

/*
 * ========
 * color
 * ========
 */

int cmd_color(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, COLOR_ERROR "Uso: color <color> <texto>\n" COLOR_RESET);
        return 1;
    }
    const char *color = argv[1];
    const char *text = argv[2];

    char *colorCode;

    if (strcmp(color, "green") == 0) {
        colorCode = "\033[32m";
    } else if (strcmp(color, "yellow") == 0) {
        colorCode = "\033[33m";
    } else if (strcmp(color, "red") == 0) {
        colorCode = "\033[31m";
    } else if (strcmp(color, "blue") == 0) {
        colorCode = "\033[34m";
    } else {
        fprintf(stderr,COLOR_ERROR "Color invalido: %s\n" COLOR_RESET, color);
        return 1;
    }

    printf("%s%s\n" COLOR_RESET,colorCode, text);
    return 0;
}

/*
 * ========
 * clonar
 * ========
 */
int cmd_clone(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, COLOR_ERROR "Uso: clone <archivo>\n" COLOR_RESET);
        return 1;
    }

    const char *src_filename = argv[1];
    char name[256];
    snprintf(name, sizeof(name), "clone.%s", src_filename);

    int success = link(src_filename, name);

    if (success == -1)
    {
        LOG_SYSCALL_ERROR(strerror(errno));
    }

    printf("Archivo %s exitosamente clonado en %s\n", src_filename, name);

    return 0;
}
