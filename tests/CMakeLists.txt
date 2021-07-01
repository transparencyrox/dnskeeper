# Catch2 integration

Include(FetchContent)
set(CTEST_OUTPUT_ON_FAILURE ON)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.1)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)

add_library(test_main test.cpp)
target_include_directories(test_main
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_main
    PUBLIC 
        Catch2::Catch2)

# --[ Tests ]--
include(CTest)
include(Catch)

set(test_targets)
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.t.cpp)
foreach (testsrc ${TEST_SOURCES})
    get_filename_component(testname ${testsrc} NAME_WLE)
    get_filename_component(libname ${testname} NAME_WLE)
    message("-> " ${testname} " : " ${libname})
    add_executable(${testname} ${testsrc})
    target_link_libraries(${testname} PUBLIC test_main ${libname})
    list(APPEND test_targets "${testname}")
    catch_discover_tests(${testname})
endforeach (testsrc ${TEST_SOURCES})

