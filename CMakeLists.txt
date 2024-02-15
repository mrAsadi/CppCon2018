cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(ir-websocket-server VERSION 0.1.0 LANGUAGES CXX)

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
find_package(Boost REQUIRED COMPONENTS context filesystem system thread json REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set(JWT_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(JWT_HEADER_FILES ${JWT_INCLUDE_PATH}/jwt-cpp/jwt.h)
foreach(traits ${JWT_JSON_TRAITS_OPTIONS})
  list(APPEND JWT_HEADER_FILES ${JWT_INCLUDE_PATH}/jwt-cpp/traits/${traits}/defaults.h
       ${JWT_INCLUDE_PATH}/jwt-cpp/traits/${traits}/traits.h)
endforeach()

set(CERTIFICATE_DIR ${CMAKE_BINARY_DIR}/certs)
file(MAKE_DIRECTORY ${CERTIFICATE_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/ssl/ DESTINATION ${CMAKE_BINARY_DIR}/certs/)
add_custom_target(generate_certificates DEPENDS
    ${CERTIFICATE_DIR}/certs/
)

file(GLOB_RECURSE JSON_FILES
  ${Boost_INCLUDE_DIRS}/boost/json/*.hpp
  ${Boost_INCLUDE_DIRS}/boost/json/*.ipp)

source_group(TREE ${Boost_INCLUDE_DIRS}/boost/json PREFIX json FILES ${JSON_FILES})

get_filename_component (BOOST_ROOT ../../ ABSOLUTE)
add_definitions (-DBOOST_ALL_STATIC_LINK=1)
add_definitions (-DBOOST_ASIO_NO_DEPRECATED=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_BIND=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_REGEX=1)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING=1)

set(BOOST_INCLUDEDIR ${BOOST_ROOT})
set(BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib)
find_package(Boost COMPONENTS context filesystem system thread REQUIRED)
link_libraries(Boost::context Boost::filesystem Boost::system Boost::thread)
link_directories(${BOOST_ROOT}/stage/lib)

add_definitions (-DBOOST_ASIO_SEPARATE_COMPILATION=1)
add_definitions (-DBOOST_BEAST_SEPARATE_COMPILATION=1)

source_group (lib "/")

add_library (
    lib-asio STATIC
    include/lib/lib_asio.cpp
)

add_library (
    lib-asio-ssl STATIC
    include/lib/lib_asio_ssl.cpp
)

set_property(TARGET lib-asio-ssl PROPERTY FOLDER "static-libs")

add_library (
    lib-beast STATIC
    include/lib/lib_beast.cpp
)

set_property(TARGET lib-beast PROPERTY FOLDER "static-libs")

target_link_libraries(lib-beast lib-asio jwt-cpp)
source_group(include/boost/beast beast)
source_group(common common)


file(GLOB APP_FILES
  common/server_certificate.hpp
  beast.hpp
  http_session.cpp
  detect_session.hpp
  http_session.hpp
  listener.cpp
  listener.hpp
  main.cpp
  shared_state.cpp
  shared_state.hpp
  websocket_session.cpp
  websocket_session.hpp
  chat_client.html
  Jamfile
  README.md)

source_group("" FILES ${APP_FILES})
add_executable(ir-websocket-server ${APP_FILES} ${JSON_FILES})

if(NOT WIN32)
  find_package(Threads)
  add_library(jwt-cpp INTERFACE)
  add_library(jwt-cpp::jwt-cpp ALIAS jwt-cpp) # To match export
  target_compile_features(jwt-cpp INTERFACE cxx_std_11)
  target_link_libraries(ir-websocket-server PRIVATE Threads::Threads Boost::json jwt-cpp ${Boost_SYSTEM_LIBRARY} ${OPENSSL_LIBRARIES})
endif()

set_target_properties(ir-websocket-server PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY
  VS_STARTUP_PROJECT ir-websocket-server)

  target_include_directories(jwt-cpp INTERFACE $<BUILD_INTERFACE:${JWT_INCLUDE_PATH}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(${JWT_SSL_LIBRARY} MATCHES "OpenSSL")
target_link_libraries(jwt-cpp INTERFACE OpenSSL::SSL OpenSSL::Crypto Boost::context Boost::filesystem Boost::system Boost::thread 
lib-asio
lib-asio-ssl
lib-beast)

endif()