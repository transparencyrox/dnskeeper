# Internal libraries

file(GLOB SrvCache_sources SrvCache.cpp)
add_library(SrvCache ${SrvCache_sources})
target_include_directories(SrvCache 
    PRIVATE
        ${PQXX_SDK}/include
        ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SrvCache 
    PUBLIC
        pqxx pq)


file(GLOB DnsHandler_sources DnsHandler.cpp)
add_library(DnsHandler ${DnsHandler_sources})
target_include_directories(DnsHandler 
    PRIVATE
        ${AWS_SDK}/include
        ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(DnsHandler
    PUBLIC
        ${AWS_LINKAGE})

file(GLOB Display_sources Display.cpp)
add_library(Display ${Display_sources})
target_include_directories(Display 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(Display
    PUBLIC
        fmt::fmt)