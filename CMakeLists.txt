cmake_minimum_required(VERSION 3.20)
project(zigbee_mesh VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ZIGBEE_BUILD_TESTS "Build tests" ON)
option(ZIGBEE_BUILD_EXAMPLES "Build examples" ON)
option(ZIGBEE_BUILD_CLI "Build CLI application" ON)
option(ZIGBEE_USE_OPENSSL "Use OpenSSL for crypto" ON)
option(ZIGBEE_USE_SQLITE "Use SQLite for storage" ON)
option(ZIGBEE_ENABLE_LOGGING "Enable logging" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)

if(ZIGBEE_USE_OPENSSL)
    find_package(OpenSSL REQUIRED)
endif()

if(ZIGBEE_USE_SQLITE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
endif()

if(ZIGBEE_BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

add_library(zigbee_mesh_core STATIC
    src/drivers/driver_factory.cpp
    src/drivers/mrf24j40.cpp
    src/drivers/cc2530.cpp
    src/drivers/xbee.cpp
    src/zigbee/ieee802154.cpp
    src/zigbee/security_manager.cpp
    src/zigbee/zdo_layer.cpp
    src/zigbee/zigbee_stack.cpp
    src/routing/routing_table.cpp
    src/mesh/mesh_manager.cpp
    src/services/network_manager.cpp
    src/security/security_service.cpp
    src/storage/storage_manager.cpp
    src/cli/cli.cpp
)

target_include_directories(zigbee_mesh_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(zigbee_mesh_core
    PUBLIC
        Threads::Threads
)

if(ZIGBEE_USE_OPENSSL)
    target_link_libraries(zigbee_mesh_core PUBLIC OpenSSL::Crypto)
    target_compile_definitions(zigbee_mesh_core PUBLIC ZIGBEE_USE_OPENSSL=1)
endif()

if(ZIGBEE_USE_SQLITE)
    target_include_directories(zigbee_mesh_core PUBLIC ${SQLITE3_INCLUDE_DIRS})
    target_link_libraries(zigbee_mesh_core PUBLIC ${SQLITE3_LIBRARIES})
    target_compile_definitions(zigbee_mesh_core PUBLIC ZIGBEE_USE_SQLITE=1)
endif()

if(ZIGBEE_ENABLE_LOGGING)
    target_compile_definitions(zigbee_mesh_core PUBLIC ZIGBEE_ENABLE_LOGGING=1)
endif()

target_compile_options(zigbee_mesh_core PRIVATE
    -Wall -Wextra -Wpedantic -Wno-unused-parameter
)

if(ZIGBEE_BUILD_CLI)
    add_executable(zigbee-cli src/main.cpp)
    target_link_libraries(zigbee-cli PRIVATE zigbee_mesh_core)
    set_target_properties(zigbee-cli PROPERTIES
        OUTPUT_NAME "zigbee-cli"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

if(ZIGBEE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ZIGBEE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

install(TARGETS zigbee_mesh_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

include(GNUInstallDirs)
install(FILES configs/zigbee_mesh.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/zigbee_mesh
)

message(STATUS "========================================")
message(STATUS "  Zigbee Mesh v${PROJECT_VERSION}")
message(STATUS "  Build Tests: ${ZIGBEE_BUILD_TESTS}")
message(STATUS "  Build Examples: ${ZIGBEE_BUILD_EXAMPLES}")
message(STATUS "  Build CLI: ${ZIGBEE_BUILD_CLI}")
message(STATUS "  OpenSSL: ${ZIGBEE_USE_OPENSSL}")
message(STATUS "  SQLite: ${ZIGBEE_USE_SQLITE}")
message(STATUS "========================================")
