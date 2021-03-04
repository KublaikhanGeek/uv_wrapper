aux_source_directory(. DIR_SRCS)

include_directories(
  ${PROJECT_SOURCE_DIR}/include)

add_library(utils ${DIR_SRCS})
target_link_libraries(utils uv zlog pthread)

