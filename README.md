# Shell Text Editor

## Integrantes

* Juan Esteban Palacio Betancur
* Mateo Montoya Ospina
* Thomas Serna Saldarriaga

## Descripción

Este proyecto desarrollado en C para Linux, implementa un shell personalizado llamado `sys-shell` y un editor de texto interactivo mediante el comando `editar`, 
que permite abrir, consultar y modificar archivos planos desde la terminal.

El editor funciona completamente mediante CLI y utiliza llamadas al sistema POSIX para la manipulación de archivos, sin concurrencia.

## Requisitos

* Linux / POSIX
* GCC
* GNU Make

## Compilación y ejecución

```bash
make
./sys_shell
```

Para limpiar los archivos compilados:

```bash
make clean
```

## Comandos del Shell

|  Comando   |             Descripción             |
|:----------:|:-----------------------------------:|
|   `help`   |          Muestra la ayuda           |
|  `clear`   |         Limpia la pantalla          |
| `saludar`  |   Muestra un saludo personalizado   |
| `despedir` | Muestra una despedida personalizada |
|  `color`   |       Imprime texto con color       |
|  `clone`   |          Clona un archivo           |
|  `editar`  |           Abre el editor            |
|   `exit`   |           Sale del shell            |

## Comandos del Editor

|        Comando        |          Descripción           |
|:---------------------:|:------------------------------:|
|     `o <archivo>`     |     Abre o crea un archivo     |
|          `q`          |        Cierra el editor        |
|      `p [línea]`      | Imprime el archivo o una línea |
|     `s "<texto>"`     |          Busca texto           |
|          `m`          |       Muestra metadatos        |
|     `a "<texto>"`     |   Agrega una línea al final    |
| `i <línea> "<texto>"` |       Inserta una línea        |
|      `d <línea>`      |       Elimina una línea        |
|      `y <línea>`      |        Copia una línea         |
|      `x <línea>`      |         Pega una línea         |

## Syscalls utilizadas

El proyecto utiliza principalmente:

```text
open()
close()
read()
write()
lseek()
fstat()
ftruncate()
getuid()
link()
```

Estas permiten realizar operaciones de apertura, lectura, escritura, posicionamiento, consulta de metadatos, modificación de archivos y creación de enlaces físicos.

## Estructura

```text
src/
├── main.c
├── shell.h
├── cat_entrenamiento.c
├── cat_editor.c
└── editor/
    ├── editor.c
    ├── editor.h
    ├── editor_basic.c
    ├── editor_inspection.c
    ├── editor_modify.c
    └── editor_clipboard.c
```