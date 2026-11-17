# Reutilizar la partida de Kirby de Switch 1 en Switch 2

## Qué se puede concluir ahora

Es razonable intentar conservar la partida anterior mediante la identidad de guardado correcta y una copia independiente. **No está confirmado que el juego Switch 2 del usuario acepte su formato**, ni que el bloqueo con DLC permita llegar a la pantalla de carga. No se ha abierto, listado ni modificado ningún save, clave o dump del usuario.

El contexto parece corresponder a Kirby and the Forgotten Land y su edición ampliada; esto se infiere de los IDs y de la expansión mencionada. El ID de programa aportado es `04004D300C5AE001`; el DLC es `01004D300C5AF001`; el NACP aportado solo especifica `dlc_base_title_id=01004D300C5AF000`. **Ese dato no demuestra cuál es save_data_owner_id.** El origen de la partida —emulador, consola o exportación— se consultó por separado; los pasos se distinguen abajo para no asumirlo.

## Evidencia del emulador

| Dato | Código | Consecuencia |
| --- | --- | --- |
| El propietario declarado del save no es necesariamente el programa que arranca | `src/core/file_sys/control_metadata.cpp:176–181`: GetTitleId devuelve save_data_owner_id y GetDLCBaseTitleId devuelve otro campo. | No deducir el destino del save a partir del ID del DLC. |
| La ruta usa el ID del atributo de save; si es cero, usa el programa | `src/core/file_sys/savedata_factory.cpp:104–134`; Create/Open en 64–82. | El destino real depende de lo que pide el juego y del perfil; preservar NACP no implementa por sí solo una redirección. |
| Layout tradicional | Mismo archivo, 131–134. | Bajo SaveDir: `user/save/0000000000000000/<UID32>/<TITLEID16>`. Account usa el usuario; Device puede usar UID cero. |
| Layout alternativo, preferido solo si ya existe | Mismo archivo, 26–50 y 115–123. | Para Account: `user/save/account/<UUID>/<TITLEID_SIN_BYTE_BAJO>/0`; para Device: `user/save/device/<TITLEID_SIN_BYTE_BAJO>/0`. No se crea o elige siempre. |
| El botón de abrir carpeta usa program_id de la lista | `src/yuzu/main_window.cpp:2957–3002`. | Es una ayuda inicial, no prueba definitiva de la ruta que el invitado abre cuando usa un propietario legacy explícito. |
| EnsureSaveData crea usando el programa del applet | `src/core/hle/service/am/service/application_functions.cpp:168–179`. | Puede existir una carpeta nueva vacía además de la que el juego desea abrir; su existencia no demuestra que sea el save utilizado. |
| La integración NUP conserva el NACP original | `src/core/loader/nup.cpp:56–61`. | Evita perder propietario, flags e identidad al sintetizar metadatos; no convierte los archivos guardados. |

La ruta SaveDir predeterminada apunta al NAND del perfil del emulador (`common/fs/path_util.cpp:175–179`), pero puede estar personalizada o ser portable. Usar las carpetas abiertas desde la instancia correcta evita suponer `%APPDATA%`, rutas Linux o el perfil de otro emulador.

Se compararon las dos puntas: `savedata_factory.cpp`, `control_metadata.cpp` y `save_data_controller.cpp` no tienen diferencias entre Eglantine cf6cfe9c y Orchid 8fe9d7a7. **Orchid no añade en esos caminos una migración automática Switch 1→Switch 2 ni un convertidor específico de Kirby.** Su soporte de expansión y su conservación del NACP son problemas relacionados pero distintos.

## No basta con renombrar el Title ID

Con el ID aportado, el layout tradicional podría terminar en `04004D300C5AE001` si ese es el atributo solicitado o el fallback del programa. El layout alternativo busca `04004D300C5AE000/0`, al limpiar el byte bajo; ese cambio no transforma `0400` en `0100`.

`01004D300C5AE000` es un candidato a identidad legacy de la aplicación, **no un propietario confirmado por esta investigación**. Si el invitado solicita ese propietario y usa el mismo perfil y almacenamiento, la partida podría encontrarse ya sin moverla. Si solicita la identidad Switch 2, hará falta una importación al destino efectivo. Los directorios `01004D300C5AF000` y `01004D300C5AF001` identifican add-on en el contexto aportado: no deben elegirse como destino del save por ese motivo.

Tampoco hay un convertidor universal de saves por cambiar el prefijo. El juego puede mantener el mismo formato, importar uno anterior, ampliar un formato existente o usar otro incompatible. Una partida actualizada podría no ser legible por el juego anterior. No se propone editar bytes, checksums ni NACP para forzar que pase.

## Procedimiento propuesto si la partida está en un emulador o ya exportada

1. Identificar la instancia que conserva la partida y su perfil. Abrir su carpeta de guardado desde el menú del juego. No cambiar ni borrar el origen.
2. Con los emuladores cerrados, guardar una copia completa independiente del origen y, si existe, del destino. Conservar todos los archivos y subcarpetas de la partida como conjunto.
3. Usar un perfil o almacenamiento de prueba separado para la edición Switch 2. Cuando pueda arrancar, permitir que cree un save de prueba, cerrar el emulador y determinar el destino efectivo. Si el menú y el juego discrepan, comprobar únicamente metadatos de apertura: tipo de save, program_id solicitado, perfil y ruta resuelta. No hace falta registrar el contenido.
4. Si ya busca la identidad legacy en el mismo almacenamiento, probar primero una copia del entorno/perfil que contiene esa partida; no renombrar a ciegas. Si busca otra ruta y el juego ofrece importación propia, preferirla. En caso contrario, probar una copia íntegra de la partida exportada en el destino de prueba correcto, sin mezclarla con restos de un save vacío.
5. Arrancar y comprobar en el juego el archivo correcto, progreso, coleccionables y acceso a la expansión. Guardar, cerrar y volver a abrir para verificar persistencia. Mantener el original de Switch 1 separado incluso si funciona.
6. Si el juego avisa incompatibilidad/corrupción o no reconoce el progreso, detener la prueba y restaurar solo el destino de prueba desde su respaldo. No repetir renombrados sobre el original. Habría que investigar la versión/formato o el mecanismo de importación del juego.

No se ejecutaron estos pasos. La guía local `docs/content/user/ImportingSaves.mdx` documenta la importación por Open Save Data Location y advierte que un juego puede usar varios archivos; el procedimiento anterior añade aislamiento para evitar compartir una partida entre ediciones. Si la aplicación está bloqueada antes de cargar saves, primero se resuelve ese bloqueo: copiar una partida no sustituye la validación de DLC.

## Si la partida está únicamente en una consola Switch 1

Hay dos destinos diferentes:

- **Consola Switch 2:** investigar el mecanismo oficial de transferencia de datos/partidas y la compatibilidad de esta edición concreta. No equivale a renombrar carpetas del emulador. La transferencia de consola no entrega automáticamente un save importable por Eglantine.
- **Eglantine/Orchid:** se necesita una exportación de la propia partida en formato utilizable por el emulador. Copiar una carpeta cualquiera de la microSD o los datos cifrados de la consola no basta. No se ha solicitado ni inspeccionado material de claves; el origen debe permanecer intacto.

Intenté consultar la [ficha oficial de Kirby Switch 2 Edition](https://www.nintendo.com/us/store/products/kirby-and-the-forgotten-land-nintendo-switch-2-edition-plus-star-crossed-world-switch-2/), la [guía de transferencia de Nintendo](https://www.nintendo.com/us/gaming-systems/switch-2/transfer-guide/) y la [página de ediciones Switch 2](https://www.nintendo.com/us/gaming-systems/switch-2/games/nintendo-switch-2-edition-games/). El proxy devolvió 403 al establecer el túnel a www.nintendo.com. Son referencias pendientes de consulta, **no fuentes verificadas que permitan afirmar compatibilidad específica, transferencia inversa o condiciones de Nintendo Switch Online**. No se sorteó esa restricción.

## Decisión de implementación futura

Proponer un asistente de importación que muestre programa, propietario NACP, usuario y ruta efectiva; use copia transaccional a un destino separado, rechace sobrescrituras accidentales y deje un respaldo identificable. La comprobación de formato seguirá siendo específica del juego o de su importador.

No implementar un alias global entre `0400…` y `0100…`, no redirigir todas las ediciones hacia el original y no generar un save de progreso inventado. Antes de escribir código de migración para Kirby falta identificar su save_data_owner_id efectivo y comprobar en una copia si acepta la partida anterior. El objetivo es conservar tu progreso; con los datos actuales no se puede prometer todavía que no haga falta una conversión específica.
