cmake_minimum_required(VERSION 3.15)
project(KeyloggerResearchProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX /MP /Oi /GL)
    add_link_options(/LTCG)
    string(REPLACE "/MT" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(WindowsSDK REQUIRED)
find_package(Crypto REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${WINDOWSSDK_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    advapi32.lib
    crypt32.lib
    winhttp.lib
    wininet.lib
    dnsapi.lib
    iphlpapi.lib
    psapi.lib
    gdiplus.lib
    taskschd.lib
    comsuppw.lib
    ${CRYPTO_LIBRARIES}
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
)

configure_file(
    ${CMAKE_SOURCE_DIR}/resources/config/default.cfg
    ${CMAKE_BINARY_DIR}/default.cfg
    COPYONLY
)