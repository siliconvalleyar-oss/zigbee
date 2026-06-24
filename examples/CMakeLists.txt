# Example executables

add_executable(coordinator_example coordinator/coordinator.cpp)
target_link_libraries(coordinator_example PRIVATE zigbee_mesh_core)
set_target_properties(coordinator_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)

add_executable(router_example router/router.cpp)
target_link_libraries(router_example PRIVATE zigbee_mesh_core)
set_target_properties(router_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)

add_executable(end_device_example end_device/end_device.cpp)
target_link_libraries(end_device_example PRIVATE zigbee_mesh_core)
set_target_properties(end_device_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
