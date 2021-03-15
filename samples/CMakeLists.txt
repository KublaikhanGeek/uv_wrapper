include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/utils
  ${PROJECT_SOURCE_DIR}/src/communication/net
  ${PROJECT_SOURCE_DIR}/src/communication/serial)

#link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(tcp_server tcp_server.c)
target_link_libraries(tcp_server communication utils uv zlog pthread)

add_executable(tcp_client tcp_client.c)
target_link_libraries(tcp_client communication utils uv zlog pthread)

add_executable(tcp_server_client tcp_server_client.c)
target_link_libraries(tcp_server_client communication utils uv zlog pthread)

add_executable(udp_sender udp_sender.c)
target_link_libraries(udp_sender communication utils uv zlog pthread)

add_executable(udp_receiver udp_receiver.c)
target_link_libraries(udp_receiver communication utils uv zlog pthread)

add_executable(udp_multicast_sender udp_multicast_sender.c)
target_link_libraries(udp_multicast_sender communication utils uv zlog pthread)

add_executable(udp_multicast_receiver udp_multicast_receiver.c)
target_link_libraries(udp_multicast_receiver communication utils uv zlog pthread)

add_executable(udp_broadcast_sender udp_broadcast_sender.c)
target_link_libraries(udp_broadcast_sender communication utils uv zlog pthread)

add_executable(udp_broadcast_receiver udp_broadcast_receiver.c)
target_link_libraries(udp_broadcast_receiver communication utils uv zlog pthread)