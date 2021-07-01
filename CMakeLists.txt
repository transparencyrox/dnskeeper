cmake_minimum_required (VERSION 3.11 FATAL_ERROR)

Include(FetchContent)

list (APPEND CMAKE_MODULE_PATH 
     "${CMAKE_CURRENT_SOURCE_DIR}/.cmake")

# Meta information about the project TeamWork
set (META_NAME                "dnskeeper")
set (META_DESCRIPTION         "A tool to keep track of DNS entries")
set (META_AUTHOR_ORGANIZATION "Harsh@Pyrotechnics")
set (META_AUTHOR_DOMAIN       "http://www.pyrotechnics.io")
set (META_AUTHOR_MAINTAINER   "harsh@pyrotechnics.io")
set (META_VERSION_MAJOR       "0")
set (META_VERSION_MINOR       "1")
set (META_VERSION_PATCH       "0")

# Build flags
project (${META_NAME} C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-fpermissive -Wall -Wextra -pedantic -Werror")

# Dependent system libraries
find_package (Threads REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)       # Debian: libssl-dev

# Http-Lib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.8.9
    SOURCE_SUBDIR  cmake)
FetchContent_MakeAvailable(httplib)

# AIXLog
FetchContent_Declare(
    aixlog
    GIT_REPOSITORY https://github.com/badaix/aixlog.git
    GIT_TAG        v1.5.0
    CMAKE_ARGS     -DBUILD_EXAMPLE=OFF)
FetchContent_MakeAvailable(aixlog)

# FmtLib
FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        7.1.3 
    CMAKE_ARGS     -DFMT_TEST=OFF)
FetchContent_MakeAvailable(fmtlib)

# PostgreSQL
if (NOT DEFINED PQXX_SDK)
    set(PQXX_SDK "/tmp/build/pqxx")
endif()
set(PostgreSQL_TYPE_INCLUDE_DIR /usr/include/postgresql)
set(libpqxx_DIR "${PQXX_SDK}/lib/cmake/libpqxx")
find_package(libpqxx)

# AWS CPP SDK
if (NOT DEFINED AWS_SDK)
    set(AWS_SDK "/tmp/build/aws_sdk")
endif()
if (EXISTS AWS_SDK)
    message(FATAL_ERROR "AWS_SDK directory does not exist")
else()
    message(STATUS "Using AWS SDK: ${AWS_SDK}")
    set(AWS_INCLUDES "${AWS_SDK}/include")
    set(AWS_LINKAGE "-laws-cpp-sdk-core -laws-cpp-sdk-route53")
endif()

set(CMAKE_INSTALL_RPATH "${AWS_SDK}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
include_directories(${AWS_SDK}/include 
                    ${PQXX_SDK}/include 
                    ${aixlog_SOURCE_DIR}/include 
                    ${PROJECT_BINARY_DIR}/include 
                    ${CMAKE_SOURCE_DIR}/include)
link_directories(${AWS_SDK}/lib ${PQXX_SDK}/lib)

find_program(DATE date)
execute_process(
    COMMAND ${DATE} +%Y%m%d.%H%M
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE BUILD_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set (DNSKEEPER_VERSION 
    "${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH} (${BUILD_DATE})")
if(NOT DEFINED DNSKEEPER_DEFAULT_PORT)
    set (DNSKEEPER_DEFAULT_PORT 9500)
endif()

if(NOT DEFINED DNSKEEPER_DEFAULT_BUFFER_SIZE)
    set (DNSKEEPER_DEFAULT_BUFFER_SIZE 9256)
endif()

configure_file (
    "${PROJECT_SOURCE_DIR}/include/dnskeeper.h.in"
    "${PROJECT_BINARY_DIR}/include/dnskeeper.h"
)

add_subdirectory (src)
add_subdirectory (lib)
add_subdirectory (etc)

# Integrate Catch2
enable_testing()
add_subdirectory (tests)

set(CPACK_PACKAGE_NAME META_NAME)
set(CPACK_PACKAGE_VENDOR META_AUTHOR_ORGANIZATION)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY META_DESCRIPTION)
set(CPACK_PACKAGE_VERSION_MAJOR META_VERSION_MAJOR)
set(CPACK_PACKAGE_VERSION_MINOR META_VERSION_MINOR)
set(CPACK_PACKAGE_VERSION_PATCH META_VERSION_PATCH)

include(CPack)
