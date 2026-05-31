cmake_minimum_required(VERSION 3.24)

project(nozzle_ffgl
    VERSION 0.1.0
    DESCRIPTION "FFGL plugins for nozzle texture sharing"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NOZZLE_INSTALL OFF CACHE BOOL "Disable nozzle installation" FORCE)
set(NOZZLE_BUILD_TESTS OFF CACHE BOOL "Disable nozzle tests for plugin builds" FORCE)
set(NOZZLE_BUILD_EXAMPLES OFF CACHE BOOL "Disable nozzle examples for plugin builds" FORCE)
set(NOZZLE_BUILD_OPENGL ON CACHE BOOL "Enable nozzle OpenGL interop" FORCE)
add_subdirectory(nozzle)

set(FFGL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/ffgl")
set(FFGL_LIB_DIR "${FFGL_ROOT}/source/lib")
set(FFGL_SDK_SOURCE "${FFGL_LIB_DIR}/FFGLSDK.cpp")

if(NOT EXISTS "${FFGL_SDK_SOURCE}")
    message(FATAL_ERROR "missing FFGL SDK source: ${FFGL_SDK_SOURCE}")
endif()

add_library(nozzle_ffgl_common STATIC
    src/common/nozzle_ffgl_common.cpp
)
target_include_directories(nozzle_ffgl_common PUBLIC
    src/common
    "${FFGL_LIB_DIR}"
)
target_link_libraries(nozzle_ffgl_common PUBLIC nozzle)
target_compile_definitions(nozzle_ffgl_common PUBLIC
    $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION=1>
)
target_compile_options(nozzle_ffgl_common PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/EHs-c->
)

function(add_nozzle_ffgl_plugin target_name source_file bundle_identifier)
    add_library(${target_name} MODULE
        ${source_file}
        ${FFGL_SDK_SOURCE}
    )
    target_include_directories(${target_name} PRIVATE
        src/common
        "${FFGL_LIB_DIR}"
    )
    target_link_libraries(${target_name} PRIVATE nozzle_ffgl_common nozzle)
    target_compile_definitions(${target_name} PRIVATE
        $<$<PLATFORM_ID:Windows>:TARGET_OS_WIN=1>
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
    )
    target_compile_options(${target_name} PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-unused-variable>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fno-exceptions>
        $<$<CXX_COMPILER_ID:MSVC>:/W3>
        $<$<CXX_COMPILER_ID:MSVC>:/EHs-c->
    )

    if(APPLE)
        target_link_libraries(${target_name} PRIVATE
            "-framework OpenGL"
            "-framework Carbon"
            "-framework AppKit"
        )
        set_target_properties(${target_name} PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "bundle"
            PREFIX ""
            OUTPUT_NAME "${target_name}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FFGLPluginInfo.plist.in"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${bundle_identifier}"
            MACOSX_BUNDLE_BUNDLE_NAME "${target_name}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        )
    elseif(WIN32)
        target_link_libraries(${target_name} PRIVATE opengl32 d3d11 dxgi bcrypt)
        set_target_properties(${target_name} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${target_name}"
            SUFFIX ".dll"
        )
    else()
        message(FATAL_ERROR "nozzle-ffgl first pass supports macOS and Windows only")
    endif()
endfunction()

add_nozzle_ffgl_plugin(NozzleReceive src/receive/nozzle_receive.cpp org.nozzle-io.ffgl.receive)
add_nozzle_ffgl_plugin(NozzleSend src/send/nozzle_send.cpp org.nozzle-io.ffgl.send)
