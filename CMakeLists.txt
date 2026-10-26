cmake_minimum_required(VERSION 3.16)

# glad.c 为 C 源文件，因此同时启用 C 与 CXX
project(LearnOpenGL LANGUAGES C CXX)

# ============================================================================
# 全局编译选项
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(IMAGE_DIR "${PROJECT_SOURCE_DIR}/image/")
add_compile_definitions(IMAGE_DIR="${IMAGE_DIR}")

# 生成 compile_commands.json，方便 VS Code 的 clangd / C/C++ 插件使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# 目录设置
# ============================================================================
set(INCLUDE_DIR        ${CMAKE_SOURCE_DIR}/include)
set(EXTERNAL_LIB_DIR   ${CMAKE_SOURCE_DIR}/lib)

# 可执行文件输出目录（与 .vscode/launch.json 中的 ${workspaceFolder}/output 对应）
set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/output CACHE PATH "Executable output directory")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# ============================================================================
# 查找系统依赖
# ============================================================================
find_package(OpenGL REQUIRED)

# ============================================================================
# 1. GLAD —— 从源码编译（include/glad/glad.c）
# ============================================================================
add_library(glad STATIC ${INCLUDE_DIR}/glad/glad.c)
target_include_directories(glad PUBLIC ${INCLUDE_DIR})

# ============================================================================
# 2. ImGui —— 从源码编译（include/imgui/*.cpp）
#    注意：imgui_impl_opengl3.cpp 自带加载器(imgui_impl_opengl3_loader.h)，
#    无需额外定义 IMGUI_IMPL_OPENGL_LOADER_GLAD
# ============================================================================
set(IMGUI_SOURCES
  ${INCLUDE_DIR}/imgui/imgui.cpp
  ${INCLUDE_DIR}/imgui/imgui_demo.cpp
  ${INCLUDE_DIR}/imgui/imgui_draw.cpp
  ${INCLUDE_DIR}/imgui/imgui_tables.cpp
  ${INCLUDE_DIR}/imgui/imgui_widgets.cpp
  ${INCLUDE_DIR}/imgui/imgui_impl_glfw.cpp
  ${INCLUDE_DIR}/imgui/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${INCLUDE_DIR})

# ============================================================================
# 3. GLFW / Assimp —— 使用 lib/ 下预编译的导入库
#    Windows(MinGW)：libglfw3dll.a / libassimp.dll.a（配合根目录 assimp.dll）
#    其他平台：通过 find_package 查找系统安装
# ============================================================================
if(WIN32)
  set(GLFW_LIB   ${EXTERNAL_LIB_DIR}/libglfw3dll.a)
  set(ASSIMP_LIB ${EXTERNAL_LIB_DIR}/libassimp.dll.a)
else()
  find_package(glfw3 REQUIRED)
  find_package(assimp REQUIRED)
  set(GLFW_LIB   glfw)
  set(ASSIMP_LIB assimp)
endif()

# imgui 的 GLFW 后端依赖 GLFW
target_link_libraries(imgui PUBLIC glad OpenGL::GL ${GLFW_LIB})

# ============================================================================
# 4. 通用链接依赖（所有可执行目标共用）
# ============================================================================
set(COMMON_LIBS
  glad
  imgui
  OpenGL::GL
  ${GLFW_LIB}
  ${ASSIMP_LIB}
)

# ============================================================================
# 5. 添加可执行目标
#    - 每个 lesson 目录下的每个 .cpp 文件都是一个独立程序（自带 main()）
#    - 命名规则：目录名；若目录内有多个源文件则追加文件名，如
#      01_hello_window / 02_hello_triangle_draw_triangle
#    - 根目录 src/main.cpp 生成 main
# ============================================================================

# 设置 VS Code / Visual Studio 调试时的工作目录为项目根目录
# （shader/纹理 均通过相对路径 ./src/... 加载，必须从根目录运行）
set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

function(add_lesson_target target_name source_file)
  add_executable(${target_name} ${source_file})
  target_include_directories(${target_name} PRIVATE ${INCLUDE_DIR})
  target_link_libraries(${target_name} PRIVATE ${COMMON_LIBS})
  set_target_properties(${target_name} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  # 便捷运行目标：cmake --build build --target run_<名称>
  # 自动以项目根目录为工作目录运行（shader/纹理为相对路径，必须从根目录启动）
  add_custom_target(run_${target_name}
    COMMAND $<TARGET_FILE:${target_name}>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Running ${target_name} (working dir: project root)")
  # 收集所有目标名，供后续 DLL 复制使用
  set_property(GLOBAL APPEND PROPERTY LESSON_TARGETS_PROP ${target_name})
  message(STATUS "  + ${target_name}  <-  ${source_file}")
endfunction()

message(STATUS "Building targets:")

# 根目录的 main.cpp
add_lesson_target(main ${CMAKE_SOURCE_DIR}/src/main.cpp)

# 遍历 src/ 下的所有 lesson 目录
file(GLOB LESSON_DIRS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*/)

set(i 1)

foreach(lesson_dir IN LISTS LESSON_DIRS)
  get_filename_component(lesson_name ${lesson_dir} NAME)

  # 跳过构建产物目录
  if(lesson_name STREQUAL "build" OR lesson_name STREQUAL "output")
    continue()
  endif()

  file(GLOB LESSON_SOURCES CONFIGURE_DEPENDS ${lesson_dir}/*.cpp)

  foreach(source_file IN LISTS LESSON_SOURCES)
    # 每个 .cpp 文件都是一个独立程序，编号必须在内层循环里递增，
    # 否则同一目录下的多个文件会得到相同的目标名（如 02_hello_triangle）
    add_lesson_target("lesson_${i}" ${source_file})
    math(EXPR i "${i} + 1")
  endforeach()

endforeach()

# ============================================================================
# 6. 运行时 DLL 处理（Windows）
#    把需要的 DLL 复制到输出目录，保证程序可直接运行
# ============================================================================
if(WIN32)
  get_property(LESSON_TARGETS GLOBAL PROPERTY LESSON_TARGETS_PROP)

  # 需要复制到输出目录的 DLL：列表项格式为 "源路径|目标文件名"
  set(DLL_COPY_PAIRS)

  # assimp：导入库 libassimp.dll.a 引用的是 libassimp-6.dll
  # 优先使用根目录直接存在的 libassimp-6.dll；
  # 若根目录只有 assimp.dll，则复制时重命名为 libassimp-6.dll
  if(EXISTS ${CMAKE_SOURCE_DIR}/libassimp-6.dll)
    list(APPEND DLL_COPY_PAIRS "${CMAKE_SOURCE_DIR}/libassimp-6.dll|libassimp-6.dll")
  elseif(EXISTS ${CMAKE_SOURCE_DIR}/assimp.dll)
    list(APPEND DLL_COPY_PAIRS "${CMAKE_SOURCE_DIR}/assimp.dll|libassimp-6.dll")
  endif()

  # glfw3.dll：可能在项目根目录、lib、output 目录或系统 PATH 中
  find_file(GLFW3_DLL NAMES glfw3.dll
    HINTS ${EXTERNAL_LIB_DIR} ${CMAKE_SOURCE_DIR} ${OUTPUT_DIR} ENV PATH
    NO_DEFAULT_PATH)
  if(GLFW3_DLL)
    list(APPEND DLL_COPY_PAIRS "${GLFW3_DLL}|glfw3.dll")
  endif()

  # 为每个可执行目标添加复制 DLL 的构建后命令
  if(DLL_COPY_PAIRS)
    foreach(target_name IN LISTS LESSON_TARGETS)
      foreach(pair IN LISTS DLL_COPY_PAIRS)
        string(REPLACE "|" ";" pair_list "${pair}")
        list(GET pair_list 0 src_dll)
        list(GET pair_list 1 dst_name)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${src_dll} ${OUTPUT_DIR}/${dst_name}
          COMMENT "Copying ${src_dll} -> ${OUTPUT_DIR}/${dst_name}")
      endforeach()
    endforeach()
  endif()
endif()
