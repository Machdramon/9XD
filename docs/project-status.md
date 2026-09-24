# Revisión rápida del proyecto · 2026-09-24

Revisión del código y documentación, con pruebas automatizadas locales en macOS ARM64.
No sustituye una prueba visual del renderizador Vulkan en GPU ni pruebas en Windows/Linux.

## Implementado

- Editor hexadecimal virtualizado con caché de 20 bloques de 1 MiB, interpretación,
  selección secuencial/rectangular, sobrescritura directa/Shift, undo/redo y guardado.
- Inspector tabular, formato en vivo, reglas y búsqueda por hex/comodines, texto y regex.
- Biblioteca editable de magic bytes con importación/exportación y navegación a resultados.
- Entropía por ventanas, tareas de lectura/análisis en segundo plano y monitor del visor.
- Renderizador Vulkan nativo macOS, cámara, controles de lectura de mallas y esfera de prueba.
- Etiquetas y selección de vértices/caras/mallas con ocultación, colores y gradiente por índice.
- Colección de mallas en sesión y vista UV con selección compartida con 3D.
- Scripting Python incrustado en un auxiliar y C++ compilado, consola y lectura binaria/mallas.
- Persistencia automática de distribución de paneles y ventana, recuperación y restablecimiento.

- Exportación OBJ/GLB/STL con selección de mallas, escala, ejes, normales y UV.
- Historial persistente de 12 archivos recientes y reapertura con protección de cambios.

## Corrección de esta revisión

El temporizador de autoguardado de UI se reiniciaba con cada notificación de distribución.
Una secuencia continua podía posponer indefinidamente el guardado durante esa actividad.
Ahora la primera notificación agenda la escritura y las siguientes se agrupan sin aplazarla.
Se añadió una prueba con eventos continuos. No modifica el esquema del archivo ya guardado.

## Pendientes de los objetivos discutidos

1. Guardar y reabrir un proyecto editable completo; exportar geometría ya está implementado.
2. Separar visualmente bloques/containers y plegarlos; hoy existen marcas y reglas,
   pero no un árbol de bloques expandible con vistas independientes.
3. Sincronizar selección 3D/UV con navegación y marcado de bytes. Ya se muestran offsets,
   pero no se salta al hex desde esa selección ni hay inspector navegable de strips.
4. Descompresión/desempaquetado. Reconocer una firma no descomprime su contenido.
5. Acceso a RAM de procesos, captura, edición y refresco de regiones.
6. Vincular texturas y materiales al visor 3D/UV; GLB admite un material mate de color de prueba.
7. Windows/Linux: superficie Vulkan nativa, compilación y pruebas reales; empaquetado sin
   depender de las rutas de Qt, Python, compilador y SDK instaladas en este equipo.
8. Gradientes por offset/strip/UV/coordenadas y editor libre de puntos del gradiente.
   Actualmente solo hay tres colores editables y mapeo por índice de vértice/cara.

## Parcial o trabajo de consolidación

- La distribución UI se guarda, pero no hay un proyecto completo con mallas, descriptores,
  cámara, ajustes visuales, reglas y scripts. Las mallas son capturas de la sesión.
- Los descriptores se introducen manualmente; no se interpreta automáticamente una cabecera
  ni se conserva un descriptor editable asociado a cada entrada de la colección.
- Los UV actuales son uno por vértice. Costuras mediante duplicados funcionan; índices UV
  independientes por esquina no están modelados. La vista no edita coordenadas.
- Los scripts trabajan sobre la malla activa y leen el binario; la API aún no ofrece
  escritura de bytes ni administración de toda la colección.
- C++ se compila y ejecuta, no es un intérprete interactivo equivalente a Python.
- El contador de ocho canales no incluye el trabajo de mallas ni los procesos de scripting.
- La preparación combinada de la escena/BVH y cargas GPU aún se hace en el hilo de interfaz;
  debe revisarse para colecciones grandes. No se ha medido su umbral de fluidez.
- Cerrar mientras se decodifica una malla espera a su trabajo; falta cancelación integrada.
- Detener scripting termina el proceso auxiliar directo, no garantiza terminar sus descendientes.
- El editor sobrescribe; insertar/eliminar bytes y crear contenido desde un archivo vacío
  no están implementados. Texto/regex se limita a regiones de 1 MiB.
- La integración específica de Google Drive sigue siendo opcional y no está implementada;
  se pueden abrir archivos locales sincronizados como cualquier otro archivo.

Siguiente orden sugerido: proyecto guardable → vínculo hex/3D/UV
→ bloques plegables → descompresión. Consolidar rendimiento y multiplataforma en paralelo
al crecimiento del visor, sin confundir las pruebas sin pantalla con validación gráfica.

Revisión de exportación: compilación correcta y ambas suites automatizadas completas pasan.
Diálogo inspeccionado mediante captura sin GPU; exportación de la esfera a los tres formatos
verificada. No se ha validado aún su importación en Blender u otra aplicación externa.
