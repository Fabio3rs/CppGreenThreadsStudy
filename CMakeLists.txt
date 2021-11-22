set(myprojectname GreenThreadsTests)

cmake_minimum_required(VERSION 3.6.0)
project(GreenThreadsTests VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.;-extra-arg=-std=c++17;-export-fixes;-checks=read*,modern*,hicpp*,performance*")

include(CPack)

include(CTest)
enable_testing()

file(
    GLOB SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${myprojectname} ${SOURCES})

target_include_directories(${myprojectname} PRIVATE ${INCLUDE_DIRS})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    message(STATUS "Setting G++ flags")
    target_compile_options(${myprojectname} PRIVATE -Wall -Wextra $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -Wformat-security -Wconversion -Wsign-conversion)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    message(STATUS "Setting Clang flags")
    target_compile_options(${myprojectname} PRIVATE -Weverything -Wno-global-constructors $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>  -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-exit-time-destructors)
endif()

target_link_libraries(${myprojectname} PRIVATE pthread)

