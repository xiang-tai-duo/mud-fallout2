cmake_minimum_required(VERSION 3.20)
project(mud_fallout2)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/wd4819)
endif ()

include_directories("${CMAKE_SOURCE_DIR}/../vcpkg/packages/curl_x64-windows/include")
include_directories("${CMAKE_SOURCE_DIR}/../vcpkg/packages/openssl_x64-windows/include")
include_directories("${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/core/include")
include_directories("${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/tmt/include")
include_directories("${CMAKE_SOURCE_DIR}/../cpp-httplib")
include_directories("${CMAKE_SOURCE_DIR}/../json/single_include")

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    link_libraries(ws2_32)
    add_definitions(-DWINDOWS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
else()
    add_definitions(-DMAC)
endif ()

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/impl/userdef.hpp")
    file(WRITE "${CMAKE_SOURCE_DIR}/impl/userdef.hpp" "// TODO: Add your owner codes here\n")
endif ()

add_executable(${PROJECT_NAME}
        impl/stages.cpp
        impl/character.cpp
        impl/engine.cpp
        main.cpp
        impl/session.cpp
        impl/session.h
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Set-ExecutionPolicy Bypass -Scope Process
    # .\build.ps1 build -DBUILD_MODULES="tmt" -DCMAKE_TOOLCHAIN_FILE='../vcpkg/scripts/buildsystems/vcpkg.cmake'
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/sdk_build/lib/Debug/tencentcloud-sdk-cpp-core.lib")
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/sdk_build/lib/Debug/tencentcloud-sdk-cpp-tmt.lib")
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/../vcpkg/packages/openssl_x64-windows/lib/libssl.lib")
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/../vcpkg/packages/openssl_x64-windows/lib/libcrypto.lib")
    file(COPY "${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/sdk_build/bin/Debug/tencentcloud-sdk-cpp-core.dll" DESTINATION .)
    file(COPY "${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/sdk_build/bin/Debug/tencentcloud-sdk-cpp-tmt.dll" DESTINATION .)
    file(COPY "${CMAKE_SOURCE_DIR}/../vcpkg/packages/curl_x64-windows/debug/bin/libcurl-d.dll" DESTINATION .)
    file(COPY "${CMAKE_SOURCE_DIR}/../vcpkg/packages/zlib_x64-windows/debug/bin/zlibd1.dll" DESTINATION .)
    file(COPY "${CMAKE_SOURCE_DIR}/../vcpkg/packages/openssl_x64-windows/bin/legacy.dll" DESTINATION .)
    file(COPY "${CMAKE_SOURCE_DIR}/../vcpkg/packages/openssl_x64-windows/bin/libcrypto-3-x64.dll" DESTINATION .)
    file(COPY "${CMAKE_SOURCE_DIR}/../vcpkg/packages/openssl_x64-windows/bin/libssl-3-x64.dll" DESTINATION .)
else ()
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/lib/libtencentcloud-sdk-cpp-core.dylib")
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/../tencentcloud-sdk-cpp/lib/libtencentcloud-sdk-cpp-tmt.dylib")
endif ()