cmake_minimum_required(VERSION 3.24)
project(NinesXD VERSION 0.3.6 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    # Localized compiler detection can be misdecoded by a Windows console.
    # Allow a machine preset to supply the exact UTF-8 /showIncludes prefix.
    set(WORKBENCH_MSVC_INCLUDE_PREFIX "" CACHE STRING "Override localized MSVC include prefix when automatic detection is misdecoded")
    if(WORKBENCH_MSVC_INCLUDE_PREFIX)
        set(CMAKE_CL_SHOWINCLUDES_PREFIX "${WORKBENCH_MSVC_INCLUDE_PREFIX}")
        set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${WORKBENCH_MSVC_INCLUDE_PREFIX}")
    endif()
    # Source strings and scripting output use UTF-8 on every platform.
    add_compile_options(/utf-8 /Zc:__cplusplus)
    # Main window integration and UI regression translation units exceed COFF section limits.
    set_source_files_properties(src/main.cpp tests/window_tests.cpp PROPERTIES COMPILE_OPTIONS /bigobj)
    add_compile_definitions(NOMINMAX)
endif()
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)
if(NOT TARGET Qt6::QTiffPlugin OR NOT TARGET Qt6::QWebpPlugin)
    message(FATAL_ERROR "UV/2D requires the Qt Image Formats module (TIFF and WebP plugins). Install qtimageformats for this Qt version.")
endif()
# Native archive decoding uses zlib, bundled by Qt where available.
find_package(Qt6ZlibPrivate QUIET)
if(TARGET Qt6::ZlibPrivate)
    link_libraries(Qt6::ZlibPrivate)
    add_compile_definitions(WORKBENCH_QT_ZLIB)
else()
    find_package(ZLIB REQUIRED)
    link_libraries(ZLIB::ZLIB)
endif()
# Keep LGPL LZX decoder in a replaceable shared library.
add_library(nines_lzx SHARED third_party/libmspack/lzxd.c third_party/libmspack/nines_lzx.c)
set_target_properties(nines_lzx PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(nines_lzx PRIVATE third_party/libmspack)
link_libraries(nines_lzx)
add_compile_definitions(WORKBENCH_LZX)
find_package(Vulkan REQUIRED)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter Development.Embed)
qt_standard_project_setup()
set(WORKBENCH_VULKAN_DRIVER "" CACHE FILEPATH "Optional local Vulkan driver manifest")
configure_file(src/local_config.h.in generated/local_config.h @ONLY)
qt_add_executable(binary_workbench src/main.cpp src/binary_reader.cpp src/binary_reader.h src/hex_view.cpp src/hex_view.h src/interpretation.cpp src/chunk_cache.cpp src/binary_document.cpp src/byte_format.cpp src/format_panel.cpp src/pattern_search.cpp src/magic_bytes.cpp src/magic_panel.cpp src/entropy.cpp src/entropy_panel.cpp src/direct_edit.cpp src/mesh_data.cpp)
if(WIN32)
    set_target_properties(binary_workbench PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
if(APPLE)
    set_target_properties(binary_workbench PROPERTIES MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.binaryworkbench.dev"
        MACOSX_BUNDLE_BUNDLE_NAME "9XD")
endif()
target_include_directories(binary_workbench PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(binary_workbench PRIVATE Qt6::Widgets Qt6::Concurrent Vulkan::Vulkan)
if(NOT WIN32)
    find_package(Iconv REQUIRED)
    target_link_libraries(binary_workbench PRIVATE Iconv::Iconv)
endif()
get_filename_component(VULKAN_LIBRARY_DIR "${Vulkan_LIBRARY}" DIRECTORY)
set_property(TARGET binary_workbench APPEND PROPERTY BUILD_RPATH "${VULKAN_LIBRARY_DIR}")
include(CTest)
if(BUILD_TESTING)
    qt_add_executable(native_asset_audit src/native_asset_audit.cpp src/binary_reader.cpp src/binary_reader.h src/binary_document.cpp)
    target_include_directories(native_asset_audit PRIVATE src)
    target_link_libraries(native_asset_audit PRIVATE Qt6::Core Qt6::Gui Qt6::Concurrent)
endif()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(reader_tests tests/reader_tests.cpp src/binary_reader.cpp src/binary_reader.h src/hex_view.cpp src/hex_view.h src/interpretation.cpp src/chunk_cache.cpp src/binary_document.cpp src/byte_format.cpp src/format_panel.cpp src/pattern_search.cpp src/magic_bytes.cpp src/magic_panel.cpp src/entropy.cpp src/entropy_panel.cpp src/direct_edit.cpp src/mesh_data.cpp)
    target_include_directories(reader_tests PRIVATE src)
    target_link_libraries(reader_tests PRIVATE Qt6::Test Qt6::Concurrent Qt6::Widgets)
    if(NOT WIN32)
        target_link_libraries(reader_tests PRIVATE Iconv::Iconv)
    endif()
    add_test(NAME reader_tests COMMAND reader_tests -o -,txt)
    set_tests_properties(reader_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

if(BUILD_TESTING)
    qt_add_executable(window_tests tests/window_tests.cpp src/binary_reader.cpp src/binary_reader.h src/hex_view.cpp src/hex_view.h src/interpretation.cpp src/chunk_cache.cpp src/binary_document.cpp src/byte_format.cpp src/format_panel.cpp src/pattern_search.cpp src/magic_bytes.cpp src/magic_panel.cpp src/entropy.cpp src/entropy_panel.cpp src/direct_edit.cpp src/mesh_data.cpp)
    target_include_directories(window_tests PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(window_tests PRIVATE Qt6::Test Qt6::Concurrent Qt6::Widgets)
    if(NOT WIN32)
        target_link_libraries(window_tests PRIVATE Iconv::Iconv)
    endif()
    add_test(NAME window_tests COMMAND window_tests -o -,txt)
    set_tests_properties(window_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

foreach(target binary_workbench window_tests)
    if(TARGET ${target})
        target_sources(${target} PRIVATE src/export_dialog.cpp src/layout_persistence.cpp src/vulkan_panel.cpp src/vulkan_panel.h src/mesh_controls.cpp src/mesh_collection.cpp src/uv_panel.cpp src/uv_panel.h)
        target_link_libraries(${target} PRIVATE Vulkan::Vulkan)
        set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH "${VULKAN_LIBRARY_DIR}")
        if(APPLE)
            enable_language(OBJCXX)
            target_sources(${target} PRIVATE src/metal_surface.mm)
            target_link_libraries(${target} PRIVATE "-framework Cocoa" "-framework QuartzCore")
        endif()
    endif()
endforeach()

find_program(WORKBENCH_GLSLC glslc HINTS "${Vulkan_INCLUDE_DIR}/../bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)
set(mesh_shaders)
foreach(stage vert frag)
    set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/mesh.${stage}.spv")
    add_custom_command(OUTPUT "${output}" COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND "${WORKBENCH_GLSLC}" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh.${stage}" -o "${output}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh.${stage}" VERBATIM)
    list(APPEND mesh_shaders "${output}")
endforeach()
foreach(target binary_workbench window_tests)
    if(TARGET ${target})
        qt_add_resources(${target} mesh_shaders PREFIX "/shaders" BASE "${CMAKE_CURRENT_BINARY_DIR}/shaders" FILES ${mesh_shaders})
    endif()
endforeach()

qt_add_executable(script_runner src/script_runner.cpp src/script_context.cpp src/binary_reader.cpp src/binary_reader.h src/binary_document.cpp)
set_target_properties(script_runner PROPERTIES MACOSX_BUNDLE FALSE)
target_include_directories(script_runner PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(script_runner PRIVATE Qt6::Core Qt6::Concurrent Python3::Python)
foreach(target binary_workbench window_tests)
    if(TARGET ${target})
        target_sources(${target} PRIVATE src/script_panel.cpp src/script_context.cpp)
        add_dependencies(${target} script_runner)
        target_compile_definitions(${target} PRIVATE WORKBENCH_SCRIPT_RUNNER="$<TARGET_FILE:script_runner>")
        qt_add_resources(${target} scripting_api PREFIX "/scripting" BASE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES src/workbench_script_api.h)
    endif()
endforeach()

foreach(target binary_workbench reader_tests window_tests)
    if(TARGET ${target})
        target_sources(${target} PRIVATE src/model_export.cpp src/recent_files.cpp src/data_inspector.cpp src/data_inspector.h src/mesh_debug.cpp src/mesh_scene.cpp src/mesh_scene.h)
    endif()
endforeach()

# Test executables need a console subsystem for useful CTest diagnostics on Windows.
if(WIN32 AND BUILD_TESTING)
    set_target_properties(reader_tests window_tests PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

if(WIN32)
    foreach(target binary_workbench reader_tests window_tests)
        if(TARGET ${target})
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/imageformats"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt6::QTiffPlugin>" "$<TARGET_FILE_DIR:${target}>/imageformats/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Qt6::QWebpPlugin>" "$<TARGET_FILE_DIR:${target}>/imageformats/")
        endif()
    endforeach()
endif()
add_custom_command(TARGET binary_workbench POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:binary_workbench>/licenses"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/bcdec-LICENSE.txt" "$<TARGET_FILE_DIR:binary_workbench>/licenses/bcdec.txt")

if(WIN32)
    find_file(WORKBENCH_PACKAGE_VULKAN_LOADER NAMES vulkan-1.dll
        PATHS "$ENV{VULKAN_SDK}/Bin" "$ENV{SystemRoot}/System32" NO_DEFAULT_PATH)
    if(WORKBENCH_PACKAGE_VULKAN_LOADER)
        get_target_property(workbench_qmake Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(workbench_qt_bin "${workbench_qmake}" DIRECTORY)
        add_custom_target(windows_package
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/package_windows.py"
                --build-dir "${CMAKE_CURRENT_BINARY_DIR}" --qt-bin "${workbench_qt_bin}"
                --python "${Python3_EXECUTABLE}" --version "${PROJECT_VERSION}"
                --vulkan-loader "${WORKBENCH_PACKAGE_VULKAN_LOADER}"
                --vulkan-license "${CMAKE_CURRENT_SOURCE_DIR}/tools/Vulkan-Loader-LICENSE.txt"
            DEPENDS binary_workbench script_runner USES_TERMINAL
            COMMENT "Create and verify the Windows Release ZIP (windows-release profile)")
    endif()
endif()

# Dependency probes may reload CMakeCXXCompiler.cmake, restoring a misdecoded
# localized prefix. Apply the explicit override again after all package probes.
if(MSVC AND WORKBENCH_MSVC_INCLUDE_PREFIX)
    set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${WORKBENCH_MSVC_INCLUDE_PREFIX}")
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "${WORKBENCH_MSVC_INCLUDE_PREFIX}")
endif()

foreach(target binary_workbench window_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE Qt6::Network)
    endif()
endforeach()

# Optional, read-only batch validation adapter; shares the production decoders.
option(WORKBENCH_BUILD_DECODER_PROBE "Build the headless AI testing adapter" ${BUILD_TESTING})
if(WORKBENCH_BUILD_DECODER_PROBE)
    add_executable(decoder_probe src/decoder_probe.cpp src/binary_reader.cpp src/binary_reader.h src/binary_document.cpp src/mesh_data.cpp)
    set_target_properties(decoder_probe PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)
    target_include_directories(decoder_probe PRIVATE src)
    target_link_libraries(decoder_probe PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent)
endif()

# Autodesk SDK is a developer dependency; end users only need the runtime DLL.
set(WORKBENCH_FBX_SDK_ROOT "" CACHE PATH "Autodesk FBX SDK root (include/fbxsdk.h and lib)")
foreach(target binary_workbench reader_tests window_tests)
    if(TARGET ${target})
        target_sources(${target} PRIVATE src/fbx_export.cpp)
        if(WORKBENCH_FBX_SDK_ROOT)
            if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
                message(FATAL_ERROR "This FBX SDK integration currently requires Windows x64 MSVC")
            endif()
            if(NOT EXISTS "${WORKBENCH_FBX_SDK_ROOT}/include/fbxsdk.h" OR NOT EXISTS "${WORKBENCH_FBX_SDK_ROOT}/lib/x64/release/libfbxsdk.lib")
                message(FATAL_ERROR "WORKBENCH_FBX_SDK_ROOT is not a supported FBX SDK installation")
            endif()
            target_include_directories(${target} PRIVATE "${WORKBENCH_FBX_SDK_ROOT}/include")
            target_compile_definitions(${target} PRIVATE WORKBENCH_FBX_SDK FBXSDK_SHARED)
            target_link_libraries(${target} PRIVATE "${WORKBENCH_FBX_SDK_ROOT}/lib/x64/release/libfbxsdk.lib")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WORKBENCH_FBX_SDK_ROOT}/lib/x64/release/libfbxsdk.dll" "$<TARGET_FILE_DIR:${target}>/libfbxsdk.dll"
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/licenses"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WORKBENCH_FBX_SDK_ROOT}/License.rtf" "$<TARGET_FILE_DIR:${target}>/licenses/Autodesk-FBX-SDK.rtf")
        endif()
    endif()
endforeach()

# Public logo resources are always available in source builds.
foreach(target binary_workbench window_tests)
    if(TARGET ${target})
        qt_add_resources(${target} public_branding PREFIX "/branding" BASE assets FILES assets/logo-sticker.png)
        qt_add_resources(${target} mesh_profiles PREFIX "/mesh-profiles" BASE profiles FILES profiles/nfsu2-pc.json)
        # Optional local artwork is embedded only when present; public builds use the logo.
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/about-splash.png")
            qt_add_resources(${target} local_branding PREFIX "/branding" BASE assets FILES assets/about-splash.png)
        endif()
    endif()
endforeach()
if(WIN32)
    enable_language(RC)
    target_sources(binary_workbench PRIVATE assets/app.rc)
endif()

# Reassert after all language/resource probes, including the Windows RC language.
if(MSVC AND WORKBENCH_MSVC_INCLUDE_PREFIX)
    set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${WORKBENCH_MSVC_INCLUDE_PREFIX}")
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "${WORKBENCH_MSVC_INCLUDE_PREFIX}")
endif()

add_custom_command(TARGET binary_workbench POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:binary_workbench>/licenses/libmspack-source"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libmspack" "$<TARGET_FILE_DIR:binary_workbench>/licenses/libmspack-source")
