# T1: Kirby y montaje de DLC

## Alcance y procedencia

Análisis estático local del 16-11-2026. No se usó internet ni se leyeron claves, dumps o saves. La única escritura solicitada es este documento; no se modificó código ni se ejecutó el emulador.

Existe `AGENTS.md`. **No existe `docs/AGENTS-eglantine-dlc.md` en el checkout actual.** Las políticas referidas en AGENTS no están en `docs/policies/*.md`; se consultaron sus equivalentes disponibles en `docs/content/policies/AI.mdx` y `Coding.mdx`.

Solo existe la referencia local `work`, HEAD `cf6cfe9caa6e160d3562742308a38eb3c9014789`. No están disponibles `build/pr724` ni `nx2-experimental`. Por tanto, estas conclusiones describen este SHA, no verifican la PR #724. No se ha comparado con el repositorio oficial ni integrado ninguna mejora. `docs` es un submódulo: este archivo queda dentro de él.

Contexto aportado por el usuario, no reproducido aquí: programa `04004D300C5AE001`, DLC `01004D300C5AF001`, NACP `dlc_base_title_id=01004D300C5AF000`; PrepareAddOnContent(1) dos veces, Serving loose add-on content, NotifyMountAddOnContent y CheckAddOnContentMountStatus; después consulta de idioma cada ~100 ms y ausencia de actividad GPU. Sin DLC arranca; Mario Kart World funciona con el mismo build. Las líneas AOC aportadas (184/208/220) difieren de este SHA. La cadena «Serving loose add-on content» no se encontró en los archivos de implementación de filesystem examinados; no se puede atribuir a este checkout el camino exacto que generó ese log.

## (a) Estado y evento tras CheckAddOnContentMountStatus

Referencias relativas a la raíz del repositorio, con líneas de este SHA:

- `src/core/hle/service/aoc/addon_content_manager.cpp:166–170`: PrepareAddOnContent registra índice/PID y devuelve éxito. No comprueba disponibilidad, prepara contenido ni registra una transición.
- Mismo archivo, `191–206`: NotifyMountAddOnContent, NotifyUnmountAddOnContent y CheckAddOnContentMountStatus solo registran STUBBED y devuelven éxito. Sus firmas no reciben argumentos ni devuelven un estado adicional; no hay tabla de montajes ni comprobación de errores en estos cuerpos.
- `92`, `173–188`: se crea `aoc_change_event` con nombre `GetAddOnContentListChanged:Event`; ambos getters entregan su handle legible. `95–96` lo cierra. No hay llamada a Signal sobre ese evento en esta implementación.
- `src/core/hle/kernel/k_readable_event.cpp:19–23,44–49`: el evento se inicializa no señalado; Signal cambia ese estado y notifica disponibilidad a los esperadores. Devolver éxito de un IPC no equivale a señalar un evento.

**Conclusión observable:** después de CheckAddOnContentMountStatus, el invitado recibe éxito y no se produce ninguna transición de montaje ni señal AOC explícita. No se puede deducir del servidor qué hará el juego después. Es hipótesis que su SDK espere un montaje validado, un error concreto o una notificación posterior; el contrato real no está implementado en estos métodos.

**¿Debería señalarse `aoc_change_event`?** Su nombre y API lo vinculan al cambio de lista de add-ons, no demuestran que sea un evento de finalización del montaje. Si la lista observable cambia y existe un consumidor esperando esa notificación, señalizarlo sería una hipótesis razonable de comportamiento pendiente. No hay evidencia local suficiente para recomendar señalarlo incondicionalmente en Check o NotifyMount. Montar un DLC ya enumerado no demuestra por sí mismo un cambio de lista. Antes de un parche hay que identificar adquisición del handle, espera efectiva y transición que debería despertarla. Un evento nunca señalado es un mecanismo posible de bloqueo, no la causa probada de este caso.

La enumeración también merece distinguirse del montaje: `addon_content_manager.cpp:42–57,60–62` captura los AOC disponibles al construir el servicio; Count/List filtran por base (`99–144`). GetAddOnContentBaseId usa el NACP cuando existe y solo usa el cálculo derivado si falta (`147–163`). No existe aquí una relación automática entre servir un filesystem y cambiar el estado AOC.

## (b) Diferencias entre nx2:gpu y nx2:vi2

No se proporcionaron los IDs de los comandos exclusivos de Kirby ni trazas comparables de Mario Kart. **No puede establecerse cuáles fueron exclusivos**, ni que sean la causa. El número de comando debe interpretarse junto con el servicio y el tipo TIPC/CMIF; por ejemplo, 202 tiene significados distintos en cada servicio.

| Servicio/camino local | Comportamiento en este SHA | Implicación para investigar |
| --- | --- | --- |
| `nx2:gpu`, tabla `nx2_gsv.h:335–365` | Gestiona memoria/espacios GPU, dispositivos y sesiones worker. 23/24/25/34/51/8001–8004 usan StubReplyZeroed. | Un éxito con ceros no garantiza capacidades, IDs o efectos laterales válidos. La semántica de los Unknown permanece desconocida. |
| `nx2:gpu` 11/12/21/22, `390–418,486–489` | Rango de 32 eventos; 12 devuelve evento por ID; 21 sí lo señala; 22 devuelve otra sesión del mismo objeto. | STUBBED no significa siempre «no hace nada»: aquí hay handles y señalización reales. No es el evento AOC. |
| `nx2:gpu` 33/201/202/203, `421–455` | Respuestas específicas: ID 1, partición de usuario, partición GSV vacía y rango VA. | Las respuestas tienen forma y valores deliberados; no equivalen al stub genérico. Los comentarios sobre SDK son observaciones del repositorio, no validación nueva con Kirby. |
| `nx2:vi2`, `1695–1772` | Sesión de pantalla: consultas, eventos, buffers y presentación. Numerosos Unknown usan StubReplyZeroed, incluyendo 13/51/101/102/107/108/111–113/202/230–232/251/252 y otros de la tabla. | Ceros pueden significar ausencia de capacidad o de disponibilidad; el significado de Unknown no está demostrado. |
| `nx2:vi2` 11 frente a 31/32/261 y 263/265/267/269/271, `2742–2859` | 11 crea un evento sin señalizarlo en ese cuerpo. 31/32 reutilizan eventos nativos; el temporizador señala vsync; BindChange señala una vez y desliga del vsync. | Diferencia material entre devolver un handle y conectar una fuente de señales. El comentario de BindChange menciona Kirby Air Riders reconsultando HDR con cada present; no lo vincula al DLC ni al bucle de idioma. |
| `nx2:vi2` 103/212/229, `2674–2679,2700–2705,1811–1817` | Devuelve refresco 60 Hz, slots disponibles 0x7fff y procesa presentación con contador. | Hay implementación parcial real; «sin GPU» requiere separar ausencia de submit de ausencia de present o espera en fence. |

Detalle de registro: `nx2_gsv.h:1750–1761` repite 261–272 como stubs después de los handlers de eventos. `service.cpp:145–149` usa emplace_hint en el mapa, conservando la primera entrada para una clave existente: esos duplicados no sustituyen los handlers registrados antes (`1712–1723`). No deben clasificarse como stubs activos solo por leer la segunda lista.

`src/core/hle/service/service.cpp:108–115` devuelve éxito y palabras cero para StubReplyZeroed. Para comandos sin handler, `153–183` permite auto-stub por prefijo NX2 o proceso Switch 2, registra cada llamada pero suprime avisos repetidos por servicio/comando. La frecuencia de avisos visibles no equivale a frecuencia de llamadas. Una comparación útil necesita servicio, ID, orden, resultado y presencia de handles/eventos, sin payloads sensibles.

## (c) Hipótesis y pruebas discriminantes

Todas las causas de esta tabla son **hipótesis**, no diagnósticos confirmados. Los datos nuevos propuestos son metadatos de ejecución; no requieren leer claves, dumps ni saves y no se recogieron en esta T1.

| Hipótesis | Evidencia a favor | Evidencia en contra o límite | Dato nuevo que la confirmaría o descartaría |
| --- | --- | --- | --- |
| El invitado espera una notificación AOC que nunca llega. | Evento inicial no señalado y sin Signal en el servicio; solo falla con DLC según usuario. | No consta adquisición ni espera de ese handle; ListChanged no acredita finalización de montaje. | Correlación del handle AOC con una espera y su timeout, hilo y momento. Sin espera dependiente de ese handle se descarta este mecanismo concreto. |
| El éxito incondicional oculta preparación/montaje incompleto. | Los tres cuerpos no validan ni mantienen estado. | El usuario observa filesystem servido y montaje notificado; podría bastar ese éxito para el SDK. | Resultados y orden de operaciones FS posteriores, tamaños solicitados/devueltos y errores, sin contenidos. Un fallo reproducible tras abrir el DLC lo apoya; completar esas operaciones y bloquearse en otro recurso lo debilita. |
| Identidad/base/índice del DLC incoherentes entre capas. | Count/List comparan bases derivadas mientras GetBaseId usa NACP; existen dos espacios de ID aportados. | Índice 1 y base NACP + 1 coinciden con DLC aportado; se sirve contenido. No prueba selección errónea. | IDs efectivos de caller, Count/List/GetBaseId y título resuelto al abrir FS. Coincidencia completa descarta esa discrepancia concreta. |
| El idioma o los metadatos provocan el reintento. | La consulta de idioma es el síntoma repetido aportado. | `read_only_application_control_data_interface.cpp:190–221` acepta explícitamente supported_languages=0 y retorna el primer idioma prioritario si la configuración se convierte correctamente. No monta DLC ni espera eventos. | Resultado efectivo e idioma devuelto y siguiente IPC del mismo hilo. Éxito estable descarta «cero implica error», aunque no un fallo posterior del consumidor. |
| Una respuesta incompleta de nx2:gpu bloquea inicialización/worker. | Hay Unknown con ceros y contratos parcialmente emulados. | Mario Kart funciona; no se aportaron IDs exclusivos ni prueba de dependencia con DLC. El worker 21 sí señala su evento. | Comparación Kirby con/sin DLC del primer comando divergente, respuesta y espera asociada. Si el bloqueo antecede a ese camino, se descarta como disparador. |
| Una espera de pantalla/vsync/capacidad nx2:vi2 impide arrancar GPU. | Hay eventos y respuestas de capacidad; comentarios locales documentan esperas antes del primer submit (`2767–2771`) y consultas de refresco que pueden impedir dibujar (`2674–2677`). | No son evidencia nueva del bloqueo DLC de Kirby; eventos nativos ya tienen temporizador y BindChange tiene señal única. | ID exacto consultado, handle esperado y recuento de señales/submit/present. Eventos entregados y consumidos con avance posterior descartan esa espera específica. |
| El hilo que consulta idioma sigue vivo pero otro hilo/recurso mantiene el bloqueo. | Cadencia ~100 ms es compatible con un reintento o temporizador; correlación temporal no demuestra causalidad. | No hay estados de hilos ni dependencias observadas en T1. | Estados y motivos de espera por hilo, IDs de objetos y contadores de progreso sin memoria ni contenidos. Un bloqueo identificado en otro recurso lo confirma; flujo completo avanzando lo descarta. |

Para la hipótesis de IDs: `src/core/file_sys/common_funcs.h:10–12,20–21,41–52` aplica máscara base `0xFFFFFFFFFFFFE000`, offset `0x1000` y máscara de índice `0x7FF`. Aplicadas a los IDs aportados, las bases quedan `04004D300C5AE000` para el programa y `01004D300C5AE000` para el DLC: no coinciden por esos helpers. Esto es una discrepancia aritmética comprobable en este SHA, **no prueba de fallo en la PR**: hay que verificar normalización y caller efectivo en su rama. NACP + índice sí produce `01004D300C5AF001`.

Prioridad sugerida para una fase posterior: disponer del código local exacto de la PR y sus instrucciones DLC; correlacionar resultados AOC/NS/FS y esperas con/sin DLC; después identificar los IDs NX2 divergentes. Solo entonces evaluar qué mejoras oficiales aplicar. Esta T1 no propone señalar eventos a ciegas ni realiza cambios de implementación.
