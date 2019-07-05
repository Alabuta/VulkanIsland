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
    src/camera/handlers/mouseHandler.hxx            src/camera/handlers/mouseHandler.cxx
    src/camera/cameraController.hxx                 src/camera/cameraController.cxx
    src/camera/camera.hxx                           src/camera/camera.cxx

	src/device/device.hxx                           src/device/device.cxx
    src/device/deviceConfig.hxx

    # src/ecs/camera.hxx                              src/ecs/camera.cxx
    src/ecs/ecs.hxx
    src/ecs/mesh.hxx
    src/ecs/node.hxx                                src/ecs/node.cxx
    src/ecs/transform.hxx                           src/ecs/transform.cxx

    src/input/inputDataTypes.hxx
    src/input/inputManager.hxx                      src/input/inputManager.cxx
    src/input/mouseInput.hxx                    	src/input/mouseInput.cxx

	src/loaders/loaderGLTF.hxx                      src/loaders/loaderGLTF.cxx
	src/loaders/loaderSPIRV.hxx
	src/loaders/loaderTARGA.hxx                     src/loaders/loaderTARGA.cxx

    src/platform/window.hxx                         src/platform/window.cxx

    src/renderer/attachments.hxx
    src/renderer/material.hxx                       src/renderer/material.cxx
    src/renderer/pipelines.hxx                      src/renderer/pipelines.cxx
    src/renderer/pipelineVertexInputState.hxx       src/renderer/pipelineVertexInputState.cxx
    src/renderer/renderPass.hxx                     src/renderer/renderPass.cxx

	src/resources/buffer.hxx                        src/resources/buffer.cxx
	src/resources/image.hxx                         src/resources/image.cxx
    src/resources/memory.hxx                        src/resources/memory.cxx
    src/resources/program.hxx                       src/resources/program.cxx
    src/resources/resource.hxx                      src/resources/resource.cxx
	
	src/commandBuffer.hxx
	src/debug.hxx                                   src/debug.cxx
    src/descriptor.hxx                              src/descriptor.cxx
	src/helpers.hxx
	src/instance.hxx                                src/instance.cxx
    src/main.hxx                                    src/main.cxx
    src/math.hxx                                    src/math.cxx
    src/queueBuilder.hxx
    src/queues.hxx
    src/sceneTree.hxx                               src/sceneTree.cxx
    src/semaphore.hxx
    src/staging.hxx
	src/swapchain.hxx                               src/swapchain.cxx
    src/vertexFormat.hxx
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _GLIBCXX_USE_CXX11_ABI=1)
endif ()

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
