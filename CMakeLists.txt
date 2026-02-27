cmake_minimum_required(VERSION 3.10)
project(teste LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Encontrar ferramentas de compilação de shaders
find_program(DXC_EXECUTABLE dxc REQUIRED)
find_program(SPIRV_CROSS_EXECUTABLE spirv-cross REQUIRED)

if(NOT DXC_EXECUTABLE OR NOT SPIRV_CROSS_EXECUTABLE)
    message(WARNING "DXC or SPIRV-Cross not found. Shader compilation will be skipped.")
    set(SKIP_SHADER_COMPILATION TRUE)
endif()

if(EMSCRIPTEN)
    include_directories(${CMAKE_SOURCE_DIR}/core/libs)
    add_compile_definitions(PLATFORM_WEBGL)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_DISABLE_EXCEPTION_CATCHING")
    
    # Build preload files list
    set(PRELOAD_FILES 
        "--preload-file ${CMAKE_SOURCE_DIR}/scene.scnb@scene.scnb"
        "--preload-file ${CMAKE_SOURCE_DIR}/px.png@px.png"
        "--preload-file ${CMAKE_SOURCE_DIR}/py.png@py.png"
        "--preload-file ${CMAKE_SOURCE_DIR}/pz.png@pz.png"
        "--preload-file ${CMAKE_SOURCE_DIR}/nx.png@nx.png"
        "--preload-file ${CMAKE_SOURCE_DIR}/ny.png@ny.png"
        "--preload-file ${CMAKE_SOURCE_DIR}/nz.png@nz.png"
    )
    
    file(GLOB GLSL_SHADERS "${CMAKE_SOURCE_DIR}/*.glsl")
    foreach(GLSL_FILE ${GLSL_SHADERS})
        get_filename_component(GLSL_NAME ${GLSL_FILE} NAME)
        list(APPEND PRELOAD_FILES "--preload-file ${GLSL_FILE}@${GLSL_NAME}")
    endforeach()

    file(GLOB OBJ_FILES "${CMAKE_SOURCE_DIR}/*.obj")
    foreach(OBJ_FILE ${OBJ_FILES})
        get_filename_component(OBJ_NAME ${OBJ_FILE} NAME)
        list(APPEND PRELOAD_FILES "--preload-file ${OBJ_FILE}@${OBJ_NAME}")
    endforeach()
    
    string(REPLACE ";" " " PRELOAD_FILES_STR "${PRELOAD_FILES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_SDL=2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 ${PRELOAD_FILES_STR}")
else()
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(SDL2 REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glm REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Vulkan REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR}/core/src)

# Scene compiler
if(EMSCRIPTEN)
    find_program(SCENE_COMPILER_EXE scene_compiler PATHS ${CMAKE_SOURCE_DIR}/tools REQUIRED)
    set(SCENE_COMPILER_CMD ${SCENE_COMPILER_EXE})
    set(SCENE_COMPILER_DEPS)
else()
    add_executable(scene_compiler core/src/scene_compiler.cpp)
    set_target_properties(scene_compiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/tools)
    set(SCENE_COMPILER_CMD scene_compiler)
    set(SCENE_COMPILER_DEPS scene_compiler)
endif()

file(GLOB SCENE_FILES "${CMAKE_SOURCE_DIR}/*.scn")
set(COMPILED_SCENES)
foreach(SCENE_FILE ${SCENE_FILES})
    get_filename_component(SCENE_NAME ${SCENE_FILE} NAME_WE)
    set(OUTPUT_FILE "${CMAKE_SOURCE_DIR}/${SCENE_NAME}.scnb")
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SCENE_COMPILER_CMD} ${SCENE_FILE} ${OUTPUT_FILE}
        DEPENDS ${SCENE_COMPILER_DEPS} ${SCENE_FILE}
        COMMENT "Compiling ${SCENE_NAME}.scn -> ${SCENE_NAME}.scnb"
    )
    list(APPEND COMPILED_SCENES ${OUTPUT_FILE})
endforeach()
add_custom_target(compile_scenes ALL DEPENDS ${COMPILED_SCENES})

# Main executable
file(GLOB_RECURSE SOURCES "core/src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/core/src/scene_compiler.cpp")

if(NOT EMSCRIPTEN)
    file(GLOB WEBGL_RENDERER_FILES "${CMAKE_SOURCE_DIR}/core/src/renderer/backends/webgl/*")
    list(REMOVE_ITEM SOURCES ${WEBGL_RENDERER_FILES})
endif()

if(EMSCRIPTEN)
    file(GLOB OPENGL_RENDERER_FILES "${CMAKE_SOURCE_DIR}/core/src/renderer/backends/opengl/*")
    list(REMOVE_ITEM SOURCES ${OPENGL_RENDERER_FILES})
    file(GLOB VULKAN_RENDERER_FILES "${CMAKE_SOURCE_DIR}/core/src/renderer/backends/vulkan/*")
    list(REMOVE_ITEM SOURCES ${VULKAN_RENDERER_FILES})
    file(GLOB DX12_RENDERER_FILES "${CMAKE_SOURCE_DIR}/core/src/renderer/backends/directx12/*")
    list(REMOVE_ITEM SOURCES ${DX12_RENDERER_FILES})
endif()

add_executable(main ${SOURCES})

if(NOT EMSCRIPTEN)
    target_link_libraries(main 
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        GLEW::GLEW
        glm::glm
        OpenGL::GL
        Vulkan::Vulkan
        d3d12
        dxgi
        d3dcompiler
    )
endif()

add_dependencies(main Shaders)

# Shader compilation - HLSL to SPIR-V only
file(GLOB VXS_SHADERS "${CMAKE_SOURCE_DIR}/*.vxs")
file(GLOB PXS_SHADERS "${CMAKE_SOURCE_DIR}/*.pxs")

if(EMSCRIPTEN)
    set(SPIRV_CROSS_ARGS --es --version 300)
else()
    set(SPIRV_CROSS_ARGS --no-es --version 330 --separate-shader-objects)
endif()

foreach(SHADER_FILE ${VXS_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(SPIRV_FILE "${CMAKE_SOURCE_DIR}/${SHADER_NAME}.vxs.spv")
    set(GLSL_FILE "${CMAKE_SOURCE_DIR}/${SHADER_NAME}.vxs.glsl")

    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${DXC_EXECUTABLE} -spirv -T vs_6_0 -E main ${SHADER_FILE} -Fo ${SPIRV_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling ${SHADER_FILE} -> ${SPIRV_FILE}"
    )
    
    if(EMSCRIPTEN)
        add_custom_command(
            OUTPUT ${GLSL_FILE}
            COMMAND ${SPIRV_CROSS_EXECUTABLE} ${SPIRV_CROSS_ARGS} ${SPIRV_FILE} --output ${GLSL_FILE}
            COMMAND sed -i 's/^out vec3 out_var_/out highp vec3 out_var_/g' ${GLSL_FILE}
            DEPENDS ${SPIRV_FILE}
            COMMENT "Converting ${SPIRV_FILE} -> ${GLSL_FILE}"
        )        
    else()
        add_custom_command(
            OUTPUT ${GLSL_FILE}
            COMMAND ${SPIRV_CROSS_EXECUTABLE} ${SPIRV_CROSS_ARGS} ${SPIRV_FILE} --output ${GLSL_FILE}
            DEPENDS ${SPIRV_FILE}
            COMMENT "Converting ${SPIRV_FILE} -> ${GLSL_FILE}"
        )
    endif()
    
    list(APPEND GLSL_FILES ${GLSL_FILE})
endforeach()

foreach(SHADER_FILE ${PXS_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(SPIRV_FILE "${CMAKE_SOURCE_DIR}/${SHADER_NAME}.pxs.spv")
    set(GLSL_FILE "${CMAKE_SOURCE_DIR}/${SHADER_NAME}.pxs.glsl")
    
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${DXC_EXECUTABLE} -spirv -T ps_6_0 -E main ${SHADER_FILE} -Fo ${SPIRV_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling ${SHADER_FILE} -> ${SPIRV_FILE}"
    )
    
    add_custom_command(
        OUTPUT ${GLSL_FILE}
        COMMAND ${SPIRV_CROSS_EXECUTABLE} ${SPIRV_CROSS_ARGS} ${SPIRV_FILE} --output ${GLSL_FILE}
        DEPENDS ${SPIRV_FILE}
        COMMENT "Converting ${SPIRV_FILE} -> ${GLSL_FILE}"
    )
    
    list(APPEND GLSL_FILES ${GLSL_FILE})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${GLSL_FILES})
add_dependencies(main compile_scenes)
