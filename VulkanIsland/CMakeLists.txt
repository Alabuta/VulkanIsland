cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
include(ExternalProject)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(VulkanIsland VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_executable(${PROJECT_NAME})

configure_file(
    "${PROJECT_SOURCE_DIR}/config.hxx.in"
    "${PROJECT_SOURCE_DIR}/include/config.hxx"
)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/camera/handlers/mouse_handler.hxx           src/camera/handlers/mouse_handler.cxx
        src/camera/camera_controller.hxx                src/camera/camera_controller.cxx
        src/camera/camera.hxx                           src/camera/camera.cxx

        src/ecs/ecs.hxx
        src/ecs/mesh.hxx
        src/ecs/node.hxx                                src/ecs/node.cxx
        src/ecs/transform.hxx                           src/ecs/transform.cxx

        src/graphics/attachments.hxx                    src/graphics/attachments.cxx
        src/graphics/compatibility.hxx
        src/graphics/descriptors.hxx		            src/graphics/descriptors.cxx
        src/graphics/graphics_api.hxx                   src/graphics/graphics_api.cxx
        src/graphics/graphics_pipeline.hxx              src/graphics/graphics_pipeline.cxx
        src/graphics/graphics.hxx
        src/graphics/pipeline_states.hxx                src/graphics/pipeline_states.cxx
        src/graphics/render_pass.hxx                    src/graphics/render_pass.cxx
        src/graphics/shader_program.hxx                 src/graphics/shader_program.cxx
        src/graphics/vertex.hxx                         src/graphics/vertex.cxx

	    src/loaders/loaderGLTF.hxx                      src/loaders/loaderGLTF.cxx
	    src/loaders/material_loader.hxx                 src/loaders/material_loader.cxx
	    src/loaders/scene_loader.hxx
	    src/loaders/SPIRV_loader.hxx                    src/loaders/SPIRV_loader.cxx
        src/loaders/TARGA_loader.hxx                    src/loaders/TARGA_loader.cxx

        src/math/math.hxx                               src/math/math.cxx

        src/platform/input/input_data.hxx
        src/platform/input/input_manager.hxx            src/platform/input/input_manager.cxx
        src/platform/input/mouse.hxx                    src/platform/input/mouse.cxx

        src/platform/window.hxx                         src/platform/window.cxx
	
	    src/renderer/command_buffer.hxx                 src/renderer/command_buffer.cxx
        src/renderer/config.hxx                         src/renderer/config.cxx
        src/renderer/material.hxx                       src/renderer/material.cxx
        src/renderer/queues.hxx
        src/renderer/render_flow.hxx                    src/renderer/render_flow.cxx
	    src/renderer/swapchain.hxx                      src/renderer/swapchain.cxx

	    src/resources/buffer.hxx                        src/resources/buffer.cxx
        src/resources/framebuffer.hxx                   src/resources/framebuffer.cxx
	    src/resources/image.hxx                         src/resources/image.cxx
        src/resources/memory_manager.hxx                src/resources/memory_manager.cxx
        src/resources/resource_manager.hxx              src/resources/resource_manager.cxx
        src/resources/semaphore.hxx

	    src/utility/mpl.hxx
	    src/utility/helpers.hxx

	    src/vulkan/debug.hxx                            src/vulkan/debug.cxx
        src/vulkan/device_limits.hxx
	    src/vulkan/device.hxx                           src/vulkan/device.cxx
	    src/vulkan/instance.hxx                         src/vulkan/instance.cxx

        src/descriptor.hxx                              src/descriptor.cxx
        src/main.hxx                                    src/main.cxx
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(Vulkan REQUIRED)
#find_package(fmt 6.0 CONFIG REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../external)
find_package(fmt 6.0 CONFIG REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm 0.9 REQUIRED)
find_package(nlohmann_json 3.7 CONFIG REQUIRED)
find_package(Boost 1.68 REQUIRED)

if (NOT WIN32)
	# find_package(X11 REQUIRED)
endif ()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${GLFW_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include

    PRIVATE
        # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            _GLIBCXX_USE_CXX11_ABI=1
    )
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}

    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED NO
    CXX_EXTENSIONS OFF

    POSITION_INDEPENDENT_CODE ON

    DEBUG_POSTFIX .d
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -fconcepts

            -fasynchronous-unwind-tables                # Increased reliability of backtraces
            -fexceptions                                # Enable table-based thread cancellation
            -fpie

            -pipe

            -Wpedantic
            -Wall
            -Wextra
		    -Wold-style-cast
		    -Wnon-virtual-dtor
		    -Wcast-align
		    -Wunused
		    -Wconversion
		    -Wsign-conversion
		    -Wnull-dereference
		    -Wdouble-promotion
		    -Wformat=2
		    -Wduplicated-cond
		    -Wduplicated-branches
		    -Wlogical-op
		    -Wuseless-cast
            # -Wlifetime

            # -fsanitize=thread -fsanitize=address
    )

    set(EXTRA_LIBS ${EXTRA_LIBS}
        stdc++fs
        # pthread
    )

    target_link_options(${PROJECT_NAME}
        PRIVATE
            -pie
            -Wl, -z,defs                             # Detect and reject underlinking
            -Wl, -z,now                              # Disable lazy binding
            -Wl, -z,relro                            # Read-only segments after relocation
            # -Wl, -no-undefined                       # Report unresolved symbol references from regular object files
            # -Wl, -no-allow-shlib-undefined           # Disallows undefined symbols in shared libraries
            -Wl, -unresolved-symbols=report-all
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -Wpedantic
            -Wall
            -Wextra
		    -Wold-style-cast
		    -Wnon-virtual-dtor
		    -Wcast-align
		    -Wunused
		    -Wconversion
		    -Wsign-conversion
		    -Wnull-dereference
		    -Wdouble-promotion
		    -Wformat=2

            #-fsanitize=thread -fsanitize=address
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            /W3
    )
endif ()


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${EXTRA_LIBS}

        Vulkan::Vulkan
        
        #fmt::fmt-header-only
	    #fmt::fmt
        glfw ${GLFW_LIBRARY}
        #glm::glm
	    #nlohmann_json::nlohmann_json
)
