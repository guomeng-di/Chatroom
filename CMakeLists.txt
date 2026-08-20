cmake_minimum_required(VERSION 3.10)

project(chat_server)

# C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# 全局头文件查找根目录
include_directories(${PROJECT_SOURCE_DIR})

# ==================== 源码分组 ====================
# 网络底层
set(NETLIB_SRC

    netlib/base/Logger.cc

    netlib/net/EventLoop/EventLoop.cc
    netlib/net/Epoller/Epoller.cc
    netlib/net/Channel/Channel.cc
    netlib/net/Acceptor/Acceptor.cc
    netlib/net/TcpConnection/TcpConnection.cc
    netlib/net/TcpServer/TcpServer.cc
    netlib/net/Buffer/Buffer.cc
    netlib/net/EventLoopThread/EventLoopThread.cc
    netlib/net/EventLoopThreadPool/EventLoopThreadPool.cc
    netlib/net/TimerFd/TimerFd.cc
)

# 协议解析
set(PROTOCOL_SRC
    protocol/JsonProtocol/JsonProtocol.cc
    protocol/Message/Message.cc
    protocol/MessageCodec/MessageCodec.cc
)

# 业务服务层
set(SERVICE_SRC
    service/MessageDispatcher/MessageDispatcher.cc
    service/LoginService/LoginService.cc
    service/RegisterService/RegisterService.cc
    service/ChatService/ChatService.cc
    service/FriendService/FriendService.cc
    service/FriendRequestService/FriendRequestService.cc
    service/GroupChatService/GroupChatService.cc
    service/GroupService/GroupService.cc
    service/GroupRequestService/GroupRequestService.cc
    service/LogoutService/LogoutService.cc 
    service/FriendStatusService/FriendStatusService.cc
    service/DeleteAccountService/DeleteAccountService.cc
    service/HistoryService/HistoryService.cc
    service/GroupManageService/GroupManageService.cc
    service/VerifyCodeService/VerifyCodeService.cc
    service/ResetPasswordService/ResetPasswordService.cc 
    service/FriendBlockService/FriendBlockService.cc 
    service/FileService/FileService.cc
    netlib/base/SocketUtil/SocketUtil.cc

)

# 数据模型(内存用户表、在线用户管理)
set(MODEL_SRC
    model/UserModel/UserModel.cc
    model/FriendModel/FriendModel.cc
    model/FriendRequestModel/FriendRequestModel.cc
    model/GroupModel/GroupModel.cc 
    manager/OnlineUserManager/OnlineUserManager.cc
    model/PrivateMessageModel/PrivateMessageModel.cc
    model/GroupMessageModel/GroupMessageModel.cc
    model/GroupRequestModel/GroupRequestModel.cc
    model/FriendBlockModel/FriendBlockModel.cc
    model/FileModel/FileModel.cc
    
)
# Redis管理模块
set(REDIS_SRC
    manager/RedisManager/RedisManager.cc

)
# 数据库
set(DATABASE_SRC

    database/MySQLManager/MySQLManager.cc

    utils/SHA256/SHA256.cc

    utils/VerifyCode/VerifyCode.cc
    manager/EmailManager/EmailManager.cc

)
# ==================== 生成可执行程序 ====================
# 服务端：网络+协议+业务+模型
add_executable(server

    server/server.cc

    ${NETLIB_SRC}

    ${PROTOCOL_SRC}

    ${SERVICE_SRC}

    ${MODEL_SRC}

    ${REDIS_SRC}

    ${DATABASE_SRC}

    ${UTILS_SRC}


)

target_link_libraries(server
    hiredis
    mysqlclient
    crypto
    pthread
)

# 客户端：仅协议，不含业务与模型
add_executable(client
    client/client.cc
    netlib/base/Logger.cc
    utils/MD5/MD5.cc
    netlib/base/SocketUtil/SocketUtil.cc
    netlib/net/Buffer/Buffer.cc
    client/ClientMessageHandler/ClientMessageHandler.cc
    client/FileClient/FileClient.cc 
    manager/FileManager/FileManager.cc

    ${PROTOCOL_SRC}
)
target_link_libraries(client
    crypto
)

add_executable(test_sha256

utils/test_sha256.cc

utils/SHA256/SHA256.cc

)


target_link_libraries(test_sha256
crypto
)

find_package(CURL REQUIRED)

target_link_libraries(
    server
    CURL::libcurl
)


# ==================== GTest 单元测试 ====================

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