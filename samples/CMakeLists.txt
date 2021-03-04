include_directories(
  ${PROJECT_SOURCE_DIR}/src/utils
  ${PROJECT_SOURCE_DIR}/src/communication/net
  ${PROJECT_SOURCE_DIR}/src/communication/serial)

#link_directories(${PROJECT_SOURCE_DIR}/lib)

#add_executable(echo_client  echo_client.cc echo_service.pb.cc)
#target_link_libraries(echo_client communication base protobuf glog pthread)

#add_executable(echo_server echo_server.cc echo_service.pb.cc)
#target_link_libraries(echo_server communication base protobuf glog pthread)