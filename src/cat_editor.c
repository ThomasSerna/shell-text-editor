#include "shell.h"
#include "editor/editor.h"

int cmd_editar(int argc, char **argv) {
    (void)argc;
    (void)argv;

    return editor_main();
}