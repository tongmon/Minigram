cmake_minimum_required(VERSION 3.23)

execute_process(
  COMMAND git pull
  WORKING_DIRECTORY ${CMAKE_TOOLCHAIN_FILE}/../../..)
  
execute_process(
  COMMAND vcpkg x-update-baseline --add-initial-baseline
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

project("DBTestModule" VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

file(GLOB SRC_FILES CONFIGURE_DEPENDS ./*.cpp)

# DB 연동을 위해 SOCI 포함
find_package(SOCI CONFIG REQUIRED)

# MongoDB CXX Driver 포함
find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC 
    SOCI::soci_core
    SOCI::soci_postgresql
    mongo::bsoncxx_shared
    mongo::mongocxx_shared
)