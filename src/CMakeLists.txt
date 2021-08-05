
add_subdirectory(utils)
add_subdirectory(communication)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/utils
  ${PROJECT_SOURCE_DIR}/src/components
  ${PROJECT_SOURCE_DIR}/src/communication/net
  ${PROJECT_SOURCE_DIR}/src/communication/serial)

aux_source_directory(. DIR_SRCS)

file(GLOB_RECURSE SRC_FILES 
 ${PROJECT_SOURCE_DIR}/src/components/*.c)

link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(control_engine ${DIR_SRCS} ${SRC_FILES})
target_link_libraries(control_engine utils communication zlog uv pthread)
