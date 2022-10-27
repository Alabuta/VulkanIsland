cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(VulkanIsland VERSION 1.0.0 LANGUAGES CXX)

configure_file(
	"${PROJECT_SOURCE_DIR}/engine/include/config.hxx.in"
	"${PROJECT_SOURCE_DIR}/engine/include/config.hxx"
)

set(EXECUTABLE_TARGET_NAME engine)
add_executable(${EXECUTABLE_TARGET_NAME})

set(CMAKE_LINK_LIBRARY_USING_load_archive TRUE)
set(CMAKE_C_LINK_LIBRARY_USING_load_archive_SUPPORTED TRUE)
set(CMAKE_CXX_LINK_LIBRARY_USING_load_archive_SUPPORTED TRUE)

set(GLFW_LIBRARY_TYPE "SHARED" CACHE STRING "" FORCE)

include("cmake/InitSanitizers.cmake")
include("cmake/SetCompilerFlagsStyle.cmake")

find_package(Vulkan REQUIRED)
find_package(Boost 1.68 REQUIRED)

if(NOT WIN32)
	find_package(X11 REQUIRED)
endif()

set(SUBMODULES
	fmt
	glfw
	glm
	nlohmann_json
	range-v3
	volk
)

foreach(SUBMODULE ${SUBMODULES})
	message(STATUS "Including submodule ${SUBMODULE}")
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/${SUBMODULE} EXCLUDE_FROM_ALL SYSTEM)
endforeach()

target_sources(${EXECUTABLE_TARGET_NAME}
    PRIVATE
		./engine/src
		./engine/src/camera/handlers/mouse_handler.hxx			./engine/src/camera/handlers/mouse_handler.cxx
		./engine/src/camera/camera_controller.hxx				./engine/src/camera/camera_controller.cxx
		./engine/src/camera/camera.hxx							./engine/src/camera/camera.cxx

		./engine/src/graphics/attachments.hxx					./engine/src/graphics/attachments.cxx
		./engine/src/graphics/compatibility.hxx
		./engine/src/graphics/descriptors.hxx					./engine/src/graphics/descriptors.cxx
		./engine/src/graphics/graphics_api.hxx					./engine/src/graphics/graphics_api.cxx
		./engine/src/graphics/graphics_pipeline.hxx				./engine/src/graphics/graphics_pipeline.cxx
		./engine/src/graphics/graphics.hxx						./engine/src/graphics/graphics.cxx
		./engine/src/graphics/pipeline_states.hxx				./engine/src/graphics/pipeline_states.cxx
		./engine/src/graphics/render_pass.hxx 					./engine/src/graphics/render_pass.cxx
		./engine/src/graphics/shader_program.hxx 				./engine/src/graphics/shader_program.cxx
		./engine/src/graphics/vertex.hxx						./engine/src/graphics/vertex.cxx

		./engine/src/loaders/image_loader.hxx 					./engine/src/loaders/image_loader.cxx
		./engine/src/loaders/loaderGLTF.hxx 					./engine/src/loaders/loaderGLTF.cxx
		./engine/src/loaders/material_loader.hxx 				./engine/src/loaders/material_loader.cxx
		./engine/src/loaders/scene_loader.hxx
		./engine/src/loaders/SPIRV_loader.hxx 					./engine/src/loaders/SPIRV_loader.cxx
		./engine/src/loaders/TARGA_loader.hxx 					./engine/src/loaders/TARGA_loader.cxx

		./engine/src/math/math.hxx 								./engine/src/math/math.cxx
		./engine/src/math/pack-unpack.hxx

		./engine/src/platform/input/input_data.hxx
		./engine/src/platform/input/input_manager.hxx 			./engine/src/platform/input/input_manager.cxx
		./engine/src/platform/input/mouse.hxx 					./engine/src/platform/input/mouse.cxx
		./engine/src/platform/window.hxx 						./engine/src/platform/window.cxx

		./engine/src/primitives/box.cxx
		./engine/src/primitives/icosahedron.cxx
		./engine/src/primitives/plane.cxx
		./engine/src/primitives/sphere.cxx
		./engine/src/primitives/teapot.cxx
		./engine/src/primitives/primitives.hxx

		./engine/src/renderer/command_buffer.hxx 				./engine/src/renderer/command_buffer.cxx
		./engine/src/renderer/config.hxx 						./engine/src/renderer/config.cxx
		./engine/src/renderer/material.hxx 						./engine/src/renderer/material.cxx
		./engine/src/renderer/queues.hxx
		./engine/src/renderer/render_flow.hxx 					./engine/src/renderer/render_flow.cxx
		./engine/src/renderer/swapchain.hxx 					./engine/src/renderer/swapchain.cxx
		./engine/src/renderer/renderer.hxx 						./engine/src/renderer/renderer.cxx
		./engine/src/renderer/swapchain.hxx 					./engine/src/renderer/swapchain.cxx

		./engine/src/resources/buffer.hxx 						./engine/src/resources/buffer.cxx
		./engine/src/resources/framebuffer.hxx 					./engine/src/resources/framebuffer.cxx
		./engine/src/resources/image.hxx 						./engine/src/resources/image.cxx
		./engine/src/resources/memory_manager.hxx 				./engine/src/resources/memory_manager.cxx
		./engine/src/resources/resource_manager.hxx 			./engine/src/resources/resource_manager.cxx
		./engine/src/resources/sync_objects.hxx

		./engine/src/utility/exceptions.hxx
		./engine/src/utility/mpl.hxx
		./engine/src/utility/helpers.hxx

		./engine/src/vulkan/debug.hxx 							./engine/src/vulkan/debug.cxx
		./engine/src/vulkan/device.hxx 							./engine/src/vulkan/device.cxx
		./engine/src/vulkan/device_config.hxx
		./engine/src/vulkan/device_limits.hxx
		./engine/src/vulkan/instance.hxx 						./engine/src/vulkan/instance.cxx

		./engine/src/descriptor.hxx 							./engine/src/descriptor.cxx

		./engine/src/app.hxx 									./engine/src/app.cxx
		./engine/src/main.hxx 									./engine/src/main.cxx
)

target_include_directories(${EXECUTABLE_TARGET_NAME}
	SYSTEM PRIVATE
		${Boost_INCLUDE_DIR}
		${CMAKE_CURRENT_LIST_DIR}/external
		./engine/include

	PRIVATE
		./engine/src
)

set_target_properties (${EXECUTABLE_TARGET_NAME}
	PROPERTIES
		VERSION ${PROJECT_VERSION}

		CXX_STANDARD 23
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF

		POSITION_INDEPENDENT_CODE ON

		DEBUG_POSTFIX .d
)

target_compile_options(${EXECUTABLE_TARGET_NAME}
	PRIVATE
		${EXTRA_COMPILER_OPTIONS}

		"$<$<OR:$<BOOL:${CXX_FLAGS_STYLE_GNU}>,$<BOOL:${CXX_FLAGS_STYLE_CLANGCL}>>:"
			-Wpedantic
			-Wall
			-Wextra
			-Werror
			-Wconversion

			-Wold-style-cast
			-Wnon-virtual-dtor
			-Wcast-align
			-Wunused
			-Woverloaded-virtual
			-Wconversion
			-Wsign-conversion
			-Wnull-dereference
			-Wdouble-promotion
			-Wformat=2
			-Wduplicated-cond
			-Wduplicated-branches
			-Wlogical-op
			-Wuseless-cast
			-Wmisleading-indentation

			-Wno-c++98-compat
			-Wno-c++98-compat-pedantic
			-Wno-pre-c++17-compat

			-Wno-switch-enum
			-Wno-covered-switch-default
		">"

		"$<$<BOOL:${CXX_FLAGS_STYLE_GNU}>:"
			-fconcepts

			-fasynchronous-unwind-tables                # Increased reliability of backtraces
			-fexceptions                                # Enable table-based thread cancellation
			# -fPIE
			# -fpie

			-pipe
		">"

		"$<$<BOOL:${CXX_FLAGS_STYLE_CLANGCL}>:"
			/EHa
			# -Xclang -fexceptions -Xclang -fcxx-exceptions
			SHELL:-Xclang -fexceptions

			-Wno-unknown-pragmas
			-Wno-unknown-warning-option

			-Wno-shadow-field-in-constructor
		">"

		"$<$<BOOL:${CXX_FLAGS_STYLE_MSVC}>:"
			"/std:c++latest"

			# /W4
			# /WX
			/w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
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
		">"
)

target_link_directories(${EXECUTABLE_TARGET_NAME}
	PRIVATE
		${EXTRA_LINK_DIRS}
)

target_link_libraries(${EXECUTABLE_TARGET_NAME}
	PRIVATE
		"$<$<IN_LIST:${CMAKE_SYSTEM_NAME}," Linux";"Darwin">:"
			stdc++fs
			# pthread
		">"

		Vulkan::Vulkan
		${Boost_LIBRARIES}

		fmt::fmt-header-only
		glfw
		glm::glm
		nlohmann_json::nlohmann_json
		range-v3 range-v3-meta range-v3::meta range-v3-concepts
		volk::volk_headers
)

target_link_options(${EXECUTABLE_TARGET_NAME}
	PRIVATE
		${EXTRA_LINK_OPTIONS}

		"$<$<IN_LIST:${CMAKE_SYSTEM_NAME}," Linux";"Darwin">:"
			# LINKER:SHELL:-z defs
			LINKER:-z,defs;                         # Detect and reject underlinking
			LINKER:-z,now;                          # Disable lazy binding
			LINKER:-z,relro                         # Read-only segments after relocation
		">"

		"$<$<OR:$<BOOL:${CXX_FLAGS_STYLE_GNU}>,$<BOOL:${CXX_FLAGS_STYLE_CLANGCL}>>:"
			LINKER:-pie;
			LINKER:-no-undefined;                   # Report unresolved symbol references from regular object files
			LINKER:-no-allow-shlib-undefined;       # Disallows undefined symbols in shared libraries
			LINKER:-unresolved-symbols=report-all
		">"
)
