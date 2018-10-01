cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(VulkanIsland VERSION ${PROJECT_VERSION})

configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_SOURCE_DIR}/include/config.h"
)

find_package(Vulkan REQUIRED)

if(NOT WIN32)
find_package(glm REQUIRED)
find_package(X11 REQUIRED)
endif()

find_package(glfw3 3.3 REQUIRED)

include_directories(include)

set(SOURCE_FILES
        src/buffer.hxx                          src/buffer.cxx
        src/command_buffer.hxx
        src/debug.hxx                           src/debug.cxx
        src/device.hxx                          src/device.cxx
        src/device_defaults.hxx
        src/glTFLoader.hxx                      src/glTFLoader.cxx
        src/helpers.hxx
        src/image.hxx                           src/image.cxx
        src/instance.hxx                        src/instance.cxx
        src/math.hxx
        src/mesh.hxx
        src/program.hxx
        src/queue_builder.hxx
        src/queues.hxx
        src/resource.hxx                        src/resource.cxx
        src/scene_tree.hxx                      src/scene_tree.cxx
        src/swapchain.hxx                       src/swapchain.cxx
        src/TARGA_loader.hxx                    src/TARGA_loader.cxx
        src/transform.hxx

        src/main.cxx                            src/main.hxx
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}

        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF

        POSITION_INDEPENDENT_CODE YES

        DEBUG_POSTFIX _d
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -pedantic
            -Wall
            -Wextra
            #-fsanitize=thread -fsanitize=address
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
            /Wall
)

endif()

find_package(Boost REQUIRED COMPONENTS
        filesystem
)

target_include_directories(${PROJECT_NAME} PRIVATE
	Vulkan::Vulkan
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        -Wl,-no-undefined
        -Wl,-no-allow-shlib-undefined
        -Wl,-unresolved-symbols=report-all

        dl
        X11

        pthread

        Boost::boost
        Boost::filesystem

        Vulkan::Vulkan

        glm
        glfw3
)
