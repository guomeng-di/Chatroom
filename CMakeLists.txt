cmake_minimum_required(VERSION 3.10)

project(chat_server)


# =========================
# C++17
# =========================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")


# =========================
# include
# =========================

include_directories(${PROJECT_SOURCE_DIR})



# =========================
# 自动扫描
# =========================


# =========================
# server源码
# =========================

file(GLOB_RECURSE SERVER_SRC
    "${PROJECT_SOURCE_DIR}/*.cc"
)


# 排除客户端
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/client/.*")


# 排除测试目录
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/test/.*")


# 排除测试文件
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/test_.*\\.cc$")
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/.*_test\\.cc$")


# 排除其他main入口
list(FILTER SERVER_SRC EXCLUDE REGEX ".*/netlib/main\\.cc$")


# =========================
# client源码
# =========================

file(GLOB_RECURSE CLIENT_SRC

    "${PROJECT_SOURCE_DIR}/client/*.cc"

    "${PROJECT_SOURCE_DIR}/netlib/base/*.cc"

    "${PROJECT_SOURCE_DIR}/netlib/net/Buffer/*.cc"

    "${PROJECT_SOURCE_DIR}/netlib/base/SocketUtil/*.cc"

    "${PROJECT_SOURCE_DIR}/protocol/*.cc"

    "${PROJECT_SOURCE_DIR}/protocol/*/*.cc"

    "${PROJECT_SOURCE_DIR}/manager/FileManager/*.cc"

    "${PROJECT_SOURCE_DIR}/utils/MD5/*.cc"

)


# 排除client测试

list(FILTER CLIENT_SRC EXCLUDE REGEX ".*/test_.*\\.cc$")
list(FILTER CLIENT_SRC EXCLUDE REGEX ".*/.*_test\\.cc$")



# =========================
# server
# =========================


add_executable(server

    ${SERVER_SRC}

)


target_link_libraries(server

    hiredis

    mysqlclient

    crypto

    pthread

)



# CURL

find_package(CURL REQUIRED)


target_link_libraries(server

    CURL::libcurl

)



# =========================
# client
# =========================


add_executable(client

    ${CLIENT_SRC}

)


target_link_libraries(client

    crypto

    pthread

)



# =========================
# SHA256测试
# =========================


add_executable(test_sha256

    utils/test_sha256.cc

    utils/SHA256/SHA256.cc

)


target_link_libraries(test_sha256

    crypto

)



# =========================
# GTest
# =========================


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