cmake_minimum_required(VERSION 3.11.1)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(VulkanIsland VERSION ${PROJECT_VERSION})

configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_SOURCE_DIR}/includes/config.h"
)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(X11 REQUIRED)
find_package(glfw3 3.3 REQUIRED)

set(SOURCE_FILES
        src/device_defaults.h
        src/helpers.h
        src/program.h
        src/queue_builder.h
        src/queues.h
        src/buffer.cxx                          src/buffer.h
        src/debug.cxx                           src/debug.h
        src/device.cxx                          src/device.h
        src/instance.cxx                        src/instance.h
        src/mesh_loader.cxx                     src/mesh_loader.h
        src/swapchain.cxx                       src/swapchain.h
        src/TARGA_loader.cxx                    src/TARGA_loader.h
        src/main.cxx                            src/main.h
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME})

set_target_properties(VulkanIsland PROPERTIES
        VERSION ${PROJECT_VERSION}

        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF

        POSITION_INDEPENDENT_CODE YES

        DEBUG_POSTFIX _d
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(VulkanIsland PRIVATE
            -pedantic
            -Wall
            -Wextra
            #-fsanitize=thread -fsanitize=address
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(VulkanIsland PRIVATE
            /Wall
)

endif()

find_package(Boost REQUIRED COMPONENTS
        filesystem
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

        vulkan

        glm
        glfw3
)