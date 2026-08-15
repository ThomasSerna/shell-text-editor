#include "editor.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 64
#define MAX_LINE 2048

CommandEditor commands_editor[] = {
    {
        "o",
        "o <archivo>",
        "Abre un archivo en disco. Si no existe, lo crea con los permisos adecuados. ",
        "open()",
        cmd_open
    }, {
        "q",
        "q",
        "Cierra el File Descriptor y sale de la aplicación sin dejar fugas de memoria",
        "close()",
        cmd_quit
    }, {
        "m",
        "m",
        "Imprime los metadatos del archivo",
        "fstat()",
        cmd_metadata
    }
};

const int num_commands_editor = sizeof(commands_editor) / sizeof(commands_editor[0]);

int parse_line_editor(char *line, char **argv) {
    int argc = 0;
    char *p = line;
    int in_quote = 0;
    char *arg_start = NULL;

    while (*p) {
        if (!in_quote && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
            /* Espacio en blanco fuera de comillas: Terminar el token actual */
            if (arg_start != NULL) {
                *p = '\0'; /* Inserta carácter de fin de cadena */
                argv[argc++] = arg_start;
                arg_start = NULL;
            }
        } else if (*p == '"') {
            /* Cambiar el estado de lectura de comillas */
            if (in_quote) {
                /* Fin del bloque entre comillas */
                *p = '\0';
                argv[argc++] = arg_start;
                arg_start = NULL;
                in_quote = 0;
            } else {
                /* Inicio del bloque entre comillas */
                in_quote = 1;
                arg_start = p + 1;
            }
        } else {
            /* Carácter ordinario del argumento */
            if (arg_start == NULL) {
                arg_start = p;
            }
        }
        p++;
    }
    /* Añadir el último argumento si quedó pendiente */
    if (arg_start != NULL) {
        argv[argc++] = arg_start;
    }
    argv[argc] = NULL; /* Convención POSIX de terminar argv con NULL */
    return argc;
}


void print_help_editor(const char *arg) {
    if (arg == NULL)
    {
        printf(COLOR_TITLE "\n--- Editor de texto ---\n" COLOR_RESET);
        printf("Estás en la herramienta editora de texto de la shell\n");
        printf("\nComandos disponibles:\n");
        for (int i = 0; i < num_commands_editor; i++)
        {
            printf("  " COLOR_EDITOR_PROMPT "%-10s" COLOR_RESET " -> %s\n", commands_editor[i].name, commands_editor[i].description);
        }

        printf("\nUso general:\n");
        printf("  " COLOR_EDITOR_PROMPT "help <comando>" COLOR_RESET "  -> Muestra la información especifica de un comando\n");
        printf("  " COLOR_EDITOR_PROMPT "clear" COLOR_RESET "             -> Limpia la pantalla.\n");
        printf("  " COLOR_EDITOR_PROMPT "exit" COLOR_RESET "              -> Cierra el shell.\n\n");

        printf("Si necesitas información de un comando especifico, escribe 'help <comando>'\n\n");
        return;
    }

    for (int i = 0; i < num_commands_editor; i++) {
        if (strcmp(commands_editor[i].name, arg) == 0) {
            printf(COLOR_TITLE "\nDetalles de comando: %s\n" COLOR_RESET, commands_editor[i].name);
            printf("  Descripción:  %s\n", commands_editor[i].description);
            printf("  Uso:          " COLOR_PARAM "%s" COLOR_RESET "\n", commands_editor[i].usage);
            printf("  Syscalls:     " COLOR_SYSCALL "%s" COLOR_RESET "\n\n", commands_editor[i].syscalls);
            return;
        }
    }

    printf(COLOR_ERROR "Comando '%s' no reconocido. Escribe 'help' para ver la ayuda.\n" COLOR_RESET, arg);
}

int editor_main()
{
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    EditorState state = {
        -1
    };

    printf(COLOR_INFO "Abriendo editor de texto...\n\n" COLOR_RESET);

    while (1) {
        /* Imprimir prompt cian interactivo */
        printf(COLOR_EDITOR_PROMPT "editor> " COLOR_RESET);
        fflush(stdout); /* Asegurar que se muestre en pantalla antes de bloquear en fgets */

        /* Leer línea de entrada. Retorna NULL en EOF (Ctrl+D) */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (cmd_quit(&state, 1, NULL) != 0) {
                continue;
            }
            printf("\n");
            break;
        }

        /* Tokenizar línea leída */
        int argc = parse_line_editor(line, argv);
        if (argc == 0) {
            continue; /* Ignorar comandos vacíos */
        }

        /* Comandos Built-in generales */
        if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "q") == 0) {
            if (cmd_quit(&state, 1, NULL) != 0) {
                continue;
            }
            printf(COLOR_INFO "Saliendo del editor. ¡Hasta luego!\n" COLOR_RESET);
            break;
        } else if (strcmp(argv[0], "clear") == 0) {
            printf("\033[H\033[J"); /* Limpia la pantalla usando secuencias de escape ANSI */
            continue;
        } else if (strcmp(argv[0], "help") == 0) {
            if (argc > 1) {
                print_help_editor(argv[1]);
            } else {
                print_help_editor(NULL);
            }
            continue;
        }

        /* Enrutar la ejecución buscando en la lista de comandos registrados */
        int found = 0;
        for (int i = 0; i < num_commands_editor; i++) {
            if (strcmp(argv[0], commands_editor[i].name) == 0) {
                commands_editor[i].handler(&state, argc, argv);
                found = 1;
                break;
            }
        }

        /* Si no se encuentra en el registro del shell */
        if (!found) {
            printf(COLOR_ERROR "Comando '%s' no encontrado en el editor.\n" COLOR_RESET, argv[0]);
        }
        printf("\n");
    }

    return 0;
}