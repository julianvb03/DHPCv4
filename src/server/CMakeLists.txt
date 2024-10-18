ADD_EXECUTABLE(server.out server.c server_utils.c server_network.c)

TARGET_LINK_LIBRARIES(server.out PRIVATE dhcpv4_utils)
TARGET_INCLUDE_DIRECTORIES(server.out PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
TARGET_INCLUDE_DIRECTORIES(server.out PRIVATE ${CMAKE_SOURCE_DIR}/src/include)