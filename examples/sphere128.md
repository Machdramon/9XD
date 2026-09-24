# Esfera binaria BWS1

Archivo: `sphere128.bin`, 5296 bytes, little endian. Generación reproducible mediante
`tools/generate_sphere.py`. Formato de prueba propio, no un estándar de intercambio.

128 vértices: dos polos y siete anillos de 18 vértices. 252 triángulos no degenerados;
la superficie está cerrada (cada arista pertenece a dos caras), con orientación exterior.
Posiciones XYZ Int32, radio 10000 unidades; multiplicar por 0.0001 para obtener radio 1.
UVs binary16 / Float16. Se incluyen ambos formatos de normales para compararlos.

| Apartado | Offset | Cantidad | Tipo | Stride | Bytes |
|---|---:|---:|---|---:|---:|
| Cabecera | 0x0 | 1 | BWS1 | 128 | 128 |
| Posiciones | 0x80 | 128 | Int32 XYZ | 12 | 1536 |
| Índices | 0x680 | 402 | UInt16 strip | 2 | 804 |
| UVs | 0x9B0 | 128 | Float16 UV | 4 | 512 |
| Normales flotantes | 0xBB0 | 128 | Float32 XYZ | 12 | 1536 |
| Normales enteras | 0x11B0 | 128 | SNorm16 XYZ | 6 | 768 |

Hay 42 strips separados por el índice de reinicio 0xFFFF (bytes FF FF).
El número 402 incluye los reinicios y los índices repetidos usados para corregir la
orientación; no equivale al número de caras. Al reconstruir, alternar la orientación
por cada paso del strip, incluso si ese paso es degenerado; reiniciar la paridad tras
0xFFFF. Descartar los triángulos con índices repetidos. Normales enteras: dividir por
32767 y limitar por abajo a -1.

Los UV no duplican vértices en la costura longitudinal: se priorizan exactamente 128
vértices y una superficie cerrada. Una textura con continuidad en la costura requiere
UVs por esquina o duplicar vértices; este ejemplo todavía no pretende probar texturas.

## Cabecera (128 bytes)

Todos los campos son UInt32 LE salvo la firma y la escala.

| Offset | Campo | Valor |
|---|---|---:|
| 0x00 | Firma ASCII | BWS1 |
| 0x04 | Versión | 1 |
| 0x08 | Tamaño de cabecera | 128 |
| 0x0C | Tamaño total | 5296 |
| 0x10 | Cantidad de vértices | 128 |
| 0x14 | Cantidad de índices | 402 |
| 0x18 | Cantidad de triángulos | 252 |
| 0x1C | Offset posiciones | 0x80 |
| 0x20 | Stride posiciones | 12 |
| 0x24 | Offset índices | 0x680 |
| 0x28 | Stride índices | 2 |
| 0x2C | Offset UV | 0x9B0 |
| 0x30 | Stride UV | 4 |
| 0x34 | Offset normales Float32 | 0xBB0 |
| 0x38 | Stride normales Float32 | 12 |
| 0x3C | Offset normales SNorm16 | 0x11B0 |
| 0x40 | Stride normales SNorm16 | 6 |
| 0x44 | Índice de reinicio | 0xFFFF |
| 0x48 | Escala posiciones (Float32 LE) | 0.0001 |
| 0x4C–0x7F | Reservado | 0 |

## Abrir en Binary Workbench

1. Abre `sphere128.bin` desde Archivo, o usa el botón del preset en Configuración 3D.
2. Abre Herramientas → Configuración de malla 3D. Sus valores iniciales corresponden
   a este ejemplo; el botón de preset los restablece y abre el archivo cuando no hay
   ediciones ni reglas pendientes. Los offsets son absolutos y los strides completos.
3. Pulsa Renderizar archivo actual.
4. Abre Herramientas → Vista 3D · Vulkan. Arrastra para orbitar, rueda para zoom,
   botón derecho/central para desplazar. Encuadrar restablece la cámara.
5. Para comparar normales enteras, cambia su offset a 0x11B0, stride a 6 y tipo a
   SNorm16; vuelve a Renderizar.

La cabecera está documentada para inspeccionarla; esta primera versión usa el preset
explícito y todavía no autodetecta ni importa descriptores desde cabeceras arbitrarias.
