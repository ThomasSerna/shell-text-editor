#include "shell.h"
#include "editor/editor.h"

int cmd_editar(int argc, char **argv) {
    return editor_main(argc, argv);
}