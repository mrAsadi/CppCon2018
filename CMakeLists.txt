cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(CppCon2018 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Boost_USE_STATIC_LIBS ON)

if(MSVC)
  set (Boost_USE_STATIC_RUNTIME ON)
  add_definitions(-D_WIN32_WINNT=0x0601)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS=1)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
  add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
  add_compile_options(/MP)
  string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER build)
find_package(Boost REQUIRED COMPONENTS system json)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set(JWT_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(JWT_HEADER_FILES ${JWT_INCLUDE_PATH}/jwt-cpp/jwt.h)
foreach(traits ${JWT_JSON_TRAITS_OPTIONS})
  list(APPEND JWT_HEADER_FILES ${JWT_INCLUDE_PATH}/jwt-cpp/traits/${traits}/defaults.h
       ${JWT_INCLUDE_PATH}/jwt-cpp/traits/${traits}/traits.h)
endforeach()


file(GLOB_RECURSE BEAST_FILES
  ${Boost_INCLUDE_DIRS}/boost/beast/*.hpp
  ${Boost_INCLUDE_DIRS}/boost/beast/*.ipp)

file(GLOB_RECURSE JSON_FILES
  ${Boost_INCLUDE_DIRS}/boost/json/*.hpp
  ${Boost_INCLUDE_DIRS}/boost/json/*.ipp)


source_group(TREE ${Boost_INCLUDE_DIRS}/boost/beast PREFIX beast FILES ${BEAST_FILES})
source_group(TREE ${Boost_INCLUDE_DIRS}/boost/json PREFIX json FILES ${JSON_FILES})

file(GLOB APP_FILES
  beast.hpp
  json.hpp
  http_session.cpp
  http_session.hpp
  listener.cpp
  listener.hpp
  main.cpp
  net.hpp
  shared_state.cpp
  shared_state.hpp
  websocket_session.cpp
  websocket_session.hpp
  chat_client.html
  README.md)

source_group("" FILES ${APP_FILES})

add_executable(websocket-chat-server ${APP_FILES} ${BEAST_FILES} ${JSON_FILES})

if(NOT WIN32)
  find_package(Threads)
  add_library(jwt-cpp INTERFACE)
  add_library(jwt-cpp::jwt-cpp ALIAS jwt-cpp) # To match export
  target_compile_features(jwt-cpp INTERFACE cxx_std_11)
  target_link_libraries(websocket-chat-server PRIVATE Threads::Threads Boost::json jwt-cpp ${Boost_SYSTEM_LIBRARY} ${OPENSSL_LIBRARIES})

endif()

set_target_properties(websocket-chat-server PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY
  VS_STARTUP_PROJECT websocket-chat-server)

  target_include_directories(jwt-cpp INTERFACE $<BUILD_INTERFACE:${JWT_INCLUDE_PATH}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(${JWT_SSL_LIBRARY} MATCHES "OpenSSL")
target_link_libraries(jwt-cpp INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif()