cmake_minimum_required(VERSION 3.25)
project(AC-save VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED LANGUAGES CXX)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION "$(IPO_SUPPORTED)")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.inl"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

add_library(AC-saveLib INTERFACE)

add_executable(AC-saveDemo ${SOURCES} ${HEADERS})

target_include_directories(AC-saveLib INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(AC-saveDemo PRIVATE AC-saveLib)
if(WIN32)
    target_link_libraries(AC-saveDemo PRIVATE psapi)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_executable(AC-saveTests tests/archive_tests.cpp)
    target_include_directories(AC-saveTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(AC-saveTests PRIVATE AC-saveLib)
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(MINGW_RUNTIME_DLLS "")
        foreach(runtime_dll IN ITEMS libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
            set(runtime_dll_path "${MINGW_BIN_DIR}/${runtime_dll}")
            if(EXISTS "${runtime_dll_path}")
                list(APPEND MINGW_RUNTIME_DLLS "${runtime_dll_path}")
            endif()
        endforeach()
        add_custom_command(TARGET AC-saveTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E
                copy_if_different
                ${MINGW_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:AC-saveTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
    add_test(NAME archive_tests COMMAND AC-saveTests)
endif()
