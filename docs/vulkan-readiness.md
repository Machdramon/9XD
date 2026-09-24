# Preparación del renderizador Vulkan

Revisión local: 2026-09-21. Esta revisión documenta el estado previo al renderizador. Actualización: ya hay superficie macOS, shaders, cámara y carga de mallas; consulta README.md. La validación visual en GPU sigue pendiente.

## Comprobaciones

- Proyecto C++23/CMake, Qt 6.11.2 Widgets, compilación ARM64 correcta.
- Vulkan SDK 1.4.357.1, loader, MoltenVK, glslc y manifiesto de validación instalados.
- glslc ejecutable: shaderc v2026.3. Aún no se han compilado shaders del proyecto.
- system_profiler identifica Apple M1, 8 núcleos GPU y Metal Supported.
- QtGui/qtgui-config.h: QT_FEATURE_vulkan=-1 y QT_FEATURE_metal=1.
  No podemos utilizar directamente QVulkanWindow con este paquete.
- vulkaninfo --summary con VK_DRIVER_FILES apuntando al manifiesto MoltenVK
  devuelve VK_ERROR_INCOMPATIBLE_DRIVER: Metal no disponible desde este proceso.
  Esto no demuestra incompatibilidad de la GPU: contradice el inventario del sistema.
  Puede depender del entorno restringido de ejecución; la causa no está confirmada.
  No se han validado todavía instancia/dispositivo/superficie/presentación reales.
- Windows y Linux no se han probado.

## Integración propuesta

Mantener Qt para los inspectores y el docking; separar el renderizador de la ventana.
Conservar el paquete Qt actual y estudiar una superficie nativa dentro del panel:
CAMetalLayer y VK_EXT_metal_surface en macOS, encapsulados en un pequeño archivo
Objective-C++. El motor de mallas, recursos y dibujo permanece en C++.
Antes de extenderlo a Windows/Linux, validar el ciclo de vida de la superficie al
redimensionar, ocultar, desacoplar y cerrar el panel. No asumir que portar el código
C++ basta para integrar las ventanas en cada plataforma.

Alternativa: compilar Qt con Vulkan habilitado y comprobar también el soporte del
plugin de plataforma; entonces evaluar QVulkanWindow. Esto implica mantener esa
compilación de Qt. No se ha instalado ni sustituido Qt durante esta revisión.

El núcleo requiere instancia, enumeración de GPU, dispositivo/cola, superficie,
swapchain, profundidad, buffers, sincronización y pipeline. Detectar extensiones y
features disponibles: no pedir Vulkan 1.4 solo porque el SDK sea 1.4. Para MoltenVK,
considerar VK_KHR_portability_enumeration y su flag de instancia, así como
VK_KHR_portability_subset cuando lo anuncie el dispositivo. Usar validación en debug.

## Datos del visor al renderizador

Crear un descriptor de malla con inicio, longitud, cantidad de vértices, stride,
posición relativa de XYZ, tipo de componente y endianness; índices opcionales con
inicio, cantidad y tipo UInt16/UInt32. Primera versión: XYZ Float32 y triángulos.
La lectura debe incorporar las ediciones pendientes y conservar los offsets de
origen para poder relacionar vértices y selección hexadecimal.

Decodificación en trabajos cancelables, con instantáneas versionadas: si cambia el
archivo o la configuración, descartar el resultado anterior. Validar desbordamientos,
rangos, índices y valores no finitos antes de asignar o enviar memoria a la GPU.
Separar presupuesto de caché de archivo, mallas en CPU y recursos GPU. No cargar un
contenedor completo de varios GiB para visualizar una región.

El renderizador tendrá un propietario definido para su cola y recursos; los trabajos
de lectura no enviarán comandos arbitrariamente. Las cargas pasan por staging y se
liberan/reutilizan tras fences. Mantener la superficie y eventos Qt en el hilo GUI.

## Orden de implementación y criterios

1. Diagnóstico desde una aplicación de escritorio: enumerar GPU, extensiones y
   familias de colas; mostrar errores legibles sin cerrar el editor hexadecimal.
2. Panel acoplable con fondo y triángulo generado; resize, DPI, ocultar/mostrar,
   desacoplar y cierre sin errores de validación.
3. Cubo con profundidad, cámara orbital, zoom, ejes y encuadrar modelo.
4. Una región binaria como nube de puntos XYZ; después índices y triángulos.
5. Wireframe portátil (sin asumir fillModeNonSolid), selección y vínculo con bytes.
6. Normales, UV, texturas y exportación usando el mismo modelo de malla validado.

Python, descompresión y nuevas bibliotecas de UI no son requisitos para los primeros
hitos. Las texturas y la reconstrucción automática de formatos pertenecen a etapas
posteriores.

Referencias oficiales:
- https://doc.qt.io/qt-6/qvulkanwindow.html
- https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md
