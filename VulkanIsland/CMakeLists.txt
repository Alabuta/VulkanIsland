cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(VulkanIsland
	VERSION ${PROJECT_VERSION}
	LANGUAGES CXX
)

include(build/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_SOURCE_DIR}/include/config.h"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/../bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/../bin")

find_package(Vulkan REQUIRED)

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
    
    src/renderer/pipelines.cxx
    src/renderer/renderPass.cxx
    src/renderer/vertexLayout.cxx

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


set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}

    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED NO
    CXX_EXTENSIONS OFF

    POSITION_INDEPENDENT_CODE YES

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
        /Wall
)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
	Vulkan::Vulkan
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,-no-undefined
    -Wl,-no-allow-shlib-undefined
    -Wl,-unresolved-symbols=report-all

    #pthread

    stdc++fs

    Vulkan::Vulkan

	CONAN_PKG::glm
	CONAN_PKG::boost
	CONAN_PKG::glfw
)
