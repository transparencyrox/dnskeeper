# Executable targets

# Print all variables
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

add_executable(main main.cpp)
target_include_directories(main
    PRIVATE
        ${httplib_SOURCE_DIR})
target_link_libraries(main 
    PRIVATE
        OpenSSL::SSL 
        OpenSSL::Crypto 
        Display
        DnsHandler
        SrvCache)

install(TARGETS main DESTINATION bin)