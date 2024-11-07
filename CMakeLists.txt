cmake_minimum_required(VERSION 3.19)
project(CloCXX)

set(GENERATED_HEADER ${CMAKE_BINARY_DIR}/clocxx.hpp)

add_executable(mkclocxx mkclocxx.cpp)

add_custom_command(
  OUTPUT ${GENERATED_HEADER}
  COMMAND ./mkclocxx ${CMAKE_SOURCE_DIR}
  DEPENDS mkclocxx
)

install(
    FILES ${GENERATED_HEADER}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
