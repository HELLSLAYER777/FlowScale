cmake_minimum_required(VERSION 3.24)
project(FlowScale VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FLOWSCALE_WITH_NEURAL "Enable the ONNX Runtime neural engine" OFF)
option(FLOWSCALE_BUILD_TESTS "Build the pure-math unit tests (build on any OS)" ON)

# --------------------------------------------------------------- sources ----
set(FS_UTIL_SRC
    src/core/Settings.cpp
    src/core/Profile.cpp
    src/core/Stats.cpp
)

set(FS_APP_SRC
    src/app/main.cpp
    src/app/App.cpp
    src/app/MainWindow.cpp
    src/core/Engine.cpp
    src/gfx/Device.cpp
    src/gfx/Presenter.cpp
    src/gfx/CShader.cpp
    src/gfx/Hud.cpp
    src/capture/WGCCapture.cpp
    src/capture/DDACapture.cpp
    src/scaling/ScalerChain.cpp
    src/framegen/FrameGen.cpp
)

if (FLOWSCALE_WITH_NEURAL)
    list(APPEND FS_APP_SRC src/neural/NeuralEngine.cpp)
endif()

# ------------------------------------------------------------- main exe -----
if (WIN32)
    add_executable(FlowScale WIN32 ${FS_APP_SRC} ${FS_UTIL_SRC})

    # Sources include their headers as "mod/Header.h" relative to src/.
    target_include_directories(FlowScale PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    target_compile_definitions(FlowScale PRIVATE
        _WIN32_WINNT=0x0A00
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE _UNICODE
        FS_VERSION="0.1.0"
    )
    if (FLOWSCALE_WITH_NEURAL)
        target_compile_definitions(FlowScale PRIVATE FS_WITH_NEURAL=1)
        # Route A: an onnxruntime CMake package (e.g. the vcpkg port -- built from source, slow).
        find_package(onnxruntime CONFIG QUIET)
        if (onnxruntime_FOUND)
            if (TARGET onnxruntime::onnxruntime)
                target_link_libraries(FlowScale PRIVATE onnxruntime::onnxruntime)
            else()
                target_link_libraries(FlowScale PRIVATE onnxruntime)
            endif()
            message(STATUS "Neural engine: using onnxruntime CMake package")
        else()
            # Route B (recommended, what CI uses): the official prebuilt release zip.
            #   scripts\fetch-onnxruntime.ps1  ->  _deps\onnxruntime-win-x64-<ver>\
            # then configure with -DONNXRUNTIME_ROOT_DIR=<that directory>
            set(ONNXRUNTIME_ROOT_DIR "" CACHE PATH
                "Prebuilt onnxruntime-win-x64 directory containing include/ and lib/")
            find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
                HINTS "${ONNXRUNTIME_ROOT_DIR}/include" REQUIRED)
            find_library(ONNXRUNTIME_LIBRARY onnxruntime
                HINTS "${ONNXRUNTIME_ROOT_DIR}/lib" REQUIRED)
            target_include_directories(FlowScale PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
            target_link_libraries(FlowScale PRIVATE "${ONNXRUNTIME_LIBRARY}")
            # Ship the runtime DLLs next to the exe so the zip is self-contained.
            get_filename_component(_ort_bin_dir "${ONNXRUNTIME_LIBRARY}" DIRECTORY)
            foreach(_ort_dll onnxruntime.dll onnxruntime_providers_shared.dll)
                if (EXISTS "${_ort_bin_dir}/${_ort_dll}")
                    add_custom_command(TARGET FlowScale POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${_ort_bin_dir}/${_ort_dll}" $<TARGET_FILE_DIR:FlowScale>)
                endif()
            endforeach()
            unset(_ort_bin_dir)
            unset(_ort_dll)
            message(STATUS "Neural engine: using prebuilt onnxruntime from ${ONNXRUNTIME_ROOT_DIR}")
        endif()
    endif()

    target_compile_options(FlowScale PRIVATE /permissive- /W4 /MP /O2 /Zi /EHsc)
    target_link_libraries(FlowScale PRIVATE
        d3d11 dxgi d3dcompiler d2d1 dwrite
        windowsapp runtimeobject
        user32 gdi32 shell32 advapi32 ole32
        comctl32 dwmapi psapi shlwapi
    )

    find_package(nlohmann_json CONFIG REQUIRED)
    target_link_libraries(FlowScale PRIVATE nlohmann_json::nlohmann_json)

    # Runtime shader assets: the engine compiles these with D3DCompile at start,
    # so a shader tweak never requires a rebuild of the exe.
    add_custom_command(TARGET FlowScale POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/src/scaling/shaders $<TARGET_FILE_DIR:FlowScale>/shaders/scaling
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/src/framegen/shaders $<TARGET_FILE_DIR:FlowScale>/shaders/framegen
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/shaders/user           $<TARGET_FILE_DIR:FlowScale>/shaders/user
        COMMAND ${CMAKE_COMMAND} -E make_directory         $<TARGET_FILE_DIR:FlowScale>/models
        COMMENT "Copying HLSL sources next to the executable"
    )
else()
    message(STATUS "Non-Windows host: only the pure-math tests will be built here; run the app build on Windows.")
endif()

# ----------------------------------------------------------------- tests ----
if (FLOWSCALE_BUILD_TESTS)
    add_executable(fs_math_tests tests/math_tests.cpp src/core/Stats.cpp)
    target_include_directories(fs_math_tests PRIVATE src)
    target_compile_definitions(fs_math_tests PRIVATE NOMINMAX)
    if (MSVC)
        target_compile_options(fs_math_tests PRIVATE /W4 /EHsc)
    else()
        target_compile_options(fs_math_tests PRIVATE -Wall -Wextra -O2)
    endif()
    enable_testing()
    add_test(NAME math_tests COMMAND fs_math_tests)
endif()

if (TARGET FlowScale)
    install(TARGETS FlowScale RUNTIME DESTINATION .)
endif()
