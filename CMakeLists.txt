#
#
cmake_minimum_required(VERSION 3.7)
project(servod)
set(PROJECT_VERSION "0.01")
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_BINARY_DIR}/include")
include_directories("/opt/vc/include")
link_directories("/opt/vc/lib")

set(SERVO_SRC src/servod.cc src/mailbox.cc)
add_executable(servod ${SERVO_SRC})
target_link_libraries(servod bcm_host)
