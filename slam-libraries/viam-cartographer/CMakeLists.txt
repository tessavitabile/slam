cmake_minimum_required(VERSION 3.2)

project(viam-cartographer)

set(VIAM_CARTOGRAPHER_MAJOR_VERSION 1)
set(VIAM_CARTOGRAPHER_MINOR_VERSION 0)
set(VIAM_CARTOGRAPHER_PATCH_VERSION 0)
set(VIAM_CARTOGRAPHER_VERSION ${CARTOGRAPHER_MAJOR_VERSION}.${CARTOGRAPHER_MINOR_VERSION}.${CARTOGRAPHER_PATCH_VERSION})
set(VIAM_CARTOGRAPHER_SOVERSION ${CARTOGRAPHER_MAJOR_VERSION}.${CARTOGRAPHER_MINOR_VERSION})

include("${PROJECT_SOURCE_DIR}/cmake/functions.cmake")

find_package(absl REQUIRED)
find_package(Ceres REQUIRED COMPONENTS SuiteSparse)
find_package(Lua 5.2 REQUIRED)


# WARNING: PR Review comment:
# Absolute paths are generally not a good idea. If at all possible,
# just use the library name itself (cartographer) for linking.
# `/usr/local/lib` should be in the LD path on most linux systems already,
# so that should "just work."
# The include is a similar thing, though the files would need to be reference
# as <cartographer/file.h> instead of just <file.h> if they're in their own folder.

# TODO[kat]: First attempts showed that in order to fix this, a cmake module has to be written.
# This task is postponed until I start working on these two tickets (the last two tickets to be tackled):
# https://viam.atlassian.net/browse/DATA-123
# https://viam.atlassian.net/browse/DATA-124
include_directories("/usr/local/include/cartographer")
link_libraries("/usr/local/lib/libcartographer.a")

find_package(PCL REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

if(WIN32)
  # On Windows, Protobuf is incorrectly found by the bundled CMake module, so prefer native CMake config.
  set(protobuf_MODULE_COMPATIBLE TRUE CACHE INTERNAL "")
  find_package(Protobuf 3.0.0 CONFIG)
else()
  find_package(Protobuf 3.4.0 REQUIRED)
endif()

include(FindPkgConfig)
if (NOT WIN32)
  PKG_SEARCH_MODULE(CAIRO REQUIRED cairo>=1.12.16)
else()
  find_library(CAIRO_LIBRARIES cairo)
endif()

file(GLOB_RECURSE ALL_VIAM_HDRS "src/*.h")
file(GLOB_RECURSE ALL_VIAM_SRCS "src/*.cc")

# Remove dotfiles/-folders that could potentially pollute the build.
file(GLOB_RECURSE ALL_DOTFILES ".*/*")
if (ALL_DOTFILES)
  list(REMOVE_ITEM ALL_VIAM_HDRS ${ALL_DOTFILES})
  list(REMOVE_ITEM ALL_VIAM_SRCS ${ALL_DOTFILES})
endif()

foreach(ABS_FIL ${ALL_PROTOS})
  file(RELATIVE_PATH REL_FIL ${PROJECT_SOURCE_DIR} ${ABS_FIL})
  get_filename_component(DIR ${REL_FIL} DIRECTORY)
  get_filename_component(FIL_WE ${REL_FIL} NAME_WE)

  list(APPEND ALL_PROTO_SRCS "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.cc")
  list(APPEND ALL_PROTO_HDRS "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.h")

  add_custom_command(
    OUTPUT "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.cc"
           "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.h"
    COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --cpp_out  ${PROJECT_BINARY_DIR} -I
      ${PROJECT_SOURCE_DIR} ${ABS_FIL}
    DEPENDS ${ABS_FIL}
    COMMENT "Running C++ protocol buffer compiler on ${ABS_FIL}"
    VERBATIM
  )
endforeach()

list(APPEND ALL_LIBRARY_HDRS ${ALL_VIAM_HDRS})
list(APPEND ALL_LIBRARY_SRCS ${ALL_VIAM_SRCS})

add_library(${PROJECT_NAME} STATIC ${ALL_LIBRARY_HDRS} ${ALL_LIBRARY_SRCS})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "/usr/local/include")

configure_file(
  ${PROJECT_SOURCE_DIR}/cartographer/cartographer/common/config.h.cmake
  ${PROJECT_BINARY_DIR}/common/config.h)

message("PROJECT_SOURCE_DIR and PROJECT_BINARY_DIR: ${PROJECT_SOURCE_DIR}, ${PROJECT_BINARY_DIR}")

google_binary(main
  SRCS
    src/main.cc
)

google_binary(main_old
  SRCS
    src/main_old.cc
)

# WARNING: PR Review comment:
# Absolute paths are generally not a good idea. If at all possible,
# just use the library name itself (cartographer) for linking.
# `/usr/local/lib` should be in the LD path on most linux systems already,
# so that should "just work."
# The include is a similar thing, though the files would need to be reference
# as <cartographer/file.h> instead of just <file.h> if they're in their own folder.

# TODO[kat]: First attempts showed that in order to fix this, a cmake module has to be written.
# This task is postponed until I start working on these two tickets (the last two tickets to be tackled):
# https://viam.atlassian.net/browse/DATA-123
# https://viam.atlassian.net/browse/DATA-124
target_link_libraries(main PUBLIC "/usr/local/lib/libcartographer.a")
target_include_directories(main SYSTEM PUBLIC "/usr/local/include/cartographer")

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  "${CERES_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} PUBLIC ${CERES_LIBRARIES})

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  "${LUA_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} PUBLIC ${LUA_LIBRARIES})

if (WIN32)
  find_package(glog REQUIRED)
  set(GLOG_LIBRARY glog::glog)
else()
  set(GLOG_LIBRARY glog)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${GLOG_LIBRARY})
target_link_libraries(${PROJECT_NAME} PUBLIC gflags)
if(WIN32)
  # Needed to fix conflict with MSVC's error macro.
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DGLOG_NO_ABBREVIATED_SEVERITIES)
endif()
if(MSVC)
  # Needed for VS 2017 5.8
  target_compile_definitions(${PROJECT_NAME} PUBLIC -D_ENABLE_EXTENDED_ALIGNED_STORAGE -D_USE_MATH_DEFINES)
endif()

if("${CAIRO_INCLUDE_DIRS}")
  target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    "${CAIRO_INCLUDE_DIRS}")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${CAIRO_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${PCL_LIBRARIES})

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROTOBUF_LIBRARY} 
  absl::algorithm
  absl::base
  absl::debugging
  absl::flat_hash_map
  absl::memory
  absl::meta
  absl::numeric
  absl::str_format
  absl::strings
  absl::synchronization
  absl::time
  absl::utility 
)
if (NOT WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
endif()

set(TARGET_COMPILE_FLAGS "${TARGET_COMPILE_FLAGS} ${GOOG_CXX_FLAGS}")
set_target_properties(${PROJECT_NAME} PROPERTIES
  COMPILE_FLAGS ${TARGET_COMPILE_FLAGS})

# Add the binary directory first, so that port.h is included after it has
# been generated.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT CartographerExport
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

foreach(HDR ${INSTALL_SOURCE_HDRS})
  file(RELATIVE_PATH REL_FIL ${PROJECT_SOURCE_DIR} ${HDR})
  get_filename_component(DIR ${REL_FIL} DIRECTORY)
  install(
    FILES ${HDR}
    DESTINATION include/${DIR}
  )
endforeach()

foreach(HDR ${INSTALL_GENERATED_HDRS})
  file(RELATIVE_PATH REL_FIL ${PROJECT_BINARY_DIR} ${HDR})
  get_filename_component(DIR ${REL_FIL} DIRECTORY)
  install(
    FILES ${HDR}
    DESTINATION include/${DIR}
  )
endforeach()
