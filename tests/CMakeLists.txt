# Test CMakeLists.txt
add_executable(zigbee_tests
    unit/test_types.cpp
    unit/test_bytebuffer.cpp
    unit/test_result.cpp
    unit/test_routing.cpp
    unit/test_mesh.cpp
)

target_link_libraries(zigbee_tests
    PRIVATE
        zigbee_mesh_core
        GTest::gtest_main
)

target_include_directories(zigbee_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(zigbee_tests)
