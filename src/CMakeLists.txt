
add_subdirectory(utils)
add_subdirectory(communication)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/utils
  ${PROJECT_SOURCE_DIR}/src/communication)

aux_source_directory(. DIR_SRCS)

file(GLOB_RECURSE SRC_FILES 
 ${PROJECT_SOURCE_DIR}/src/componets/*.c)


add_executable(main ${DIR_SRCS} ${SRC_FILES})
target_link_libraries(main utils communication zlog uv pthread)
