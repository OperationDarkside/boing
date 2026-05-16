cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_COMPILER "$ENV{HOME}/gcc-trunk/bin/gcc")
set(CMAKE_CXX_COMPILER "$ENV{HOME}/gcc-trunk/bin/g++")

project(boing VERSION 0.1.0 LANGUAGES CXX)

# Target C++20 for Coroutines support
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-std=c++26 -freflection -Wall -Wextra)

set(CMAKE_BUILD_TYPE Debug)

# Find Boost components
# Asio coroutines often require Boost.Context if using stackful, 
# but for C++20 awaitables, we just need System and Thread.
find_package(Boost REQUIRED COMPONENTS system thread url)

add_library(CppJsonMagic INTERFACE)
target_include_directories(CppJsonMagic INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/CppJsonMagic
)

add_executable(boing main.cpp
                    annotations.cpp
                    context.cpp
                    router.cpp
                    session_manager.cpp
                    session.cpp
                    webserver.cpp
                    simple_session.cpp
                    )
target_link_options(boing PRIVATE -static-libstdc++ -static-libgcc)

target_link_libraries(boing 
    PRIVATE 
    Boost::system 
    Boost::thread
    Boost::url
    Threads::Threads
    CppJsonMagic
)

# Useful compiler warnings
if(MSVC)
    target_compile_options(boing PRIVATE /W4)
else()
    target_compile_options(boing PRIVATE -Wall -Wextra -Wpedantic)
endif()