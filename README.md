# Binary Workbench

Editor hexadecimal local con sobrescritura de bytes en C++23 y Qt 6, preparado para CLion. Abre un archivo con Archivo → Abrir archivo o Cmd+O en macOS. El ejemplo `examples/pattern.bin` contiene un patrón conocido. No requiere Google Drive ni incorpora conexión a su API.

## Navegación continua

La única barra vertical del visor recorre todo el archivo. La rueda y las flechas de la barra avanzan por filas; arrastrar el control permite saltar a otra región. Re Pág y Av Pág avanzan una pantalla; Inicio y Fin llevan al principio y al final. Ir acepta una posición decimal o hexadecimal con prefijo 0x. La tabla muestra únicamente las filas que caben en pantalla, sin crear una fila por cada registro del archivo.

Se ha retirado la barra horizontal de posición global y los botones de página. Para archivos que superan el rango entero de Qt, la posición del control se escala al rango completo del archivo: el arrastre es aproximado; la rueda y los pasos por filas conservan precisión, y el campo Ir permite una posición exacta de 64 bits.

## Caché y trabajo en segundo plano

ChunkCache comparte hasta 20 bloques de 1 MiB (20 MiB de datos retenidos como máximo) entre el visor, el inspector y la copia. Los bloques están alineados a posiciones múltiplo de 1 MiB. Se cargan bajo demanda: recorrer una región reutiliza su bloque; cruzar el límite carga el siguiente; retroceder reutiliza bloques conservados. Al llegar al límite, se descarta el menos recientemente utilizado (LRU). No se reservan ni leen 20 MiB al abrir un archivo pequeño. No hay precarga especulativa.

La capacidad de la caché no representa el consumo total del proceso. Qt, las celdas visibles, los resultados y las lecturas en curso necesitan memoria adicional. Una operación que cruza un límite puede utilizar dos bloques; varias lecturas pueden mantener temporalmente referencias a bloques recién descartados. La copia tiene un límite adicional de 1 MiB de región fuente.

Cada canal de lectura usa QThreadPool y QtConcurrent con una tarea activa y una petición pendiente reemplazable. Las peticiones de navegación obsoletas no se acumulan. Los trabajadores abren sus propios archivos y no acceden a widgets; el hilo de interfaz recibe resultados mediante QFutureWatcher. La caché usa un mutex solo para consultar/actualizar la lista; nunca mantiene ese bloqueo mientras lee el disco.

Al cambiar de archivo se vacía la caché y se incrementa su generación, evitando que lecturas antiguas vuelvan a introducir bloques. Se verifican tamaño y fecha de modificación incluso al reutilizar bloques. Esto detecta cambios habituales, pero no constituye una instantánea ni detecta sustituciones que conserven esos metadatos. Para un análisis reproducible se recomienda una copia local estable.

Al cerrar se descartan peticiones pendientes y se espera asíncronamente a las activas. El sistema operativo no siempre permite interrumpir inmediatamente una lectura bloqueada. La aplicación continúa procesando eventos durante esa espera.

## Stride e inspector

Stride indica los bytes por fila, de 1 a 256, y puede escribirse directamente en el control. Al cambiarlo se conserva el byte del cursor inicial y se reinicia allí la selección.

Las opciones de texto son ASCII, UTF-8, UTF-16LE, UTF-16BE y japonés CP932 (variante de Shift JIS). La interpretación numérica ofrece enteros de 8/16/32/64 bits y Float32/Float64, con little endian o big endian. En la tabla se interpreta cada fila independientemente; un carácter o valor cortado por el stride puede aparecer incompleto. El inspector lee desde el cursor inicial, incluso si el valor cruza una fila o un bloque.

El inspector muestra hasta 64 bytes de texto y valores numéricos desde el cursor inicial. Puede usar bytes posteriores que no pertenezcan a la selección; el propio panel lo indica. No presupone que esa interpretación corresponda al formato real.

## Selección y copia

- Clic y arrastrar: selección secuencial de bytes.
- Alt + arrastrar: selección rectangular según el stride.
- Shift + clic o Shift + flechas: extender desde el cursor inicial; se conserva al desplazarse.
- Copiar: hexadecimal al portapapeles y datos binarios en un formato MIME adicional. Una selección rectangular se copia por filas.
- Seleccionar vista: selecciona los bytes actualmente mostrados.

La copia se rechaza si la región fuente abarca más de 1 MiB, sin truncarla silenciosamente. El panel Inspector se puede mover o desacoplar.

## Menús y herramientas

Archivo incluye abrir, volver a abrir, guardar, guardar como, opciones y salir. Editar incluye sobrescribir bytes, pegar hexadecimal, deshacer y rehacer. Cortar permanece desactivado: esta versión no inserta ni elimina bytes. La barra principal ofrece acciones comunes.

## Lectura y escritura

Puedes escribir directamente tras seleccionar en el visor. Para introducir una secuencia completa mediante diálogo, usa doble clic o Editar → Sobrescribir bytes (F2). Introduce pares hexadecimales, por ejemplo `00 FF 2A`. Con un solo byte seleccionado, una secuencia sobrescribe desde esa posición; con una selección mayor o rectangular, el número de bytes debe coincidir exactamente. Pegar acepta los datos binarios copiados por la aplicación o texto hexadecimal. No se cambia el tamaño del archivo ni se desplazan sus offsets.

Los cambios se superponen al contenido original: el visor, el inspector y la copia los muestran inmediatamente. Deshacer y rehacer funcionan hasta guardar. El título y la franja inferior indican cambios pendientes. Al cerrar, abrir otro archivo o volver a abrir, se ofrece guardar, descartar o cancelar.

Guardar y Guardar como trabajan en un cuarto pool, en segundo plano. Se recorre el original en bloques de 1 MiB y se aplican los cambios a un archivo temporal situado junto al destino. QSaveFile sustituye el destino al finalizar; se desactiva el modo de escritura directa. Un error conserva los cambios pendientes. Se comprueban tamaño y fecha del original antes y después de copiar; no es un bloqueo exclusivo contra otros programas ni detecta cambios que conserven esos metadatos. Durante el guardado no se permite editar, cambiar de archivo ni cerrar; el monitor de hilos muestra los bytes procesados.

El guardado requiere espacio libre para una copia completa, puede tardar en archivos grandes y no conserva su carácter disperso. No carga el archivo completo en RAM. Se mantienen los permisos básicos; no se garantiza conservar otros metadatos, atributos extendidos o enlaces duros. Para un destino que sea enlace simbólico, usa Guardar como con una ruta normal.

Límites de edición: 1 MiB por operación, 4096 filas por edición rectangular y un historial de hasta 20 MiB de bytes de reemplazo, 1024 operaciones o 16384 regiones. Este historial y sus índices son memoria adicional a los 20 MiB de caché. Guardar reinicia el historial. La escritura por inserción/eliminación y la creación de contenido en archivos vacíos quedan pendientes.

Herramientas contiene una calculadora UInt64 con entradas decimales o hexadecimales, operaciones aritméticas y AND/OR/XOR; controla desbordamientos y división por cero. Acerca de ofrece un aviso sobre el estado del prototipo y esta documentación local.

## Entorno de desarrollo

- Qt 6.11.2: `/Users/jarky/Qt/6.11.2/macos`
- Vulkan SDK 1.4.357.1: `/Users/jarky/VulkanSDK/1.4.357.1/macOS`
- Apple Clang de Xcode; CMake y Ninja incluidos en CLion.
- Perfil `mac-local` (Mac ARM64 - Qt y Vulkan), en CMakeUserPresets.json.
- Compilaciones fuera del proyecto: `../../work/build-binary-workbench`.
- Objetivo: `binary_workbench`.

La decodificación CP932 usa iconv en macOS/Linux y la API de Windows. El visor hexadecimal no necesita inicializar una GPU. La vista 3D usa Vulkan/MoltenVK mediante integración nativa en macOS, porque este paquete Qt declara QT_FEATURE_vulkan=-1. Python y C++ están disponibles en el dock Scripting.

Compartir código, pruebas, ejemplos y configuraciones generales. CMakeUserPresets.json, .idea, SDK y compilaciones son locales a cada equipo. Google Drive no interpreta .gitignore. La aplicación aún depende de las bibliotecas instaladas y no es un paquete distribuible.

## Verificación

Pruebas con archivos dispersos de 10 y 64 GiB: posiciones de 64 bits, últimos bytes, stride no divisor del tamaño, desplazamiento escalado, rueda y avance exacto por filas. Pruebas de la caché: cruce de bloques, reutilización sin releer datos, máximo de 20 MiB, descarte LRU, truncado y exclusión de resultados tras cambiar de archivo. También se verifican selección rectangular, copia, japonés/Unicode, floats y endianness, calculadora, cola acotada y respuesta del bucle de eventos. La edición tiene pruebas de superposición, deshacer/rehacer, selección rectangular, inspector y copia, guardado sobre el original y en otra ruta, cruce de bloques, direcciones de 64 bits y conservación del destino ante errores.

Los archivos dispersos tienen gran tamaño lógico sin ocuparlo físicamente. Estas pruebas no equivalen a un benchmark de disco sobre archivos reales de ese tamaño. Compilado y probado en macOS ARM64; Windows también se ha compilado y probado: véase la guía Windows. Linux aún no se ha ejecutado.

```sh
cmake --preset mac-local
cmake --build --preset mac-local
ctest --test-dir ../../work/build-binary-workbench --output-on-failure
```

CTest usa QT_QPA_PLATFORM=offscreen. `--smoke-test` abre y cierra la aplicación automáticamente.

### Monitor de hilos

El indicador «Hilos», integrado en la misma franja que el estado del archivo, muestra canales ocupados y solicitudes pendientes.
Al pulsarlo se abre un panel acoplable con el estado del visor, inspector, copia, guardado, búsqueda, firmas, entropía y edición directa,
los offsets y tamaños solicitados, tiempos, tareas terminadas, errores, resultados
obsoletos descartados y solicitudes pendientes sustituidas. También se abre desde Ver.
Cada canal usa un pool de un hilo, con un máximo conjunto de ocho tareas activas y
tres lecturas pendientes (el guardado, la búsqueda, las firmas y la entropía no tienen cola); el hilo de la interfaz queda separado. Los contadores corresponden
a la sesión, incluso si se cambia de archivo. La actualización es cada 200 ms.
La ocupación mide tareas, no CPU. Los tiempos incluyen esperas y entrega a la interfaz;
las tareas breves pueden terminar entre actualizaciones y quedan registradas en los contadores.

## Formato visual y reglas de enmarcado

El bloque «Formato», integrado en el inspector «Reglas de formato», aplica los cambios
inmediatamente a la regla seleccionada, sin crear reglas nuevas ni cambiar sus rangos.
No hay botón Aplicar. Color de texto, background, contorno y subrayado tienen una
casilla de activación, un valor editable `#RRGGBB` y una muestra que abre el selector
de color. Negrita y cursiva usan casillas simples. El subrayado tiene color propio.
Sin regla seleccionada, los controles solo preparan el estilo para nuevas marcas;
no modifican ninguna regla existente. Los colores incompletos o inválidos no se aplican.

«Jump to» guarda un destino hexadecimal por regla. Por defecto usa el primer byte
marcado; editar el campo y confirmar con Enter permite saltar directamente. Pulsar
la regla en la lista también lleva el cursor a su destino. El salto no modifica el
rango de formato y rechaza direcciones fuera del archivo. Tanto el destino como el
color del subrayado se conservan al importar/exportar; los JSON anteriores siguen
siendo compatibles.
Para crear regiones nuevas utiliza «+» o las acciones de marcado de coincidencias.
La selección actual se distingue mediante una línea inferior, conservando los colores.
Las selecciones rectangulares guardan el stride con el que se crearon: cambiar después
el stride de la vista no cambia los bytes marcados.

El inspector adicional «Reglas de formato» está en una pestaña junto al Inspector y
puede moverse o desacoplarse. Permite añadir secuencias con nombre, inicio, longitud,
paso entre inicios y número de repeticiones. Ejemplo: inicio 0x40, longitud 4, paso 16,
4 repeticiones marca 0x40–0x43, 0x50–0x53, 0x60–0x63 y 0x70–0x73. «Selección»
rellena esos campos. Pulsa + para abrir el editor de regla y Aceptar para crearla con el estilo actual (incluye marco solo si está activado).
Los límites se validan contra el tamaño del archivo. No se permiten secuencias
solapadas dentro de una misma regla (paso ≥ longitud); sí pueden solaparse reglas distintas.

Cada regla tiene una casilla para activarla. Selecciona una en la lista para actualizar
su estilo/nombre, cambiar sus parámetros, ir a ella, reordenarla o eliminarla. Las reglas
creadas desde una selección manual conservan su región; «Modificar» cambia
su estilo/nombre. La última regla coincidente sustituye el estilo completo de las anteriores.
Se admiten hasta 256 reglas. Las repeticiones se calculan para los bytes visibles: no se
recorre ni se carga el archivo entero para aplicar formato.

El formato afecta tanto al hexadecimal como a los caracteres/valores a la derecha.
Cada elemento decodificado conserva su rango de bytes (ASCII, UTF-8, UTF-16, CP932
y tipos numéricos). Si una regla toca algún byte de un carácter o valor multibyte,
se aplica a todo ese elemento; entre varias, prevalece la última. Las unidades cortadas
por el final de una fila se siguen interpretando de forma independiente en cada fila.
Los controles/espacios se muestran como puntos en la tabla. No hay selección de bits
individuales: la unidad actual es el byte.

El formato no modifica el contenido binario ni el historial de edición. Se mantiene al
guardar el binario. «Exportar reglas» lo guarda por separado en JSON y «Importar reglas»
permite recuperarlo o reutilizarlo sobre un archivo compatible. Los offsets se serializan
como cadenas para preservar 64 bits. La importación valida estructura, límites y tamaño
(máximo 1 MiB) antes de reemplazar el conjunto actual. Al cerrar o cambiar de archivo
se ofrece exportar las reglas que aún no se han guardado. El guardado de bytes y el de
formato son independientes; deshacer/rehacer se aplica a los bytes, y las reglas se gestionan
con la lista.

Puedes probar la importación abriendo `examples/pattern.bin` e importando `examples/pattern.format.json`, que contiene una selección coloreada y cuatro secuencias enmarcadas.

## Inspector compacto y búsqueda de patrones

El formato ya no ocupa una barra superior. Los controles están dentro del inspector.
Bajo la lista: **+**, **−**, **Modificar**, **↑**, **↓**. El editor de una regla solo
aparece al añadirla o modificarla. Importar y Exportar están al final del inspector;
si no caben, se accede desplazando su barra lateral.

«Buscar patrones» admite cuatro modos:

- **Hex + comodines:** `48 8B ?? A?`. Cada `?` sustituye un dígito hexadecimal;
  `??` sustituye un byte, `A?` coincide con A0–AF. Busca por bloques en todo el
  archivo o en la región indicada, incluidas coincidencias que cruzan bloques.
  Los comodines son de longitud fija: `*` no es un comodín hexadecimal.
- **Regex de bytes:** por ejemplo `\x48.{2}\x41`. Cada byte se representa con
  un carácter Latin-1, conservando los offsets; el punto incluye saltos de línea.
- **Texto literal:** busca el texto exacto con UTF-8, UTF-16LE/BE o CP932 según el
  selector del visor. En modo ASCII se utiliza una correspondencia Latin-1 de un
  carácter por byte, incluidos controles. Los espacios no se sustituyen por puntos
  al buscar, aunque la tabla los visualice así.
- **Regex de texto:** por ejemplo `Nombre[0-9]+`, con la misma decodificación.
  Los resultados se convierten a offsets y longitudes de bytes, incluidos caracteres
  multibyte. La región debe empezar en un límite de carácter para evitar fragmentos
  inválidos, especialmente en UTF-16.

**Texto y regex trabajan sobre una región de hasta 1 MiB**, no sobre el archivo
entero automáticamente. Indica Inicio y Bytes, o pulsa Selección para utilizar el
intervalo comprendido entre sus extremos (en una selección rectangular incluye
los huecos entre filas). Los anclajes y lookarounds de regex se evalúan dentro de
esa región; `^` y `$` no representan necesariamente los extremos del archivo.
Las coincidencias vacías se omiten. Regex no devuelve coincidencias solapadas;
los comodines hexadecimales sí pueden hacerlo.

La búsqueda se ejecuta en un quinto canal del thread pool, con progreso y botón
Detener. Incluye las modificaciones aún no guardadas. Si se cambia el documento,
se cancela y descarta la búsqueda; el formato existente sigue asociado a offsets.
Se muestran hasta 10 000 resultados, indicando si se alcanzó el límite y la búsqueda
quedó incompleta. Las expresiones tienen límites de tamaño y de trabajo del motor.
La cancelación se comprueba entre operaciones; no interrumpe una operación interna
ya iniciada del motor regex.

El número y las flechas recorren y seleccionan coincidencias completas. **Marcar**
crea una única regla con los resultados y el estilo actual, visible a ambos lados.
Son posiciones obtenidas en esa búsqueda, no reglas que se vuelvan a ejecutar al
editar: para actualizar coincidencias, vuelve a buscar y sustituye la regla anterior.
Se pueden exportar/importar junto con las demás reglas; el límite global de posiciones
guardadas es 10 000. La búsqueda acota la memoria de lectura; la decodificación y los
índices de caracteres requieren memoria adicional a la caché de archivos.

## Biblioteca de magic bytes

Abre **Herramientas → Biblioteca de magic bytes** (también disponible en **Ver**).
La biblioteca incluye PNG, DDS y gzip/DEFLATE como ejemplos; permite añadir,
modificar, eliminar y activar/desactivar firmas, e importar/exportar JSON. Los cambios
se conservan exportando la biblioteca; al cerrar se ofrece exportar si hay cambios.
Importar sustituye la biblioteca actual tras comprobar el documento completo.

Cada firma contiene nombre, bytes hexadecimales, desplazamiento relativo y notas.
Se admiten máscaras `??`, `A?` y `?F`, con al menos un dígito fijo. El desplazamiento
indica dónde deberían aparecer los bytes respecto al comienzo candidato del contenido.
Hay cuatro ámbitos: todo el archivo, inicio del archivo, inicio de una selección
continua e inicios de las regiones definidas por las reglas de formato activas.
En estos tres últimos ámbitos la firma debe caber dentro de la región. Una selección
rectangular marcada aporta una región por fila. La vista plegable de bloques sigue pendiente.

Los resultados muestran el offset navegable de la firma y el inicio candidato
(offset menos desplazamiento relativo). Pulsar una fila selecciona sus bytes en el visor.
Selecciona varias filas para enmarcarlas con el estilo actual del inspector de formato;
los intervalos solapados se unen. El estilo afecta al hexadecimal y a su representación
interpretada. Las marcas se exportan por separado mediante las reglas de formato.
No hay una correspondencia obligatoria entre formato y color.

Una coincidencia **no valida el archivo ni permite deducir su longitud**. Puede ser una
coincidencia accidental o un fragmento incompleto. El análisis recorre el archivo en
chunks de 1 MiB, conserva el solapamiento necesario y aplica las ediciones pendientes.
Utiliza un sexto canal de trabajo, con progreso y cancelación. Abrir, editar o guardar
invalida los resultados; cambiar las reglas durante un análisis de regiones lo cancela.
Límites: 64 firmas de hasta 4096 bytes, 10 000 regiones/resultados, importaciones de
hasta 1 MiB. Las firmas con pocos dígitos fijos pueden tardar más y producir muchas
coincidencias. No se carga el archivo entero en RAM.

Fuentes de las firmas iniciales:
- PNG: https://www.w3.org/TR/png-3/
- DDS: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-file-layout-for-textures
- gzip: https://www.rfc-editor.org/rfc/rfc1952

## Visor de entropía

El panel «Entropía» aparece debajo del inspector; también se abre desde Herramientas
 o Ver. Selecciona archivo completo o selección continua, elige una ventana mínima
(de 256 bytes a 1 MiB) y pulsa Analizar. La curva muestra la entropía de Shannon de
cada región en bits por byte (0–8). No es una medida de la diferencia numérica entre
bytes adyacentes. Al pasar el cursor muestra offset, longitud, entropía, porcentaje de
ceros y ASCII imprimible (32–126). Pulsar el gráfico selecciona esa región en el visor.

El análisis recorre todos los bytes en chunks de 1 MiB, incorpora las ediciones
pendientes y utiliza un séptimo canal de trabajo cancelable. Conserva como máximo
8192 puntos: aumenta automáticamente la ventana y muestra su tamaño efectivo.
Una selección más pequeña permite inspeccionar transiciones con mayor resolución.
La última ventana puede ser más corta. No hay muestreo ni suavizado de los valores.
Abrir, editar o guardar cancela el análisis e invalida el gráfico; se actualiza pulsando
Analizar otra vez. La lectura y los resultados tienen memoria acotada.

La entropía de bytes no demuestra compresión ni cifrado: repetir los 256 valores por
orden también produce entropía 8. La descompresión sigue pendiente.


## Escritura directa

Haz clic en la columna hexadecimal o en un carácter/valor de la columna interpretada
 y escribe. La selección activa determina desde dónde se sobrescribe:

- Hexadecimal: admite 0–9 y A–F. La primera tecla sustituye el dígito alto y conserva
  el bajo; la segunda sustituye el bajo y avanza al siguiente byte.
- Shift + tecla: aplica esa misma posición a todos los elementos seleccionados,
  incluidas las regiones rectangulares, sin avanzar ni perder la selección.
  Por ejemplo `AB CD` → Shift+0 → `0B 0D` → Shift+0 → `00 00`.
- Números: sustituye los caracteres del valor decimal mostrado uno a uno. Conserva
  los ceros iniciales durante la edición para que no se desplacen las posiciones.
  Valida el valor completo, el rango del tipo y su endianness en cada pulsación.
  Con Shift, los valores más cortos conservan sus bytes cuando no tienen esa posición.
  Una pulsación inválida rechaza toda la operación; no hay cambios parciales.
- Texto: escribe caracteres representables en ASCII, UTF-8, UTF-16 o CP932. Por ahora
  el nuevo carácter debe ocupar los mismos bytes que el anterior; no hay inserción.

Cada pulsación aceptada es una operación de Deshacer/Rehacer. Escape, cambiar la
selección o cambiar la interpretación reinicia la posición de escritura y descarta
pulsaciones aún pendientes. La barra de estado indica la posición dentro del elemento.
Las teclas no válidas no se escriben. El archivo original solo cambia al guardar.

La lectura y transformación usan un octavo canal en segundo plano, con una cola
máxima de 256 pulsaciones. Guardar, abrir otro archivo o cerrar desde la ventana
espera a las pulsaciones en curso. Límite por pulsación: 1 MiB de región abarcada en
hexadecimal o 64 KiB en representación interpretada; máximo 4096 filas rectangulares.
Siguen aplicándose los límites del historial. Las operaciones atómicas rechazan
valores incompletos, desbordamientos o archivos modificados externamente.

## Visor Vulkan y configuración 3D

Herramientas → Vista 3D · Vulkan abre el panel de renderizado. Herramientas →
Configuración de malla 3D abre un dock independiente que se puede ocultar, mover o
hacer flotante; ambos están disponibles también en Ver.

El descriptor permite configurar offset, cantidad, stride y tipo de posiciones e
índices; UVs y normales opcionales, escala y endianness. Admite Int16/Int32/Float16/
Float32 para XYZ, UInt16/UInt32 para índices, triangle strips con reinicio FF o listas
de triángulos, UV Float16/Float32 y normales Float32/SNorm16. Sin normales se calculan
mediante las caras. La cantidad de índices incluye separadores y degenerados.

El botón Renderizar decodifica en un pool independiente de un hilo. Incluye las
ediciones pendientes y descarta resultados si cambian el documento o los parámetros.
Las lecturas se hacen por chunks de 1 MiB. Límite inicial: 100 000 vértices, 600 000
índices y 64 MiB de secciones leídas. Valida rangos, tipos, índices y valores no finitos.
La malla CPU y sus buffers gráficos son memoria adicional a la caché de archivo.
Este pool de mallas aún no está incluido en el contador de los ocho canales del visor.

La vista utiliza shaders SPIR-V incluidos en la aplicación, profundidad, caras,
aristas mediante líneas (sin exigir fillModeNonSolid), puntos y líneas de normales.
Orbitar: arrastrar con botón izquierdo. Desplazar: derecho/central. Zoom: rueda.
Encuadrar restablece la cámara. Opciones permite cambiar fondo, color del modelo,
color/visibilidad de la cuadrícula e iluminación direccional. La vista centra y
normaliza la malla para encuadrarla; la cuadrícula muestra unidades de esa vista.
Los UV se decodifican y cargan, pero aún no se aplican texturas. Opciones y descriptores
son de la sesión; su persistencia/importación se añadirá después.

Ejemplo listo: [sphere128.bin](examples/sphere128.bin),
[mapa y cabecera](examples/sphere128.md). Tiene 128 vértices, 252 triángulos y dos
bloques de normales para comparar Float32 con SNorm16. El preset carga esos offsets;
no se trata todavía de un lector automático de cabeceras.

En macOS utiliza CAMetalLayer/MoltenVK, se suspende al ocultarse y recrea los recursos
al redimensionar. Los errores se muestran en el panel con Reintentar Vulkan. Windows utiliza una superficie Win32; Linux sigue pendiente de integración nativa. La compilación, lectura, topología,
controles y validación SPIR-V se comprueban sin GPU. La presentación real y el docking
con GPU aún requieren comprobación en una sesión gráfica con acceso a Metal.

## Scripting Python y C++

Herramientas → Scripting abre un dock con editor, selector de lenguaje, abrir/guardar,
Ejecutar, Detener y consola. Cada lenguaje conserva su borrador durante la sesión.
Python usa CPython incrustado en un ejecutable auxiliar; C++ compila una biblioteca
nativa con el compilador configurado y la ejecuta en ese auxiliar. Requiere Python
con cabeceras/biblioteca de desarrollo y el compilador local; no es todavía un paquete
redistribuible independiente. Cada ejecución comienza con un contexto nuevo.

Los ejemplos iniciales muestran la API. Python: `import workbench as bw`, `bw.size`,
`bw.path`, `bw.selection`, `bw.read(offset, count)`, `bw.mesh()` y
`bw.set_mesh(vertices, indices, normals=None, uvs=None)`. C++: incluir `workbench.hpp`
y definir `void run()`; usar `bw::read`, `bw::size()`, `bw::selection()`, `bw::mesh()`
y `bw::set_mesh(mesh)`. Las lecturas incluyen las ediciones pendientes y están limitadas
a 1 MiB por llamada. Por ahora la API del documento permite lectura, no escritura.
Las mallas usan triángulos e índices planos, hasta 100 000 vértices y 600 000 índices.
Los resultados se publican al terminar correctamente, y se descartan si cambió el
archivo o la malla durante la ejecución. La consola muestra errores y salida estándar.
Los scripts tienen los permisos locales del usuario; el auxiliar aísla fallos de la
interfaz pero no es un entorno de seguridad. Detener termina el auxiliar directo.

## Inspector tabular

Columnas Tipo / Valor con binario, enteros con y sin signo de 8–64 bits, Float32/64,
texto, fechas Unix/FILETIME/DOS y RGB. El orden de bytes se sincroniza con el visor;
la casilla hexadecimal muestra el patrón de bits de los enteros. Seleccionar una fila
y pulsar Copiar valor o Ctrl+C copia su valor. Los datos parten del cursor inicial,
pueden extenderse fuera de la selección y reflejan las ediciones pendientes.
Al final del archivo se indican los tipos sin suficientes bytes.

El acceso a RAM se ha anotado en [la propuesta de memoria de procesos](docs/process-memory-roadmap.md);
aún no se puede abrir ni modificar un proceso.


## Inspección visual de mallas

En el visor 3D, Seleccionar cambia entre Vértices, Caras y Mallas. Un clic selecciona;
arrastrar sigue orbitando. La selección muestra las coordenadas/UV o los tres índices
de la cara, y un marcador. Para mallas leídas del binario también aparecen el offset
del vértice y los offsets/slots de índices y strip de la cara. Los slots se cuentan
sobre el buffer original de índices, incluyendo reinicios y degenerados; los offsets
siguen el orden final de los vértices del triángulo, incluida la inversión de paridad.
Las mallas creadas por scripting no tienen referencias al binario inventadas.

Índices visibles activa números V/F/M desde cero. Desde y Cantidad acotan la ventana
de índices (200 por defecto, máximo 1000); además se conserva la etiqueta seleccionada
si es visible. Las caras se numeran por orden de triángulos decodificados, después de
eliminar degenerados del strip. La colección conserva IDs estables de malla y cada malla mantiene sus índices locales.
La selección se comparte con el visor UV.

Las etiquetas son geometría de texto en una pasada de Vulkan posterior a la malla;
conservan tamaño en pantalla. La CPU proyecta los anclajes, descarta los que están
fuera de pantalla o detrás de otra cara mediante un árbol de cajas de triángulos,
y evita superposiciones entre etiquetas. La selección se prioriza. La visibilidad usa
las superficies de la malla incluso en modo alambre, con una tolerancia de 0.00002 en
el espacio normalizado. Una etiqueta de cara depende de la visibilidad de su centro;
una parte visible de la cara no garantiza que el centro sea visible. No se calculan
etiquetas nuevas mientras la cámara y los ajustes no cambien.

Opciones permite elegir colores de caras, vértices y texto, tamaño de números y un
degradado con tres colores editables (0, 50 y 100 %). Se puede aplicar por separado al
índice de cara o de vértice. Para ver los colores de vértice, activar Vértices. Por ahora
los ajustes son de sesión y el degradado representa exclusivamente el índice local.

Las pruebas cubren ocultación delante/detrás, selección, recorte, referencias al archivo,
controles y shaders SPIR-V. La presentación de esta pasada en una GPU real requiere
verificación en la sesión gráfica de macOS; los tests sin pantalla no la comprueban.


## Colección de mallas y vista UV

Ver → Colección de mallas abre la lista. Configuración 3D permite **Añadir a la colección**
(valor inicial) o **Actualizar malla activa** al renderizar. Se pueden renombrar con doble
clic, activar, ocultar con su casilla o eliminar. El ID no se renumera al eliminar otra
malla. Las posiciones originales mantienen su relación espacial; el encuadre utiliza los
límites de toda la colección, incluidas las ocultas, para evitar saltos al alternar visibilidad.
El render y las comprobaciones de ocultación solo incluyen mallas visibles.

Las mallas son capturas en memoria de esta sesión: editar/abrir el documento no las borra;
las procedentes del archivo quedan marcadas como capturas anteriores. Actualizar sustituye
solo la elegida y conserva su ID/nombre. Los scripts reciben la malla activa y `set_mesh`
la actualiza; sin activa crea una entrada. Se descarta un resultado si cambia la malla de
destino durante la ejecución. La colección puede exportarse a OBJ, GLB o STL; todavía no se guarda como proyecto editable.

Ver → Vista UV muestra un dock flotante/acoplable independiente de la GPU. El cuadrado
0–1 tiene un damero de referencia: no es una textura importada. Las coordenadas exteriores
se muestran sin envolver. Encuadrar UV ajusta los límites; rueda amplía alrededor del cursor,
botón derecho/central desplaza. Solo activa filtra la colección; al desmarcarla se muestran
las mallas visibles con UV. La selección por clic funciona en ambos sentidos entre UV y 3D;
Alt+clic recorre elementos coincidentes en UV. Los modos Vértices/Caras/Mallas se sincronizan.
El seleccionado aparece resaltado, incluso si sus etiquetas normales están desactivadas.

La vista utiliza un par UV por vértice, como el lector actual. Se conservan vértices duplicados
para costuras; aún no se admiten índices UV independientes por esquina ni edición de UV.
Una malla leída sin UV no aparece artificialmente como puntos en (0,0). Los colores y degradados
comparten los ajustes de Opciones 3D al cerrar ese diálogo. Índices muestra hasta los primeros
200 elementos de la malla activa más la selección; las coincidencias se pueden recorrer sin
necesidad de etiquetas. El panel permite comprobar U/V y los índices locales de cada cara.

Verificaciones: IDs y selección estables al eliminar/ocultar, ocultación entre mallas,
normalización estable, añadir/actualizar desde el lector, selección por clic en ambos sentidos,
renombrado y sincronización de controles. El visor UV se prueba y captura sin GPU; el render
Vulkan sigue requiriendo comprobación visual en una sesión con acceso a Metal.

## Distribución persistente de la interfaz

La aplicación crea automáticamente `BinaryWorkbench/ui-layout.json` en el directorio de
configuración del usuario que proporciona Qt para cada sistema. Conserva el tamaño y posición
de la ventana, paneles visibles/ocultos, áreas de acoplamiento, pestañas, tamaños y ventanas
flotantes, además de la barra de herramientas. Es una distribución global de la aplicación;
abrir otro binario no la cambia. No guarda bytes, scripts ni la colección de mallas.

Los cambios se guardan automáticamente en intervalos de hasta 400 ms mientras hay cambios, y de nuevo al aceptar el cierre.
Se escribe mediante un archivo auxiliar y sustitución atómica para evitar estados parciales.
Si falta el archivo se crea; si su versión, integridad o contenido son inválidos se recupera
la distribución predeterminada. Los paneles flotantes que quedaron fuera de los monitores
actuales se recolocan. Ver → Restablecer distribución de paneles permite recuperar el diseño
inicial y guardar esa distribución. No es un archivo temporal que el sistema deba limpiar.


## Exportar modelos y archivos recientes

**Archivo → Exportar modelo 3D…** permite elegir mallas activas, visibles o todas,
escala uniforme, permutación XYZ y signos de los ejes de salida, orientación de caras,
normales conservadas/recalculadas/omitidas y UV con inversión V opcional.
La ventana explica siempre qué conserva y qué descarta cada formato.

| Formato | Mallas separadas | Normales | UV | Material |
| --- | --- | --- | --- | --- |
| GLB (glTF 2.0) | Sí, con nombres | Opcionales | Opcionales | Color mate opcional, doble cara |
| OBJ | Objetos con nombres | Opcionales | Opcionales | No se genera MTL |
| STL binario | Superficie combinada | De cada cara | No | No |

Se exportan las coordenadas originales de las mallas, sin la normalización del visor.
Las reflexiones ajustan automáticamente el orden de los triángulos. GLB usa metros,
Y arriba y coordenadas de mano derecha según la
[especificación glTF 2.0](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html).
Ajusta escala y ejes según el binario. OBJ/STL requieren acordar unidades en destino.
No se exportan texturas, huesos, animaciones, offsets, reglas ni colores de depuración.
El material GLB es un color de prueba, no una vinculación de texturas.

La escritura corre en segundo plano y puede cancelarse. Se guarda de forma atómica,
se protege el binario de origen y se validan índices y valores finitos.
Límite inicial: 2 millones de vértices y 4 millones de triángulos por exportación.
Las mallas marcadas como capturas anteriores se advierten en el diálogo.

**Archivo → Archivos recientes** conserva hasta 12 archivos abiertos correctamente,
sin duplicados, en `recent-files.json` junto al archivo de distribución de UI.
Los archivos desaparecidos quedan deshabilitados; el menú permite limpiar el historial.
Abrir un reciente respeta el aviso de cambios pendientes.

Las pruebas cubren formatos, transformaciones, UV, índices, cancelación y protección
del origen, además del diálogo y persistencia del historial. La esfera de prueba
se exporta con 128 vértices y 252 triángulos.


## Preparación Windows · 2026-09-24

La superficie Win32 compila y presenta imagen en una RTX 4060 Ti. Ambas suites pasan en Windows; también la prueba nativa con GPU, docking y selección/UV.
Entorno instalado: MSVC 2022 x64, Qt 6.11.2, Vulkan SDK 1.4.357.0 y Python 3.13.15. Blender 4.5.1 importa la esfera GLB/OBJ/STL con 128 vértices y 252 caras.
Pasos y resultados: [Windows](docs/windows.md). Las verificaciones macOS anteriores no acreditan Windows.
