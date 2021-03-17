include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/utils
  ${PROJECT_SOURCE_DIR}/src/communication/net
  ${PROJECT_SOURCE_DIR}/src/communication/serial)

#link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(test_tcp_server test_tcp_server.c)
target_link_libraries(test_tcp_server communication utils uv zlog pthread)

add_executable(test_tcp_client test_tcp_client.c)
target_link_libraries(test_tcp_client communication utils uv zlog pthread)

add_executable(test_tcp_server_client test_tcp_server_client.c)
target_link_libraries(test_tcp_server_client communication utils uv zlog pthread)

add_executable(test_udp_sender test_udp_sender.c)
target_link_libraries(test_udp_sender communication utils uv zlog pthread)

add_executable(test_udp_receiver test_udp_receiver.c)
target_link_libraries(test_udp_receiver communication utils uv zlog pthread)

add_executable(test_udp_multicast_sender test_udp_multicast_sender.c)
target_link_libraries(test_udp_multicast_sender communication utils uv zlog pthread)

add_executable(test_udp_multicast_receiver test_udp_multicast_receiver.c)
target_link_libraries(test_udp_multicast_receiver communication utils uv zlog pthread)

add_executable(test_udp_broadcast_sender test_udp_broadcast_sender.c)
target_link_libraries(test_udp_broadcast_sender communication utils uv zlog pthread)

add_executable(test_udp_broadcast_receiver test_udp_broadcast_receiver.c)
target_link_libraries(test_udp_broadcast_receiver communication utils uv zlog pthread)

add_executable(test_serial test_serial.c)
target_link_libraries(test_serial communication utils uv zlog pthread)