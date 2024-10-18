ADD_EXECUTABLE(client.out client.c client_utils.c client_network.c)

TARGET_LINK_LIBRARIES(client.out PRIVATE dhcpv4_utils)
TARGET_INCLUDE_DIRECTORIES(client.out PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)
TARGET_INCLUDE_DIRECTORIES(client.out PRIVATE ${CMAKE_SOURCE_DIR}/src/include)