cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
include(ExternalProject)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(VulkanIsland VERSION ${PROJECT_VERSION} LANGUAGES CXX)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.hxx.in"
    "${PROJECT_SOURCE_DIR}/include/config.hxx"
)

set(EXECUTABLE_TARGET_NAME engine)
add_executable(${EXECUTABLE_TARGET_NAME})

target_sources(${EXECUTABLE_TARGET_NAME}
    PRIVATE
        src/camera/handlers/mouse_handler.hxx           src/camera/handlers/mouse_handler.cxx
        src/camera/camera_controller.hxx                src/camera/camera_controller.cxx
        src/camera/camera.hxx                           src/camera/camera.cxx

        src/graphics/attachments.hxx                    src/graphics/attachments.cxx
        src/graphics/compatibility.hxx
        src/graphics/descriptors.hxx		            src/graphics/descriptors.cxx
        src/graphics/graphics_api.hxx                   src/graphics/graphics_api.cxx
        src/graphics/graphics_pipeline.hxx              src/graphics/graphics_pipeline.cxx
        src/graphics/graphics.hxx                       src/graphics/graphics.cxx
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

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Vulkan REQUIRED)
#find_package(fmt 6.0 CONFIG REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../external)
find_package(fmt 6.0 CONFIG REQUIRED)
find_package(glfw3 3.3 CONFIG REQUIRED)
find_package(glm 0.9 CONFIG REQUIRED)
find_package(nlohmann_json 3.7 CONFIG REQUIRED)
find_package(Boost 1.68 REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

if (NOT WIN32)
	# find_package(X11 REQUIRED)
endif ()

# target_include_directories(${EXECUTABLE_TARGET_NAME}
#     PUBLIC
#         ${GLFW_INCLUDE_DIR}
#         "${CMAKE_CURRENT_SOURCE_DIR}/include"

#     PRIVATE
#         # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
#         "${CMAKE_CURRENT_SOURCE_DIR}/src"
# )
# TODO:: replace by target_include_directories
include_directories(SYSTEM include)
include_directories(src)

if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_definitions(${EXECUTABLE_TARGET_NAME}
        PRIVATE
            _GLIBCXX_USE_CXX11_ABI=1
    )
endif ()

target_compile_features(${EXECUTABLE_TARGET_NAME}
    PUBLIC
        cxx_std_20
)

set_target_properties(${EXECUTABLE_TARGET_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}

    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED NO
    CXX_EXTENSIONS OFF

    POSITION_INDEPENDENT_CODE ON

    DEBUG_POSTFIX .d
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${EXECUTABLE_TARGET_NAME}
        PRIVATE
            -fconcepts

            -fasynchronous-unwind-tables                # Increased reliability of backtraces
            -fexceptions                                # Enable table-based thread cancellation
            # -fPIE
            # -fpie

            -pipe

            -Wpedantic
            -Wall
            -Wextra
            -Werror
		    -Wold-style-cast
		    -Wnon-virtual-dtor
		    -Wcast-align
		    -Wunused
            -Woverloaded-virtual
		    -Wconversion
		    -Wsign-conversion
            -Wmisleading-indentation
		    -Wnull-dereference
		    -Wdouble-promotion
		    -Wformat=2
		    -Wduplicated-cond
		    -Wduplicated-branches
		    -Wlogical-op
            -Wuseless-cast
            
            -Wno-unknown-pragmas

            # -fsanitize=thread -fsanitize=address
    )

    set(EXTRA_LIBS ${EXTRA_LIBS}
        stdc++fs
        # pthread
    )

    target_link_options(${EXECUTABLE_TARGET_NAME}
        PRIVATE
            # LINKER: -pie
            # LINKER: -z,defs                             # Detect and reject underlinking
            # LINKER: -z,now                              # Disable lazy binding
            # LINKER: -z,relro                            # Read-only segments after relocation
            # LINKER: -no-undefined                       # Report unresolved symbol references from regular object files
            # LINKER: -no-allow-shlib-undefined           # Disallows undefined symbols in shared libraries
            LINKER: -unresolved-symbols=report-all
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${EXECUTABLE_TARGET_NAME}
        PRIVATE
            -Wpedantic
            -Wall
            -Wextra
            -Werror
		    -Wold-style-cast
		    -Wnon-virtual-dtor
		    -Wcast-align
		    -Wunused
		    -Wconversion
            -Woverloaded-virtual
		    -Wsign-conversion
		    -Wnull-dereference
		    -Wdouble-promotion
		    -Wformat=2
            # -Wlifetime

            -Wno-unknown-pragmas
            -Wno-unknown-warning-option

            #-fsanitize=thread -fsanitize=address
    )
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${EXECUTABLE_TARGET_NAME}
        PRIVATE
            /permissive

            /W4
            /WX
            /w14242 # 'identfier': conversion from 'type1' to 'type1', possible loss of data
            /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
            /w14263 # 'function': member function does not override any base class virtual member function
            /w14265 # 'classname': class has virtual functions, but destructor is not virtual
            /w14287 # 'operator': unsigned/negative constant mismatch
            /we4289 # 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
            /w14296 # 'operator': expression is always 'boolean_value'
            /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
            /w14545 # expression before comma evaluates to a function which is missing an argument list
            /w14546 # function call before comma missing argument list
            /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
            /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
            /w14555 # expression has no effect; expected expression with side-effect
            /w14619 # pragma warning: there is no warning number 'number'
            /w14640 # Enable warning on thread un-safe static member initialization
            /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
            /w14905 # wide string literal cast to 'LPSTR'
            /w14906 # string literal cast to 'LPWSTR'
            /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
    )
endif ()


target_link_libraries(${EXECUTABLE_TARGET_NAME}
    PRIVATE
        ${EXTRA_LIBS}

        Vulkan::Vulkan
        
        fmt::fmt-header-only
	    fmt::fmt
        #glfw ${GLFW_LIBRARY}
        glfw
        glm#::glm
	    nlohmann_json::nlohmann_json

        range-v3 range-v3-meta range-v3::meta range-v3-concepts
)
