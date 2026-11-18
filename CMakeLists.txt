cmake_minimum_required(VERSION 3.25)
project(AC-save VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

add_library(AC-saveLib ${HEADERS})
#target_include_directories(AC-saveLib INTERFACE include)
set_target_properties(AC-saveLib PROPERTIES LINKER_LANGUAGE CXX)

add_executable(AC-saveDemo ${SOURCES} ${HEADERS})

target_include_directories(AC-saveLib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(AC-saveDemo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)