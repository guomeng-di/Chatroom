#pragma once
#include <nlohmann/json.hpp>
#include "../../protocol/MessageCodec/MessageCodec.h"

using json=nlohmann::json;
class TcpConnection;
class FileService{
    public:
      static json sendFileRequest(const json& js,TcpConnection* conn);
      static json acceptFile(const json& js,TcpConnection* conn);
      static void sendFileData(const json& js,TcpConnection* conn);
      static void receiveFileData(const FilePacket& packet,TcpConnection* conn);//服务器接收二进制消息
      static void finishFile(const json& js,TcpConnection* conn);
      static json querySendFileBlock(const json& js,TcpConnection* conn);
      //static json resumeFile(const json& js,TcpConnection* conn);
      static void fileBlockAck(
    const json& js,
    TcpConnection* conn
);
    };