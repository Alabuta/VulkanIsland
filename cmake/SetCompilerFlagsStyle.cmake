# Get compiler info
set(CXX_FLAGS_STYLE_GNU OFF)
set(CXX_FLAGS_STYLE_MSVC OFF)
set(CXX_FLAGS_STYLE_CLANGCL OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CXX_FLAGS_STYLE_GNU ON)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        set(CXX_FLAGS_STYLE_CLANGCL ON)
    else()
        set(CXX_FLAGS_STYLE_GNU ON)
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CXX_FLAGS_STYLE_MSVC ON)
else()
    message(FATAL_ERROR "Unsupported compiler!")
endif()