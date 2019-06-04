cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(VulkanIsland VERSION ${PROJECT_VERSION})

enable_language(CXX)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.hxx.in"
    "${PROJECT_SOURCE_DIR}/include/config.hxx"
)

find_package(Vulkan REQUIRED)

find_package(glm REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(nlohmann_json 3.6 CONFIG REQUIRED)
find_package(EnTT 3.0 CONFIG REQUIRED)
find_package(Boost 1.69 REQUIRED)

if(NOT WIN32)
	find_package(X11 REQUIRED)
endif()


include_directories(SYSTEM include)
include_directories(src)


set(SOURCE_FILES
	src/resources/memory.cxx
	src/resources/buffer.cxx
	src/resources/image.cxx
    src/resources/resource.cxx
    src/resources/program.cxx
	
	src/debug.cxx
	src/descriptor.cxx
	src/device.cxx
	
	src/loaders/loaderGLTF.cxx
	src/loaders/loaderTARGA.cxx
	
	src/instance.cxx
    
    src/renderer/pipelineVertexInputState.cxx
    src/renderer/material.cxx
    src/renderer/pipelines.cxx
    src/renderer/renderPass.cxx

	src/sceneTree.cxx
	src/swapchain.cxx

    src/camera/cameraController.cxx
    src/camera/handlers/mouseHandler.cxx
	
	src/input/inputManager.cxx
	src/input/mouseInput.cxx

    src/platform/window.cxx

    src/ecs/node.cxx
    src/ecs/transform.cxx
    src/ecs/camera.cxx

    src/main.cxx
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(NOT WIN32)
    set(EXTRA_LIBS ${EXTRA_LIBS}
        stdc++fs
        # pthread
    )

    set(EXTRA_LIBS_LINK_OPTIONS ${EXTRA_LIBS}
        -Wl,-no-undefined
        -Wl,-no-allow-shlib-undefined
        -Wl,-unresolved-symbols=report-all
    )
endif()


set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}

    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED NO
    CXX_EXTENSIONS OFF

    POSITION_INDEPENDENT_CODE ON

    DEBUG_POSTFIX .d
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE
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

        #-fsanitize=thread -fsanitize=address
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
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

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3
)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${EXTRA_LIBS_LINK_OPTIONS}

    ${EXTRA_LIBS}

    Vulkan::Vulkan

    glm
    glfw
	nlohmann_json::nlohmann_json
	EnTT
)
