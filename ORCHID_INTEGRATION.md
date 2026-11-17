# Integración de DLC y expansiones de Orchid

Actualización: la compilación completa y las correcciones posteriores de sincronización/memoria están documentadas en [ORCHID_RUNTIME_IMPLEMENTATION.md](ORCHID_RUNTIME_IMPLEMENTATION.md). Las secciones de validación siguientes conservan el estado de la primera integración.

## Revisiones y alcance

Consultadas por Git el 16 de noviembre de 2026:

- Base local y punta de la rama oficial `nx2` de Eglantine: `cf6cfe9caa6e160d3562742308a38eb3c9014789`.
- Orchid `main`: `8fe9d7a7610c20202c6a35b369b6fa1439edd670`.
- Ancestro común: `46967a1b84e857a783ace6dcbabffddd008463ff`.
- Fuentes: <https://github.com/Eglantine-Community/Eglantine> y <https://gitlab.com/MythosOfs/Orchid>.

Este cambio integra selectivamente la infraestructura de DLC y ediciones sobre Eglantine. **No es una fusión completa de ambos proyectos ni un commit de merge.** La simulación de una fusión completa produjo 204 avisos de conflicto, incluidos gráficos, audio, interfaz, configuración y el reemplazo del submódulo `docs` por un directorio. Se consultó al usuario por el alcance; mientras no llega una preferencia se preparó el alcance recomendado de DLC, conforme al objetivo de Kirby. La fusión completa sigue pendiente si se elige ese alcance.

Se conservaron los cambios de Eglantine ajenos a DLC, los encabezados de licencia originales, sus funciones públicas de extracción/edición NUP y `docs/notas.md`. No se crearon commits ni se publicó nada. Las referencias `build/pr724` y `nx2-experimental` no fueron cambiadas; la integración se preparó en el checkout `work` existente.

## Cambios portados

- Registro compartido `BootContent` para contenido Program/Data, con construcción diferida de RomFS y sustitución del conjunto al cargar un título mediante el cargador de directorios.
- Enumeración AOC de contenido suelto y embebido en NUP, refrescada en Count/List; resolución de identidad DLC desde el NACP del título cargado, conservando la identidad legacy cuando el programa Switch 2 la utiliza.
- Servicio FS capaz de servir ese contenido y los programas secundarios; fallback de programa 0 cuando corresponde al camino importado.
- NUP conserva el NACP original, selecciona versiones de DLC y registra sus RomFS y programas secundarios después de cargar ExeFS. El empaquetador incorpora DLC de estructuras anidadas y programas de ediciones.
- Resolución de carpetas `program/0`, `program/1` y `program1`, composición de capas RomFS de programa/update/base, y detección de una única edición nativa en la lista de juegos.
- Pruebas de regresión de Orchid añadidas sin eliminar las pruebas y funciones existentes de Eglantine.

Procedencia de los parches: `f03fb02ced`, `4b56b7459e`, `7978d8a4b5`, `3214278e2a`, `0be650b6f6`, `40475fe054`, `15cbd07a54`, `5d00ad961b`, `1ece93009e`, `583f602aa4`, `a8734dc57c`, `8681d24e7d` (comprobación de ExeFS vacío) y `b38d3527fa`. Se aplicaron cambios por archivo y se resolvieron conflictos, no commits completos con toda su funcionalidad ajena al alcance. Se añadió la declaración de FindAddonPackages necesaria al integrar sus dependencias.

No se modificó la señalización de `aoc_change_event` ni los cuerpos STUBBED de Prepare/NotifyMount/CheckMount. El código importado no justifica señalar ese evento incondicionalmente. Esta integración tampoco importa los cambios gráficos de Orchid ni todas sus funciones de interfaz para desactivar contenido embebido.

## Validación de esta máquina

- Pasaron cuatro casos existentes de Orchid de resolución/detección de carpetas: **58 aserciones**. Se compilaron en un ejecutable independiente con Catch2 3.13.0 y los mismos cuerpos de pruebas, sin alterar aserciones. Son un subconjunto explícito de los once casos del archivo `src/tests/core/loose_directory.cpp`; no verifican montaje AOC ni ejecución de juegos.
- Pasó también la regresión existente de Eglantine `tools/integration/game_discovery_test.cpp`: layouts sueltos, exclusión de datos de juego del escaneo, Unicode, búsqueda superficial y cancelación.
- Comprobación `g++ -std=c++20 -fsyntax-only` satisfactoria de los siete archivos principales: `nup_container.cpp`, `nup_writer.cpp`, `romfs_factory.cpp`, `loader/nup.cpp`, `deconstructed_rom_directory.cpp`, `addon_content_manager.cpp` y `fsp_srv.cpp`; también de ambos archivos de pruebas NUP/directorios. Esto comprueba las unidades, no el enlace del programa.
- No quedan entradas de índice sin resolver ni marcadores de conflicto en los archivos integrados. `git diff --check` pasa.
- Configuración completa de pruebas intentada en `build-dlc-tests`, sin Qt, CLI, OpenGL, libusb ni cubeb para aislar pruebas en esta máquina. **No se completó**: la descarga fijada de `nx_tzdb` desde `git.eden-emu.dev` recibe un 403 del proxy. No se desactivaron certificados ni comprobaciones de hash. La alternativa de GitHub consultada no contiene ese artefacto.

Los registros y el ejecutable parcial están en `/workspace/dlc-validation`; la configuración fallida está en `/workspace/eglantine-dlc-configure.log`. Son artefactos locales, no forman parte del cambio de código.

## Validación pendiente

Para dar por validada la integración hay que habilitar acceso a `git.eden-emu.dev`, completar las dependencias del entorno documentadas previamente y compilar el emulador y el ejecutable completo `tests`. Ejecutar como mínimo los filtros `[nup]`, `[loose_directory]` y las regresiones gráficas pertinentes antes de una fusión completa. Las comprobaciones individuales no sustituyen esas suites.

Después se necesita una prueba acotada de Kirby con y sin DLC, usando datos autorizados del usuario: verificar Count/List/GetBaseId, aperturas FS y progreso de submit/present. No se han leído claves, dumps ni saves, ni se ha ejecutado un juego. **No se afirma que el bloqueo de Kirby esté resuelto.**
