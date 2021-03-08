include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/utils
  ${PROJECT_SOURCE_DIR}/src/communication/net
  ${PROJECT_SOURCE_DIR}/src/communication/serial)

#link_directories(${PROJECT_SOURCE_DIR}/lib)

#add_executable(echo_client  echo_client.cc echo_service.pb.cc)
#target_link_libraries(echo_client communication base protobuf glog pthread)

add_executable(tcp_server tcp_server.c)
target_link_libraries(tcp_server communication utils uv zlog pthread)