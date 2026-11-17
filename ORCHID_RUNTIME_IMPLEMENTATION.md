# Correcciones de ejecución integradas desde Orchid

Validación del 16-11-2026 sobre Eglantine `cf6cfe9caa6e160d3562742308a38eb3c9014789` más los cambios locales. La selección parte de Orchid `8fe9d7a7610c20202c6a35b369b6fa1439edd670` y de las decisiones de ORCHID_CONFLICT_REVIEW.md. Es una integración selectiva, no un merge de todas las diferencias entre proyectos.

La integración anterior de DLC/NUP permanece; no se volvió a aplicar. La migración de partidas quedó fuera de esta tarea por indicación del usuario. No se leyeron partidas, claves ni dumps del usuario, ni se hicieron commits, pushes o publicaciones.

## Comportamiento cambiado

- **Syncpoints:** handles estables de acciones, callbacks fuera del lock y cancelación que espera callbacks en ejecución. Los incrementos se registran independientemente del contenido de la página espejo, de modo que rearmar el mismo bit puede despertar al cliente. Se conserva el orden de Eglantine: actualizar el espejo antes de publicar que el syncpoint está listo. También se libera el guard al ejecutar callbacks ya listos y ante excepciones.
- **GSV:** attach, polling y destrucción coordinados por mutex. El espejo es el único escritor de los valores; el polling consume los bits de incrementos, sin copiar valores antiguos sobre el espejo ni usar eventos todavía no creados.
- **Escrituras CPU/GPU:** una generación por recogida efectiva permite volver a notificar una página que la CPU escribe después de una recogida. Las descargas conservan las páginas con escrituras pendientes, incluso durante el intervalo entre recogerlas y aplicarlas. Se conservaron el registro de escrituras pendientes y las invalidaciones agrupadas de Eglantine; los lotes se serializan y se cierra el ciclo tanto en Vulkan como en OpenGL. Una pasada posterior de limpieza evita mantener páginas protegidas indefinidamente al quedar vacías las listas.
- **Máscaras de memoria sucia:** las escrituras se dividen al cruzar el límite representable por una máscara, se ignoran tamaños cero y se reconoce correctamente el valor centinela vacío. Se conserva el atajo para fences sin trabajo.
- **Orden de uploads:** se marca el uso de buffers obtenidos, vértices sin cambio de binding, parámetros indirectos y rangos subidos. Así, una subida posterior no puede adelantarse a operaciones que ya usan esos bytes.
- **Optimizaciones acotadas:** una región contenida en una página evita la consulta multirrango, sin sumar direcciones que puedan desbordar. Los rangos de subida exactamente repetidos no reciben hints de memoria fría; con el modo Off tampoco se ordena esa lista. La opción no se activa por defecto.
- **Texturas:** una capa sin bytes de entrada se rellena con ceros en lugar de crear un span fuera de rango.

Procedencia principal: las correcciones de syncpoints que culminan en `8fe9d7a761`, `f2eb04babd`, `225ed15fb3`, `beedd226e5` y `0bb20c94ab`. Se adaptaron sus invariantes a las APIs y protecciones actuales, conservando los backends, las optimizaciones de shaders y las APIs NUP de Eglantine.

## Compilación y arranque comprobados

Se compilaron y enlazaron `tests`, `yuzu` y `yuzu-cmd`; los programas resultantes son `bin/tests`, `bin/eglantine` y `bin/eden-cli`, bajo `build-orchid-validation`. Se hizo una segunda configuración/compilación incremental después de los últimos cambios y de corregir la identificación Git dentro del contenedor.

Entorno: Debian 13, GCC 14.2.0, Qt 6.8.2, Release con `-O2`, LTO desactivado y cuatro trabajos. Las dependencias se instalaron en Docker administrado; no se requirió sudo en el host. La receta entregada y construida está en [tools/cloud/Dockerfile](tools/cloud/Dockerfile), con instrucciones en [tools/cloud/README.md](tools/cloud/README.md).

- CLI: `eden-cli --config <INI temporal> --help`, salida 0 y texto de ayuda correcto, sin errores de configuración en la pasada final.
- GUI: arranque acotado en Xvfb con almacenamiento XDG temporal y Mesa. Se verificó una ventana principal **IsViewable**, titulada `Eglantine | cf6cfe9caa-work | GNU 14.2.0`, de 853×682. Se cerró únicamente el proceso iniciado para la prueba.

Esto verifica arranque de las interfaces; no es una prueba de emulación de un juego ni un benchmark de GPU.

## Pruebas ejecutadas con el ejecutable completo

Se usó semilla 724 y reportes JUnit nuevos por ejecución; no hubo casos omitidos en estas suites.

| Suite | Casos | Aserciones | Resultado |
| --- | ---: | ---: | --- |
| `[nup],[loose_directory]` | 43 | 652 | Pasaron todas |
| `[syncpoint_manager],[gpu_dirty_memory],[buffer_cache],[texture_row_upload],[usage_tracker],[nx2_memory]` | 26 | 84.271 | Pasaron todas |
| `[gpu_dirty_memory]` con `EDEN_NX2_PERF_OPT=0` | 4 | 24 | Pasaron todas; variante de casos anteriores |
| `[ampere]` | 81 | 60.588 | 14 aserciones fallidas en 5 casos; 76 casos sin fallos |

Las pruebas de memoria incluyen descargas síncronas y por fences que deben conservar bytes CPU más recientes, rondas superpuestas con nuevas escrituras, marcación de uso, cancelación de acciones, orden espejo/ready y máscaras que cruzan límites. Las pruebas de NUP/directorios ejercitan contenido sintético del repositorio, no archivos de juegos del usuario.

### Fallos Ampere anteriores a esta integración

Los cinco casos fallidos son:

1. `Captured Kirby high-register ALU and memory operands preserve encoding fields`.
2. `Logged Pokopia masked dependency barrier fences shared memory and rejects unknown modes`.
3. `Logged Pokopia SHF register pressure identifies the register shift operand`.
4. `Logged Pokopia F2I preserves guards and rejects unrelated modifiers`.
5. `TXD register pressure covers the layer and a single destination`.

`git diff HEAD -- src/shader_recompiler src/tests/video_core/ampere_logged_numeric.cpp` está vacío: ni el compilador ni esas expectativas se modificaron. Además, se compiló un diagnóstico independiente, sin el runtime nuevo, con `lower_to_maxwell.h` y `common_types.h` extraídos de HEAD y con sus equivalentes actuales, usando `-O2 -DNDEBUG`. Ambas versiones producen las mismas discrepancias representativas: tres variantes F2I que la prueba esperaba rechazar se aceptan; la preparación SHF y la fixture Kirby no son válidas; el caso TXD cruzando grupo se acepta; el barrier no contiene el membar esperado.

Este diagnóstico reproduce las cinco familias de discrepancias, no equivale a ejecutar otra vez los 81 casos sobre un emulador anterior completo. Permite aislarlas del código de sincronización/memoria integrado. No se cambiaron asserts, no se desactivaron casos y **la suite Ampere no se considera aprobada**. Hace falta una revisión específica de la semántica y de esas expectativas antes de afirmar validación gráfica completa.

## Datos horarios y límites de la validación

El destino fijado `git.eden-emu.dev` sigue bloqueado por el proxy. Para compilar se usó la opción soportada `YUZU_TZDB_PATH` con el paquete histórico oficial `221202` (2022g), también referenciado en el historial del repositorio. No se cambiaron `cpmfile.json`, hashes fijados ni verificación TLS.

URL: <https://github.com/lat9nq/tzdb_to_nx/releases/download/221202/221202.zip>.
SHA-256 del archivo utilizado: `991cd6fa2230ad4d73b311e67fed1102b53c052840a0432f0b3f8c7171452b77`.
La ruta de esta máquina es `/workspace/eglantine-build/tzdb-221202`.

Es un fallback de validación con reglas horarias anteriores a la dependencia `230326`, no prueba de paridad de una build de distribución. La receta documenta cómo usar datos Nintendo-format existentes sin reemplazarlos por los del sistema host.

No se probó Kirby ni Mario Kart con sus datos reales; no se afirma que el bloqueo de Kirby esté resuelto ni que mejoren los FPS. La compilación y el arranque se validaron en Linux; no se ejecutó una build Windows.

Registros de esta tarea: `/workspace/eglantine-build/build-final.log`, `test-status.tsv`, `dlc.xml`, `memory.xml`, `ampere.xml`, `dirty-no-perf.xml`, `gui-main-window.txt`, `gui.log`, `cli-help-final.log` y `ampere-baseline.txt`. Son artefactos de validación de la máquina, no datos del usuario ni instrucciones publicadas.
