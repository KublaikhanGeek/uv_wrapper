include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/utils)

aux_source_directory(. DIR_SRCS)

file(GLOB_RECURSE SRC_FILES 
 ${PROJECT_SOURCE_DIR}/src/communication/net/*.c
 ${PROJECT_SOURCE_DIR}/src/communication/serial/*.c)

add_library(communication ${DIR_SRCS} ${SRC_FILES})
target_link_libraries(communication utils uv zlog pthread)
