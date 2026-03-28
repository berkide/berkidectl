cmake_minimum_required(VERSION 3.16)
project(berkidectl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(berkidectl src/main.cpp)

target_include_directories(berkidectl PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
)

# Platform-conditional libraries
# Platforma ozel kutuphaneler
if(WIN32)
    target_link_libraries(berkidectl PRIVATE ws2_32)
elseif(APPLE)
    target_link_libraries(berkidectl PRIVATE "-framework CoreFoundation")
endif()

# TLS/SSL support (optional)
# TLS/SSL destegi (istege bagli)
option(BERKIDECTL_USE_TLS "Enable TLS/SSL support" OFF)
if(BERKIDECTL_USE_TLS)
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(berkidectl PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(berkidectl PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "berkidectl: TLS enabled")
endif()
