# Memoria de procesos: ampliación prevista

Pendiente de implementar. El inspector tabular funciona actualmente con archivos.

Añadir una fuente de datos de proceso, separada de la fuente de archivo: lista de procesos,
mapa de regiones con direcciones virtuales y permisos, lectura paginada y volcados de
regiones seleccionadas. Los huecos ilegibles deben conservarse en un mapa del volcado;
no se presentarán como bytes cero reales. Los permisos y APIs son específicos de cada
sistema (macOS, Windows y Linux).

Actualizar periódicamente solo las regiones visibles y las observadas por el inspector,
en segundo plano, con frecuencia configurable y pausa manual. No asumir que el sistema
notifica cada escritura: el sondeo puede perder cambios entre dos lecturas. Mostrar
hora de la última lectura y cambios detectados, e invalidar la caché correspondiente.
Una captura de varias regiones no garantiza un instante coherente del proceso.

Antes de escribir, comparar los bytes actuales con la captura usada para editar;
si cambiaron, mostrar conflicto. Separar cambios pendientes de datos recién leídos,
permitir cancelarlos y no reutilizar el guardado de archivos como escritura de memoria.
Gestionar salida del proceso y reutilización del PID; no continuar sobre otro proceso.
