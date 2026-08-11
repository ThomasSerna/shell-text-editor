#include "shell.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <errno.h>
#include <time.h>

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
