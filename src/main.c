#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 64
#define MAX_LINE 2048

/**
 * ====================================================================================
 * TABLA GLOBAL DE REGISTRO DE COMANDOS
 * ====================================================================================
 * Almacena los punteros a funciones e información descriptiva de cada comando educativo.
 * Cada elemento de la tabla inicializa una estructura Command declarada en shell.h.
 */
Command commands[] = {
    /* --- Categoría: Entrenamiento --- */
    {
        "saludar", "entrenamiento",
        "saludar",
        "Muestra un saludo personalizado para el usuario actual.",
        "getuid(2)",
        cmd_saludar
    },
    {
        "despedir", "entrenamiento",
        "despedir",
        "Muestra una despedida personalizada para el usuario actual.",
        "getuid(2)",
        cmd_despedir
    },
{
    "color", "entrenamiento",
    "color <color> \"<texto>\"",
    "Se le da el color al texto (green, yellow, blue, red).",
    "unknown",
    cmd_color
    },
{
    "clone", "entrenamiento",
    "clone <archivo>",
    "Se clona el archivo",
    "link()",
    cmd_clone
},

    /* --- Categoría: Editor --- */
{
    "editar", "editor",
    "editar",
    "Se abre el editor de archivos",
    "unknown",
    cmd_editar
    }
};

/* Número total de comandos en el shell */
const int num_commands = sizeof(commands) / sizeof(commands[0]);

/**
 * ====================================================================================
 * ANALIZADOR DE LÍNEA DE COMANDOS (TOKENIZADOR)
 * ====================================================================================
 * Esta función toma la línea de entrada introducida por el usuario y la divide en 
 * argumentos individuales. Soporta comillas dobles (") para permitir argumentos
 * que contienen espacios en blanco, como en: d_create archivo.txt "Este es el texto"
 *
 * Parámetros:
 * - line: La línea cruda leída del teclado. Modificada in-situ colocando caracteres nulos (\0).
 * - argv: Array de punteros que se llenará apuntando al inicio de cada argumento.
 *
 * Retorna:
 * - El número de argumentos (argc) detectados.
 */
int parse_line(char *line, char **argv) {
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

/**
 * ====================================================================================
 * FUNCIÓN DE AYUDA INTERACTIVA (HELP)
 * ====================================================================================
 * Muestra información pedagógica general del shell, por categoría o por comando individual.
 */
void print_help(const char *arg) {
    if (arg == NULL) {
        /* Caso 1: Escribió 'help' solo: Mostrar categorías principales */
        printf(COLOR_TITLE "\n--- Shell editora de texto ---\n" COLOR_RESET);
        printf("Este shell te permite explorar cómo funcionan las llamadas al sistema en Linux.\n");
        printf("Los comandos están clasificados en categorías.\n\n");
        printf("Categorías disponibles:\n");
        printf("  " COLOR_CATEGORY "entrenamiento" COLOR_RESET " - Comandos del entrenamiento (saludar, despedir, color, clonar)\n\n");
        printf("  " COLOR_CATEGORY "editor" COLOR_RESET " - Comandos de edición (editar)\n\n");
        printf("Uso general:\n");
        printf("  " COLOR_PROMPT "help <categoria>" COLOR_RESET "  - Muestra comandos específicos de una categoría.\n");
        printf("  " COLOR_PROMPT "help <comando>" COLOR_RESET "    - Explica el uso y las syscalls de un comando específico.\n");
        printf("  " COLOR_PROMPT "clear" COLOR_RESET "             - Limpia la pantalla.\n");
        printf("  " COLOR_PROMPT "exit" COLOR_RESET "              - Cierra el shell.\n\n");
        return;
    }

    /* Caso 2: El usuario escribió 'help <categoria>': Mostrar comandos del grupo */
    if (strcmp(arg, "entrenamiento") == 0 || strcmp(arg, "editor") == 0) {
        printf(COLOR_TITLE "\n--- Categoría: %s ---\n" COLOR_RESET, arg);
        for (int i = 0; i < num_commands; i++) {
            if (strcmp(commands[i].category, arg) == 0) {
                printf("  " COLOR_PROMPT "%-10s" COLOR_RESET " -> %s\n", commands[i].name, commands[i].description);
                printf("                " COLOR_INFO "Llamada(s): %s" COLOR_RESET "\n\n", commands[i].syscalls);
            }
        }
        return;
    }

    /* Caso 3: El usuario escribió 'help <comando>': Explicar syscalls individuales */
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(commands[i].name, arg) == 0) {
            printf(COLOR_TITLE "\nDetalles de comando: %s\n" COLOR_RESET, commands[i].name);
            printf("  Descripción:  %s\n", commands[i].description);
            printf("  Uso:          " COLOR_PARAM "%s" COLOR_RESET "\n", commands[i].usage);
            printf("  Syscalls:     " COLOR_SYSCALL "%s" COLOR_RESET "\n\n", commands[i].syscalls);
            return;
        }
    }

    printf(COLOR_ERROR "Categoría o comando '%s' no reconocido. Escribe 'help' para ver la ayuda.\n" COLOR_RESET, arg);
}

/**
 * ====================================================================================
 * BUCLE REPL PRINCIPAL (Read-Eval-Print Loop)
 * ====================================================================================
 * Controla el ciclo de vida del shell:
 * 1. Read: Lee la entrada del usuario usando fgets().
 * 2. Eval: Parse y busca si el comando coincide con built-ins o funciones registradas.
 * 3. Print: Imprime los resultados y las llamadas al sistema en consola.
 * 4. Loop: Repite el ciclo infinitamente hasta escribir 'exit' o presionar Ctrl+D.
 */
int main() {
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    /* Banner de bienvenida premium */
    printf(COLOR_TITLE "========================================================\n" COLOR_RESET);
    printf(COLOR_TITLE "    Shell editora de texto\n" COLOR_RESET);
    printf(COLOR_INFO "    Asignatura: SO2026B (Sistemas Operativos)\n" COLOR_RESET);
    printf(COLOR_INFO "    Escribe 'help' para iniciar. Desarrollado en C.\n" COLOR_RESET);
    printf(COLOR_TITLE "========================================================\n\n" COLOR_RESET);

    while (1) {
        /* Imprimir prompt cian interactivo */
        printf(COLOR_PROMPT "sys-shell> " COLOR_RESET);
        fflush(stdout); /* Asegurar que se muestre en pantalla antes de bloquear en fgets */

        /* Leer línea de entrada. Retorna NULL en EOF (Ctrl+D) */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        /* Tokenizar línea leída */
        int argc = parse_line(line, argv);
        if (argc == 0) {
            continue; /* Ignorar comandos vacíos */
        }

        /* Comandos Built-in generales */
        if (strcmp(argv[0], "exit") == 0) {
            printf(COLOR_INFO "Saliendo del shell. ¡Hasta luego!\n" COLOR_RESET);
            break;
        } else if (strcmp(argv[0], "clear") == 0) {
            printf("\033[H\033[J"); /* Limpia la pantalla usando secuencias de escape ANSI */
            continue;
        } else if (strcmp(argv[0], "help") == 0) {
            if (argc > 1) {
                print_help(argv[1]);
            } else {
                print_help(NULL);
            }
            continue;
        }

        /* Enrutar la ejecución buscando en la lista de comandos registrados */
        int found = 0;
        for (int i = 0; i < num_commands; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].handler(argc, argv);
                found = 1;
                break;
            }
        }

        /* Si no se encuentra en el registro del shell */
        if (!found) {
            printf(COLOR_ERROR "Comando '%s' no encontrado en el shell.\n" COLOR_RESET, argv[0]);
        }
        printf("\n");
    }

    return 0;
}
