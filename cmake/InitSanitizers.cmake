## Wait for LINK_LIBRARY:load_archive feature support
if(${USE_ASAN})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g3")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -g3")
    set(EXTRA_COMPILER_OPTIONS ${EXTRA_COMPILER_OPTIONS} -fsanitize=address -g3)

    if(CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
        set(CMAKE_C_LINK_LIBRARY_USING_load_archive "-force_load clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_C_LINK_LIBRARY_USING_load_archive
            "LINKER:--push-state,--whole-archive"
            "clang_rt.asan_dynamic_runtime_thunk-x86_64.lib"
            "LINKER:--pop-state"
        )
    elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_C_LINK_LIBRARY_USING_load_archive "/WHOLEARCHIVE:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
    else()
        # feature not yet supported for the other environments
        set(CMAKE_C_LINK_LIBRARY_USING_load_archive_SUPPORTED FALSE)
    endif()

    # The -force_load Apple linker option requires a file name
    set(asan_dynamic_runtime_thunk_lib
        "$<IF:$<LINK_LANG_AND_ID:C,AppleClang>,clang_rt.asan_dynamic_runtime_thunk-x86_64.a,clang_rt.asan_dynamic_runtime_thunk-x86_64>"
    )

    set(EXTRA_LIBS ${EXTRA_LIBS}
        #[[clang_rt.asan_dynamic-x86_64
        "$<LINK_LIBRARY:load_archive,${asan_dynamic_runtime_thunk_lib}>"]]
    )

    set(EXTRA_LINK_OPTIONS ${EXTRA_LINK_OPTIONS}
        /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib
    )

    set(EXTRA_LINK_DIRS ${EXTRA_LINK_DIRS}
        "C:/Program Files/LLVM/lib/clang/15.0.2/lib/windows"    # :TODO:
    )
endif()
