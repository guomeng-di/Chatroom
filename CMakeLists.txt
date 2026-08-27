cmake_minimum_required(VERSION 3.10)

project(chat_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

include_directories(${PROJECT_SOURCE_DIR})

find_package(glog REQUIRED)

file(GLOB_RECURSE SERVER_SRC
    "${PROJECT_SOURCE_DIR}/*.cc"
)

list(FILTER SERVER_SRC EXCLUDE REGEX ".*/client/.*")
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/test/.*")
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/test_.*\\.cc$")
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/.*_test\\.cc$")
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/netlib/main\\.cc$")

file(GLOB_RECURSE CLIENT_SRC
    "${PROJECT_SOURCE_DIR}/client/*.cc"
    "${PROJECT_SOURCE_DIR}/netlib/base/*.cc"
    "${PROJECT_SOURCE_DIR}/netlib/net/Buffer/*.cc"
    "${PROJECT_SOURCE_DIR}/netlib/base/SocketUtil/*.cc"
    "${PROJECT_SOURCE_DIR}/protocol/*.cc"
    "${PROJECT_SOURCE_DIR}/protocol/*/*.cc"
    "${PROJECT_SOURCE_DIR}/manager/FileManager/*.cc"
    "${PROJECT_SOURCE_DIR}/model/FileModel/*.cc"
    "${PROJECT_SOURCE_DIR}/model/FriendModel/*.cc"
    "${PROJECT_SOURCE_DIR}/model/GroupModel/*.cc"
    "${PROJECT_SOURCE_DIR}/database/MySQLManager/*.cc"
    "${PROJECT_SOURCE_DIR}/utils/MD5/*.cc"
)

list(FILTER CLIENT_SRC EXCLUDE REGEX ".*/test_.*\\.cc$")
list(FILTER CLIENT_SRC EXCLUDE REGEX ".*/.*_test\\.cc$")

add_executable(server
    ${SERVER_SRC}
)

target_link_libraries(server
    hiredis
    mysqlclient
    crypto
    pthread
    glog::glog
)

find_package(CURL REQUIRED)

target_link_libraries(server
    CURL::libcurl
)

add_executable(client
    ${CLIENT_SRC}
)

target_link_libraries(client
    mysqlclient
    crypto
    pthread
    glog::glog
)

add_executable(test_sha256
    utils/test_sha256.cc
    utils/SHA256/SHA256.cc
)

target_link_libraries(test_sha256
    crypto
)

enable_testing()

find_package(GTest REQUIRED)

add_executable(unit_tests
    test/MessageCodecTest.cc
    test/BufferTest.cc
    test/JsonProtocolTest.cc
    test/SHA256Test.cc
    protocol/MessageCodec/MessageCodec.cc
    netlib/net/Buffer/Buffer.cc
    protocol/JsonProtocol/JsonProtocol.cc
    utils/SHA256/SHA256.cc
)

target_include_directories(unit_tests PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(unit_tests
    GTest::gtest
    GTest::gtest_main
    pthread
    crypto
)

add_test(
    NAME unit_tests
    COMMAND unit_tests
)