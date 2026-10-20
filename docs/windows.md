# Windows: compilación y verificación · 2026-09-24

## Resultado

Compilación local correcta con Windows 11 x64, MSVC 2022 19.44.35229,
Qt 6.11.2 MSVC 2022 x64, Vulkan SDK 1.4.357.0 y CPython 3.13.15 x64
(Interpreter y Development.Embed). CMake y Ninja proceden de CLion 2026.2.3.

- `reader_tests` y `window_tests`: ambas suites pasan con `QT_QPA_PLATFORM=offscreen`.
  Informes detallados: 30 casos aprobados en reader_tests y 17 en window_tests; una omisión prevista (GPU) en offscreen. Incluyen scripting Python, compilación/ejecución C++, errores, cancelación,
  exportación y persistencia de recientes (límite de 12 y eliminación de duplicados).
-
ativeVulkanSmoke`: prueba opcional ejecutada con plataforma `windows` y
  NVIDIA GeForce RTX 4060 Ti, controlador 616.92. Resultado correcto.
- Se comprobaron esfera (128 vértices/252 triángulos), selección de cara, etiquetas,
  órbita/zoom, redimensionado, ocultar/mostrar, desacoplar/reacoplar y visibilidad de
  la colección. La selección de cara también aparece en UV.
- Se inspeccionaron visualmente fotogramas obtenidos directamente de Vulkan y
  una captura del panel UV. La capa de validación estaba habilitada; no se observaron
  errores en el registro de esta ejecución. Esto no sustituye pruebas prolongadas.
- Blender 4.5.1 LTS importó GLB, OBJ y STL en un proceso independiente: los tres
  producen una malla de 128 vértices y 252 caras. GLB/OBJ tienen una capa UV; STL ninguna.
  Fue una importación automatizada, no una revisión visual exhaustiva en Blender.

La prueba GPU se omite deliberadamente en la suite normal. Que pase offscreen no
demuestra que Vulkan presente imagen; la ejecución nativa y sus capturas son evidencia separada.

## Cambios

`src/vulkan_panel.cpp` crea una superficie `VK_KHR_win32_surface` a partir del HWND
de Qt y comparte el resto del renderizador. Solo solicita las extensiones Win32
en Windows; el camino Metal/MoltenVK de macOS permanece separado.
En plataformas offscreen falla de forma controlada y mantiene disponible el editor.

MSVC compila las fuentes y scripts como UTF-8. Las pruebas de 10/64 GiB marcan el
archivo como disperso mediante `FSCTL_SET_SPARSE` en Windows, evitando reservar
físicamente decenas de GiB en NTFS. En otros sistemas conservan el procedimiento previo.

## Preparar otro equipo

CLion es el IDE; hay que instalar también:

1. Build Tools 2022, carga Desarrollo de escritorio con C++, MSVC v143 x64 y Windows SDK.
2. Qt 6.11.x para MSVC 2022 x64 (no mezclar las bibliotecas de MinGW con MSVC).
3. Vulkan SDK x64 con glslc y un controlador GPU que soporte Vulkan.
4. Python 3.13 x64 completo, incluyendo `include/Python.h`, `libs/python313.lib`,
   `python313.dll` y la biblioteca estándar.

En esta sesión se instalaron Build Tools, Python y Vulkan mediante winget:

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --source winget --override "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
winget install --id Python.Python.3.13 --exact --source winget --architecture x64
winget install --id KhronosGroup.VulkanSDK --exact --source winget
```

Qt se descargó de su repositorio oficial mediante aqtinstall. La versión 3.3.0
de aqt requirió adaptar su selección de subdirectorio a `qt6_6112_msvc2022_64`;
no se desactivó la verificación de integridad. Para otra instalación puede usarse
el instalador oficial de Qt seleccionando el paquete MSVC 2022 de 64 bits.

## Configuración local y CLion

Todas las rutas concretas están en `CMakeUserPresets.json`, ignorado por Git.
No compartir SDK, compilaciones, DLL ni configuración del IDE.
El preset local `windows-local` usa Ninja, RelWithDebInfo y `build/windows-local`.
Es una compilación con optimización y símbolos; no se ha comprobado Debug en Windows.

En CLion seleccionar Toolchains → Visual Studio → arquitectura amd64 y el preset
`windows-local`. El entorno MSVC debe estar inicializado tanto al configurar como
al compilar. Reiniciar CLion si estaba abierto antes de instalar las herramientas.

Plantilla de preset (sustituir rutas por las del equipo):

```json
{
  "version": 6,
  "configurePresets": [{
    "name": "windows-local", "inherits": "base-debug",
    "binaryDir": "${sourceDir}/build/windows-local",
    "cacheVariables": {
      "CMAKE_BUILD_TYPE": "RelWithDebInfo",
      "CMAKE_PREFIX_PATH": "RUTA_QT/msvc2022_64",
      "Python3_ROOT_DIR": "RUTA_PYTHON",
      "CMAKE_MAKE_PROGRAM": "RUTA_NINJA/ninja.exe"
    },
    "environment": {
      "VULKAN_SDK": "RUTA_VULKAN",
      "PATH": "RUTA_QT/msvc2022_64/bin;RUTA_PYTHON;$penv{PATH}"
    }
  }],
  "buildPresets": [{"name":"windows-local","configurePreset":"windows-local"}],
  "testPresets": [{"name":"windows-local","configurePreset":"windows-local","output":{"outputOnFailure":true}}]
}
```

Desde una consola de desarrollo x64 de MSVC con CMake disponible:

```powershell
cmake --preset windows-local
cmake --build --preset windows-local
ctest --preset windows-local
```

## Ejecutar y verificar GPU

En este equipo se dejó `build/windows-local/run-workbench.cmd`: configura el entorno
MSVC y PATH de Qt/Python antes de abrir la aplicación. Es local e ignorado por Git.
En otro equipo puede arrancarse desde la consola de desarrollo con Qt/bin y Python
en PATH. El compilador de scripts necesita `INCLUDE`, `LIB` y herramientas de MSVC;
arrancar directamente el EXE desde un entorno vacío no está validado.

Se comprobó la resolución de las DLL mediante ese entorno, sin copiar SDK enteros.
El auxiliar `script_runner.exe` se encuentra mediante su ruta generada por CMake.
No se ha creado un instalador ni paquete redistribuible: Python, Qt y compilador
siguen siendo dependencias locales. Copiar solo `python313.dll` no incluye su biblioteca estándar.

Para repetir la prueba nativa desde el mismo entorno de desarrollo y con Qt/Python en PATH:

```powershell
$env:QT_QPA_PLATFORM = 'windows'
$env:WORKBENCH_GPU_TEST = '1'
$env:WORKBENCH_GPU_OUTPUT = "$PWD/build/windows-local/gpu-evidence"
$env:VK_INSTANCE_LAYERS = 'VK_LAYER_KHRONOS_validation;VK_LAYER_LUNARG_screenshot'
$env:VK_SCREENSHOT_FRAMES = '5,12,20,28,35,45'
$env:VK_SCREENSHOT_DIR = $env:WORKBENCH_GPU_OUTPUT
./build/windows-local/window_tests.exe nativeVulkanSmoke -o build/windows-local/gpu-test-results.txt,txt
```

El SDK guarda fotogramas PPM. Las capturas de QWidget/QScreen pueden quedar vacías
para superficies nativas; usar la capa de captura Vulkan para evaluar la imagen 3D.
El panel UV sí admite captura Qt. Los números de fotograma no identifican una etapa
fija: dependen del ritmo de presentación del equipo.

Resultados locales en `build/windows-local`: `tests.log`, `reader-results.txt`,
`window-results.txt`, `gpu-test-results.txt`, `gpu-test.log` y `gpu-evidence/`.
Esta última contiene `sphere.glb`, `sphere.obj`, `sphere.stl`, `sphere-uv.png`,
fotogramas PPM y `blender-import.json`.

## Límites pendientes

- Repetir la compilación y pruebas en macOS; no se ejecutaron en esta sesión Windows.
- Linux conserva su aviso de superficie nativa pendiente; no se ha compilado allí.
- Validar otras GPU, monitores con DPI distintos, suspensión y pérdida del dispositivo.
- Preparar distribución autónoma, DLL y Python redistribuibles, y el entorno de compilación de scripts.
- Ampliar la revisión visual de modelos importados; no se verificaron todos los parámetros de exportación en Blender.

No se añadieron texturas, RAM de procesos, descompresión ni persistencia completa de proyectos.
