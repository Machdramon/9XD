# Genki — referencias de PCSX2 y análisis cruzado pendiente

Actualizado: 25 de octubre de 2026. Trabajo local para 9XD; las capturas no se han subido a GitHub ni integrado todavía como correcciones del programa.

## Inventario

Todas las carpetas conservan el GS dump comprimido original, copia descomprimida, imagen de referencia, estado gráfico y capture-info.json con serial y SHA-256. Son datos del procesador gráfico y comandos de renderizado, no volcados de RAM del procesador principal. Los informes gif-register-audit.json, cuando existen, validan la estructura de los comandos; sus conteos TEX0 no equivalen a imágenes distintas.

| Juego / referencia | Carpeta | Fuente o candidato | Comprobado / pendiente |
|---|---|---|---|
| TXRD2 — March HK11, selector | txrd2-march-capture | 2022/000002a0.sdb | Dos imágenes indexadas de 512×512 y 256×256 y sus paletas completas coinciden exactamente con VRAM. Faltan asociaciones por malla, alias y otras superficies. |
| TXRD2 — Hakone downhill, fine weather, salida | txrd2-hakone-capture | Banco global 00001701.dat; candidatos locales 00001726/00001759/000017b8.dat | 30 imágenes fuente distintas coinciden en índices y 29 también en paleta. Un caso RGB24 y bancos locales pendientes; sin coincidencia en el snapshot no significa ausencia. |
| TXRD2 — referencia inicial AZ1 | txrd2-capture | Candidato 00000127.sdb | Una coincidencia de píxeles; identidad y paleta sin cerrar. Referencia auxiliar, menor prioridad que March. |
| TXR3 — Lancer Evolution V, selector | txr3-evo5-capture | study cases/txr3/000000c4.nfc y familia 0xbc–0xcb | Identidad visual del coche y badge Evolution V del atlas. Solo una imagen compartida distinta confirmada con paleta en el análisis de la muestra; el resultado ampliado de familia no confirma variante. Pendiente atlas principal y comandos de subida. |
| TXRD1 / Kaido Racer — Suzuki Alto Works 660 CP21S, selector | txrd1-alto-capture | 00001363.dat, identificado visualmente por el usuario | Pantalla confirma ALTO WORKS CP21S. Falta comparar bytes y determinar piezas, kit y LOD exactos. |
| TXRD1 / Kaido Racer — Hakone, cerca de salida, día | txrd1-hakone-capture | Bancos por identificar | Captura validada. Usuario cree downhill; dirección no confirmada independientemente. No reutilizar IDs de TXRD2. |
| TXR Zero — AE86, Buy Car | txrzero-ae86-capture | Zero/unpack/0x00000014.dat (candidato) | Pantalla TYPE-AE86T3. Archivo candidato reconocido desde vista filtrada; correspondencia binaria y selección de piezas pendientes. 0x000000ba.dat no es una referencia de coche identificada. |
| Racing Battle C1 GP — Impreza SportsWagon WRX, Challenge | c1-wrx-capture | study cases/c1 ps2/000004b8.dat | Ocho registros de imagen coinciden exactamente en índices; siete también en paleta. Incluyen faros, pilotos, parachoques y matrícula. No demuestra por sí solo la variante de geometría exacta; atlas principal y piezas visibles pendientes. La captura tiene transición de vehículo. |
| Racing Battle C1 GP — Nikko Hare/Dry, cerca de salida | c1-nikko-capture | Juego/2025/00002498.dat; grupo provisional 2498–2531 | Captura validada, comparación binaria pendiente. Banco identificado por carteles NIKKO CIRCUIT; 93 imágenes decodificadas. Grupo de trabajo: 34 archivos, 29 recursos con topología admitida, 29.654 triángulos. No es todavía una escena completa ensamblada. |

Las carpetas de extracción de C1 usan diferentes convenciones de numeración; no convertir automáticamente nombres entre ellas.

## Migración y avance — 25 de octubre de 2026

- [x] Copiar las nueve carpetas de capturas al repositorio, verificar hashes y conservar originales.
- [x] TXRD2 March: vincular entrada TOC 672, bloque UCL de BUILD.DAT, archivo extraído y fixture mediante reextracción y comparación exacta. Informe: txrd2-march-capture/SOURCE-PROVENANCE.md. Las demás procedencias continúan pendientes.

## To-do, en orden de trabajo

### 1. Procedencia y reproducibilidad

- [x] Guardar capturas, seriales, hashes e inventario común.
- [x] Separar juego, versión y condición de cada referencia.
- [ ] Vincular cada candidato al archivo original y entrada TOC por hashes y tamaños, sin equiparar ordinal hexadecimal, nombre decimal y offset físico.
- [ ] Resolver las copias de TXRD2 detectadas dentro de carpetas etiquetadas como TXR3; excluirlas de los fixtures de TXR3.
- [ ] Mantener un informe por prueba: coincidencia exacta, referencia compartida, hipótesis o pendiente.

### 2. Seguir el renderizado en orden

- [ ] Reproducir cronológicamente las subidas de imágenes a VRAM con BITBLTBUF, TRXPOS, TRXREG y TRXDIR; conservar estados por dibujo.
- [ ] Seguir TEX0, TEX1, CLAMP y el estado de la caché CLUT/CSA al dibujar, sin basarse exclusivamente en la VRAM inicial.
- [ ] Comparar índices, paleta y color final por separado; soportar el caso pendiente de paleta RGB24.
- [ ] Relacionar texturas y paletas con materiales y dibujos concretos; contemplar reutilización de memoria y recursos compartidos.

### 3. Casos de validación

- [x] **TXRD2 March primero:** implementar lectura acotada de bancos sin cabecera TIM2 global, registros con píxeles y alias; usar las dos coincidencias completas como fixture positivo.
- [ ] **TXRD2 Hakone:** cerrar el caso RGB24 y buscar bancos locales en el historial de subidas; verificar carretera, vegetación, carteles y transparencias.
- [ ] **C1 WRX:** contrastar geometría SportsWagon/sedán y distinguir kits/LOD; resolver atlas principal y color de carrocería. No deducir modelo únicamente por logotipos compartidos.
- [ ] **C1 Nikko:** determinar qué variante de banco corresponde a Hare/Dry; comparar imágenes de 00002498.dat contra la captura, resolver geometría del banco principal y verificar el grupo 2498–2531 y colocaciones. Conservar límites por recurso.
- [ ] **TXRD1 Alto:** confirmar material/paleta y correspondencia exacta de 00001363.dat; separar kits y piezas opcionales.
- [ ] **TXRD1 Hakone:** localizar bancos originales y asociarlos al circuito; validar dirección/condición antes de etiquetar variantes.
- [ ] **TXR Zero AE86:** confirmar 0x00000014.dat con texturas/estructuras, estudiar geometría gruesa, LOD y piezas superpuestas. La vista filtrada de identificación no constituye una reparación del decodificador.
- [ ] **TXR3 Evo V:** revisar el historial de subidas y atlas específicos; una textura compartida no identifica por sí sola el modelo fuente.
- [ ] **TXRD2 AZ1, opcional:** cerrar identidad y variante solo si aporta cobertura nueva que March no dé.

### 4. Integración nativa en 9XD

- [ ] Convertir únicamente reglas verificadas en lectores nativos por variante, manteniendo aislados C1, GIF-strip v2 y TXRD2 v1.
- [ ] Conectar materiales y texturas al resultado de Scan for 3D Data y a Render All; informar recursos no resueltos.
- [ ] Conservar procedencia, selección de piezas/LOD y colocaciones en Mesh Collection.
- [ ] Validar exportación GLB/FBX con texturas, UV, transparencia y ausencia de geometría duplicada.
- [ ] Añadir pruebas de los casos confirmados y rechazos de variantes incompatibles; comprobar imagen junto con números de geometría.
- [ ] Compilar, verificar en la aplicación y actualizar la documentación de cobertura real.

## Material de apoyo

- c1-track-research/nikko-source-manifest.json: fuentes, tamaños y hashes del grupo provisional.
- c1-track-research/nikko-report.json: resultados y limitaciones de la exploración de Nikko.
- c1-track-research/nikko-study-pack.zip: copia de trabajo conservada en el workspace original, fuera de este repositorio; no una exportación final del circuito.
- c1-track-research/00002498.dat.png: contacto de sus 93 texturas.
- txr-analysis/ZERO-IDENTIFICATION.md: límites y filtrado de la vista del AE86.
- README.md e informes JSON de cada carpeta de captura: evidencia detallada.

No se requieren más capturas por ahora. Pedir una adicional solo si falta un estado concreto que el historial existente no permita comprobar. Las referencias ayudan a descubrir reglas; el objetivo es extracción nativa sin depender del emulador o de la IA durante el uso habitual.

## Publicación y coordinación — 25 de octubre de 2026

Los informes, scripts, inventario y esta lista se han publicado con el código de 0.3.6. Las capturas/imágenes de juegos permanecen locales y excluidas de Git y del paquete público. La procedencia exacta de March está documentada; la versión 0.3.6 actualiza la UI, sin declarar nuevos lectores derivados de estas capturas. Mantener abiertas las tareas de renderizado e integración anteriores. Release: https://github.com/Machdramon/9XD/releases/tag/v0.3.6

## Comprobación de capturas — 25 de octubre de 2026

Informe canónico: [checks/RESULTS.md](checks/RESULTS.md). Este avance no modifica los lectores de 9XD.

- [x] Validar los nueve hashes, descomprimir de nuevo y comparar dumps, seriales, estado y límites GIF.
- [x] Reproducir 10.546 transacciones host→VRAM de formatos observados, con invalidación conservadora de destinos de dibujo/copia. Esto no completa la emulación GS/CLUT.
- [x] Comparar índices y almacenamiento de paletas en eventos de dibujo; caso RGB24 global de Hakone comprobado en 0x6ef40.
- [x] Prototipo de admisión acotada del banco March: seis slots preservados, tres con píxeles y tres alias vacíos; muestra positiva y 14 mutaciones rechazadas. Nueve comprobaciones sintéticas del replay correctas.
- [x] Corregir la procedencia del Alto: muestra USA SLUS-21236 frente a captura Europa SLES-53191; dos pares exactos en candidatos europeos 00000442/00000447.
- [ ] CLUT/CLD/CSA completo, paletas de 16 bits, TEX1/CLAMP y copia local→local de Zero.
- [ ] Resolver alias/tercera imagen y asignación por malla de March, variantes Nikko/Hakone y selección de kits/LOD.
- [ ] Claude: integrar solo las reglas concretas verificadas y validar scan/render/export; no asumir que el banco completo o la escena ya están reconstruidos.

Nuevos recuentos deduplicados de pares de índices+paleta almacenada: March 2, TXRD2 Hakone 36, C1 Nikko 30, C1 WRX 6, Alto europeo 2, TXR3 c4 1 y referencia AZ1 1. Zero sigue sin paleta confirmada. Los números no equivalen a materiales o mallas y no validan por sí solos los colores finales del shader. La búsqueda europea de Hakone obtiene 11 payloads/4 pares, compartidos entre muchos bancos; no identifica una variante única.

## Implementación nativa verificada — 25 de octubre de 2026

Por petición explícita del usuario, ChatGPT implementó este bloque además de su investigación. Informe: [checks/MATERIAL-INTEGRATION.md](checks/MATERIAL-INTEGRATION.md).

- [x] Banco March sin cabecera global, seis slots conservados: tres imágenes y tres referencias vacías sin sustituciones.
- [x] Vínculo primario material/slot + UV primarias de stride 4/8, comprobado con ST/Q de la captura.
- [x] Scan/Render All, Preview Texture, tres previews de imagen RAM y exportación GLB/FBX; CLAMP de bordes preservado.
- [x] Corrección TIM2 RGB24 y ampliación acotada de transferencias Zero/RGB16; 42 imágenes de la muestra AE86 decodificadas.
- [x] Build actualizado; 14 comprobaciones Genki con fixtures correctas, lector 45 correctas e interfaz 96 correctas, sin fallos; 75 comprobaciones opcionales omitidas en la suite general.
- [ ] Resolver referencias externas/alias, capas secundarias, tintes/shaders y selección de piezas/LOD.
- [ ] Ampliar enlaces entre archivos y variantes de pistas basándose en nuevas pruebas, sin reutilizar IDs entre regiones.
