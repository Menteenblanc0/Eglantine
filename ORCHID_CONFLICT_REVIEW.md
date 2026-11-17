# Decisiones sobre los conflictos Eglantine–Orchid

Las correcciones seleccionadas ya implementadas y sus resultados de validación se detallan en [ORCHID_RUNTIME_IMPLEMENTATION.md](ORCHID_RUNTIME_IMPLEMENTATION.md). Este documento conserva la investigación que guio la selección.

Investigación del 16-11-2026. Eglantine oficial `nx2` sigue en `cf6cfe9caa6e160d3562742308a38eb3c9014789`; Orchid `main` sigue en `8fe9d7a7610c20202c6a35b369b6fa1439edd670`. Se comprobaron ambas puntas mediante `git ls-remote`. Ancestro común: `46967a1b84e857a783ace6dcbabffddd008463ff`.

## Decisión principal

Mantener Eglantine como base, conservar la integración de expansiones ya publicada y trasladar correcciones de Orchid por conjuntos coherentes con sus pruebas. No sustituir los subsistemas completos ni resolver sus conflictos seleccionando un lado para todo el archivo. El soporte de DLC depende de identidad, cargadores y filesystem; importar todos los cambios gráficos de Orchid no lo demuestra ni es un requisito.

La investigación cubre los **204 archivos** de la simulación guardada en `/workspace/orchid-merge-preview.txt`. El inventario [ORCHID_CONFLICTS.csv](ORCHID_CONFLICTS.csv) asigna una decisión de integración a cada ruta. Se revisaron semánticamente los puntos de mayor riesgo descritos abajo; la clasificación por subsistema del resto no equivale a certificar cada hunk ni a haber resuelto un merge completo.

No hay merge activo ni entradas del índice sin resolver. Esos 204 conflictos pertenecen a una simulación entre las dos puntas, no al estado actual del índice. En esta tarea no se volvieron a aplicar parches anteriores, no se modificó `src`, no se movieron notas ni se publicó nada. La compilación y las pruebas descritas en ORCHID_INTEGRATION.md corresponden a la tarea anterior, no a una ejecución nueva.

## D1. Estructura, instrucciones y distribución: conservar Eglantine

Los conflictos incluyen AGENTS, README, plantillas, release-binaries, SCM, packaging y `docs`. Orchid convierte `docs` en directorio mientras Eglantine lo mantiene como submódulo: el aviso que propone `docs~HEAD` es un choque de tipos de entrada Git, no una corrección de emulación. Se conserva el submódulo y `docs/notas.md`; la documentación útil se adapta sin reemplazarlo.

También se conservan la marca, los destinos de actualización, las reglas de publicación y las instrucciones de Eglantine. No importar la autorización de push que aparece en el AGENTS de Orchid: Eglantine prohíbe hacerlo. No hay beneficio de FPS o de DLC que justifique estos reemplazos.

## D2. Expansiones y NUP: combinar comportamiento y conservar contratos

Se conserva lo ya integrado: BootContent Program/Data, RomFS diferido, NACP auténtico, enumeración AOC actualizada y resolución de programas secundarios. No se vuelve a aplicar.

Evidencia del árbol actual:

- `src/core/loader/nup.cpp:56–61` conserva el NACP original si existe; `207–225` registra DLC y programas secundarios después de cargar ExeFS.
- `src/core/hle/service/aoc/addon_content_manager.cpp:39–58` obtiene la identidad AOC del NACP del cargador, con fallback a metadatos instalados y cálculo derivado.
- `src/core/file_sys/romfs_factory.cpp:83–152` mantiene un conjunto de contenido, construye RomFS al abrirlo y sirve contenido Data y Program.
- `src/core/loader/deconstructed_rom_directory.cpp` y `loose_directory.h` resuelven las estructuras de ediciones; no se sustituirán por versiones que pierdan estos caminos.

Conflicto importante pendiente de una fusión total: `nup_writer.h`. Orchid usa PackResult para extracción y tiene otro contrato de edición; Eglantine ya expone ExtractResult, EditResult, MetadataEdit, editor de títulos localizados y publisher. **Decisión: conservar esas APIs públicas y adaptar los cambios de datos de Orchid detrás de ellas.** Reemplazar el header completo rompería sus consumidores de GUI/CLI y sus pruebas. Lo mismo para patch_manager: conservar sus parches y preferencias, sin importar automáticamente otros parches de FPS por juego.

Para el fallback de programa 0 se conserva primero la búsqueda del programa real; el stand-in sigue siendo compatibilidad provisional. Que abra un RomFS no demuestra que los datos sean los que espera todo juego. Requiere pruebas de edición con programa 0 presente, ausente y con datos separados.

No añadir Signal incondicional a aoc_change_event: ListChanged no está demostrado como evento de finalización de montaje. El fallo de Kirby aún necesita correlacionar resultados FS/AOC y esperas; no se considera corregido por la mera enumeración del DLC.

Pruebas exigidas antes de dar esta capa por validada: suite completa `[nup]`, `[loose_directory]`, conservación de NACP y metadatos tras empaquetar/extraer/editar, selección de versión, perfil con DLC desactivado, programa secundario y arranque acotado con/sin expansión. El subconjunto previo de cuatro pruebas no cubre todo ello.

## D3. Syncpoints: aceptar el conjunto completo, no un parche aislado

Orchid `ed368ec364` actualiza el espejo de valores al incrementar; `8fe9d7a761` corrige después la detección de cambios: la página ya actualizada no sirve para decidir si hay que despertar una espera rearmada. Añade TakeMovedGuest y bits atómicos de cambios. También hay cambios de cancelación y callbacks fuera del guard en `syncpoint_manager.*`.

En Eglantine actual, `nx2_gsv.h:4002–4021` llama a UpdateSyncptValues y solo señala al cambiar los bits de reconocimiento. No contiene TakeMovedGuest. **Decisión: seleccionar la cohorte de espejo + cambios + rearme + ciclo de vida/cancelación y su prueba de syncpoints**, adaptándola al GSV actual; no reemplazar las ~3.000 líneas diferentes del header. Revisar orden de locks para evitar inversión entre espejo, polling y destrucción.

El historial de Orchid describe un bloqueo introducido por importar solo una parte. Es evidencia de dependencia y motivo para revisar la cohorte, no reproducción nuestra ni prueba de que explique el DLC de Kirby.

Validación necesaria: incrementos antes/después de armar, rearmar el mismo bit, cancelación durante callback, acciones que cancelan otras, destrucción de sesión, IDs inválidos y wraparound. Después, medir despertares y latencia sin aumentar polling arbitrariamente.

## D4. Coherencia CPU/GPU: aceptar por corrección, con todas las dependencias

Seleccionar para adaptación conjunta `f2eb04babd` y `225ed15fb3`: protegen escrituras CPU pendientes de descargas GPU obsoletas y marcan usos que impiden reordenar uploads incorrectamente. Afectan GPUDirtyMemoryManager, memory.cpp, GPU, interfaz de rasterizer, caché de buffers, Vulkan y tests. **No portar solo buffer_cache.h.**

La descripción de Orchid informa mejoras visuales y pruebas A/B en Yoshi; no son mediciones realizadas aquí. La justificación técnica es no sobrescribir escrituras recientes ni alterar el orden de copias dependientes, antes que perseguir FPS. Conservar las optimizaciones de Eglantine al adaptar esas invariantes.

Validar escrituras concurrentes con gather/apply, readback parcial, descargas por fence, remapeos sparse y uploads a rangos coincidentes. Exigir pruebas de las dos arquitecturas de uso cuando el proyecto las soporte, no solo una ejecución monohilo.

## D5. Optimización: dos candidatos acotados y una duplicación descartada

| Cambio | Evidencia comparada | Decisión |
| --- | --- | --- |
| `beedd226e5`: evitar consulta multirrango dentro de una página de 4 KiB | `buffer_cache.h:1888–1900` ya excluye rangos vacíos y nulos, pero consulta virtual_ranges aun para una sola página. | Aceptar como candidato pequeño, adaptando cálculo de fin y verificando overflow. Probar límites de página, remapeos y buffers escritos; medir CPU por draw. No hay mejora porcentual medida aquí. |
| `0bb20c94ab`: no marcar fríos rangos reescritos | Eglantine llama HintCold por cada rango a los 600 frames (`buffer_cache.h:156–160`, `texture_cache.h:652–656`); Orchid agrupa duplicados. | Aceptar solo manteniendo el control actual de cold hints. `device_memory_manager.inc:199–201` ya retorna si están desactivados: con Off el beneficio de paginación no aplica. Medir también el coste del ordenamiento; no activarlo por defecto para justificar la optimización. |
| `327d5bf974`: pausar contador de oclusión en submissions | Eglantine ya usa CounterClose en `vk_scheduler.cpp:519–523` y limita acumulación de bancos en CloseCounter de vk_query_cache.cpp. | No volver a portarlo. Preservar la implementación existente; su aparición en la historia de Orchid no significa que falte en Eglantine. |
| `c03f5a1ea3`: aumentar caché virtual a 65.536 entradas | El tamaño es una política de caché, no una corrección de identidad o de montaje. | Aplazar hasta medir hit rate, memoria y coste de invalidación en este motor. No aceptar el aumento por la cifra sola. |

No declarar que Orchid es globalmente más rápido. La comparación debe fijar hardware, driver, resolución, perfil, build y escena; separar caché fría/caliente, p50/p95/p99 de tiempo de frame, compilación de pipelines, CPU, GPU y pico de RAM/VRAM. Medir juego base y expansión, además de Mario Kart World como regresión ya mencionada por el usuario.

## D6. Shaders, DLSS, texturas y pipelines: conservar la base gráfica

La diferencia entre ambas puntas incluye ~3.700 líneas afectadas solo en lower_to_maxwell.h, y miles en cachés. Se conservan los caminos Eglantine y se adaptan correcciones de Orchid una a una con sus consumidores y tests.

Ejemplos concretos de pérdidas al elegir Orchid completo:

- `buffer_cache/usage_tracker.h` pierde los guards de tamaño cero y la optimización de intervalos ya rellenados de Eglantine; Orchid vuelve a recorrer páginas. Decisión: conservar la base y revisar los casos nuevos contra sus invariantes.
- `vk_pipeline_cache.cpp` cambia las dependencias de precache, diagnósticos y optimización; el historial Eglantine incluye correcciones propias de compilación asíncrona. Decisión: no borrar esos caminos para introducir vk_shader_reuse.
- `engines/native_upscale.h` presenta APIs incompatibles. Decisión: conservar integración Eglantine de DLSS y seleccionar helpers de regiones/formatos de Orchid solo al adaptar todos sus consumidores. No reemplazar upscaling nativo por un stand-in como supuesto requisito de expansión.
- `texture_cache/memory_budget.h` contiene políticas diferentes de umbrales y feedback shadows. Decisión: conservar límites para tarjetas pequeñas y márgenes de Eglantine; evaluar el límite de sombras como cambio separado con prueba de imágenes en uso. No subir el presupuesto indiscriminadamente.

Validar Ampere, salida de shaders, atómicos, sparse, invalidación de código, frames y memoria con capturas autorizadas en una futura fase. No se han leído capturas ni dumps en esta investigación.

## D7. Interfaz, contenido y saves: mantener Eglantine y hacer visible la identidad real

Mantener GUI, CLI, actualización y editor NUP de Eglantine. Seleccionar la descripción de contenido embebido de Orchid como evolución futura, adaptando ReadEmbeddedPatches, PatchSource y el modelo de la pestaña Add-ons conjuntamente; mostrar un checkbox que el cargador ignora sería una regresión. Una fila informativa no debe simular que la edición está desactivada.

Para saves, priorizar diagnóstico de ruta efectiva: programa, propietario NACP, atributo de apertura y perfil. El menú actual construye la ruta con el program_id de la fila; puede diferir del atributo pedido por el invitado. La propuesta de migración está en [KIRBY_SAVE_MIGRATION.md](KIRBY_SAVE_MIGRATION.md). No añadir un alias global 0400→0100, ni compartir el original con dos versiones potencialmente incompatibles.

## D8. Compilación y pruebas: unión adaptada, no sustitución

Conservar CMake/CI de Eglantine y sumar solo fuentes seleccionadas. En los 17 archivos de pruebas conflictivos, reconciliar los casos con las APIs finales: una unión textual puede duplicar TEST_CASE o helpers. Conservar la cobertura existente y añadir casos Orchid; no borrarla para obtener un build verde.

La descarga nx_tzdb bloqueada y dependencias de sistema pendientes constan en el informe anterior. El snapshot publicado no demuestra por sí solo que esos bloqueos se hayan corregido; aquí no se intentó una compilación nueva porque esta fase es de investigación. Tampoco se solicitan nuevas credenciales para un fallo de dominio.

## D9. CPU, Windows, audio, red y servicios restantes

Conservar Eglantine en hilos, presets, kernel, red y audio salvo una corrección concreta necesaria para la cohorte elegida. `common/thread.cpp` de Orchid elimina una gran sección de la planificación de hilos de Eglantine, incluidas funciones relacionadas con CPUs híbridas: no hay medición que justifique descartarlas. El commit diferido de memoria Windows de Orchid cambia reserva, commit y errores; evaluarlo separadamente en Windows con pruebas de crecimiento y agotamiento. Un runner Linux no lo valida.

En servicios AM se revisan firmas IPC por función: p. ej. 320 tiene interpretación más específica en Orchid y 230 pasa de IsTensorRtSupported a stub. No sustituir esa tabla completa: la segunda diferencia podría cambiar el camino DLSS. Audio/codecs, sockets y SSL no son requisitos demostrados del montaje local de DLC; se conserva el comportamiento actual hasta hallar una regresión concreta.

## Orden de implementación decidido

1. Validar completamente la integración DLC existente y diagnosticar la identidad de guardado de Kirby; no reimportar sus parches.
2. Integrar y probar la cohorte de syncpoints y después la de coherencia CPU/GPU, cada una con comparación independiente.
3. Aplicar la optimización de página única; evaluar cold hints manteniendo Off como comportamiento predeterminado vigente.
4. Incorporar diagnóstico/importación de saves y descriptores de contenido embebido con pruebas de extremo a extremo.
5. Evaluar cambios gráficos mayores solo con regresiones y mediciones comparables. Si no aportan una corrección o beneficio demostrado, conservar Eglantine.

Estas son decisiones de selección y resolución, no cambios implementados en esta tarea. Ninguna métrica o compatibilidad de juego nueva queda certificada por el informe.
