#ifndef SHELL_TEXT_EDITOR_EDITOR_H
#define SHELL_TEXT_EDITOR_EDITOR_H

typedef struct
{
    int fd;
} EditorState;

typedef struct {
    const char *name;        /* Nombre textual del comando que el usuario escribe (e.g., 'd_create') */
    const char *usage;       /* Sintaxis de uso del comando para mostrar en caso de error */
    const char *description; /* Explicación en español de lo que hace el comando a nivel lógico */
    const char *syscalls;    /* Explicación de las syscalls involucradas que se mostrarán en la ayuda */
    int (*handler)(EditorState *state ,int argc, char **argv); /* Puntero a la función que implementa la lógica del comando */
} CommandEditor;

#define COLOR_RESET     "\033[0m"      /* Restablece todos los atributos de color y estilo */
#define COLOR_EDITOR_PROMPT "\033[0;32m" /* Verde */
#define COLOR_SYSCALL   "\033[1;35m"   /* Púrpura/Magenta brillante: Resalta el nombre de las syscalls ejecutadas */
#define COLOR_PARAM     "\033[0;33m"   /* Amarillo/Marrón: Usado para valores numéricos o parámetros importantes */
#define COLOR_RESULT    "\033[1;32m"   /* Verde brillante: Destaca valores de retorno exitosos (>= 0 o punteros válidos) */
#define COLOR_ERROR     "\033[1;31m"   /* Rojo brillante: Resalta fallos y descripciones de error (errno) */
#define COLOR_INFO      "\033[0;90m"   /* Gris oscuro: Texto de depuración o comentarios explicativos menores */
#define COLOR_TITLE     "\033[1;97m"   /* Blanco brillante en negrita: Para encabezados de tablas y banners */
#define COLOR_CATEGORY  "\033[1;34m"   /* Azul brillante: Categorías del sistema en el comando 'help' */

/* Imprime la llamada al sistema y sus parámetros */
#define LOG_SYSCALL(syscall_name, format, ...) \
    printf(COLOR_SYSCALL "[syscall] " syscall_name COLOR_RESET "(" format ") ... " COLOR_RESET, ##__VA_ARGS__)

/* Imprime el valor de retorno en verde cuando es un número entero exitoso (e.g., File Descriptor o bytes) */
#define LOG_SYSCALL_RESULT(result) \
    printf("= " COLOR_RESULT "%ld" COLOR_RESET "\n", (long)(result))

/* Imprime la dirección de memoria de retorno en verde cuando se retorna un puntero (e.g., mmap, sbrk) */
#define LOG_SYSCALL_RESULT_PTR(result) \
    printf("= " COLOR_RESULT "%p" COLOR_RESET "\n", (void*)(result))

/* Imprime el valor -1 en rojo indicando un error y adjunta el texto descriptivo del código errno */
#define LOG_SYSCALL_ERROR(err_name) \
    printf("= " COLOR_ERROR "-1 (%s)" COLOR_RESET "\n", err_name)

/**
 * ====================================================================================
 * Interfaz: editor
 * ====================================================================================
 */
int editor_main();

/**
 * ====================================================================================
 * COMANDO: saludar
 * ====================================================================================
 */
int cmd_open(EditorState *state ,int argc, char **argv);
int cmd_quit(EditorState *state ,int argc, char **argv);
int cmd_metadata(EditorState *state ,int argc, char **argv);
int cmd_search(EditorState *state ,int argc, char **argv);

#endif //SHELL_TEXT_EDITOR_EDITOR_H
