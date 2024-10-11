ADD_EXECUTABLE(client.out client.c)

TARGET_LINK_LIBRARIES(client.out PRIVATE dhcpv4_utils)
TARGET_INCLUDE_DIRECTORIES(client.out PRIVATE ${CMAKE_SOURCE_DIR}/lib/include)