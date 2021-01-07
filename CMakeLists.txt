cmake_minimum_required(VERSION 3.14)
project(server)

#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -Wall -Werror -Wpedantic -fPIC")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/http_server/include)

add_library(http_message STATIC
            ${PROJECT_SOURCE_DIR}/http_server/src/HttpResponse.cpp
            ${PROJECT_SOURCE_DIR}/http_server/src/HttpRequest.cpp
            ${PROJECT_SOURCE_DIR}/http_server/src/HttpResponseReader.cpp
            ${PROJECT_SOURCE_DIR}/http_server/src/HttpRequestCreator.cpp
            ${PROJECT_SOURCE_DIR}/http_server/src/TemplateManager.cpp)


add_executable(server ${PROJECT_SOURCE_DIR}/http_server/src/main.cpp)
target_link_libraries(server http_message)

###################
# Socket Library
###################
add_library(socket STATIC
            ${PROJECT_SOURCE_DIR}/http_server/socket/src/socket.cpp)
target_include_directories(socket PUBLIC
                           ${PROJECT_SOURCE_DIR}/http_server/socket/include)
###################

###################
# HTTPClient Library
###################
add_library(client STATIC
            ${PROJECT_SOURCE_DIR}/http_server/HTTPClient/src/HTTPClient.cpp)
target_include_directories(client PUBLIC
                           ${PROJECT_SOURCE_DIR}/http_server/HTTPClient/include)
target_link_libraries(client socket)
###################

######################
# Async Server Library
######################
file(GLOB ASYNC_SRC
     ${PROJECT_SOURCE_DIR}/http_server/async/src/msleep.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/Task.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/TaskFuncs.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/Worker.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/TaskBuilder.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/TasksController.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/Listener.cpp
     ${PROJECT_SOURCE_DIR}/http_server/async/src/Master.cpp)

add_library(asyncServer STATIC ${ASYNC_SRC})

# Add pkg-config functionality
find_package(PkgConfig REQUIRED) 

# search for libevent.pc
pkg_search_module(EVENT REQUIRED libevent)

target_include_directories(asyncServer PUBLIC
                           ${PROJECT_SOURCE_DIR}/http_server/async/include
                           ${EVENT_INCLUDE_DIRS})
target_link_libraries(asyncServer client pthread http_message pqxx ${EVENT_LIBRARIES})
######################

################################
# Testing
################################

# Options. Turn on with 'cmake -Dmyvarname=ON'.
#                       'cmake -DENABLE_TESTING=ON'.
option(ENABLE_TESTING "Build all tests." OFF) # Makes boolean 'test' available.

if (ENABLE_TESTING)
  file(GLOB TEST_SRC http_server/test/main.cpp)

  # Install Gtest
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.8.0
  )
  FetchContent_MakeAvailable(googletest)
  include(GoogleTest)

  include(CTest)
 
  # Link runTests with what we want to test and the GTest and DZ1 library
  add_executable(runTests ${TEST_SRC})
  target_link_libraries(runTests gtest_main pthread)

  target_link_libraries(runTests socket)
  target_link_libraries(runTests client)
  target_link_libraries(runTests asyncServer)

  gtest_discover_tests(runTests)

endif()

target_link_libraries(server socket)
target_link_libraries(server client)
target_link_libraries(server pthread)
target_link_libraries(server asyncServer)
