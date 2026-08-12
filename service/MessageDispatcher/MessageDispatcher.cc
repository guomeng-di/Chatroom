#include "MessageDispatcher.h"
#include "../LoginService/LoginService.h"
#include "../LogoutService/LogoutService.h"
#include "../RegisterService/RegisterService.h"
#include "../ChatService/ChatService.h"
#include "../GroupChatService/GroupChatService.h"
#include "../GroupService/GroupService.h"
#include "../GroupManageService/GroupManageService.h"
#include "../GroupRequestService/GroupRequestService.h"
#include "../VerifyCodeService/VerifyCodeService.h"
#include "../FriendBlockService/FriendBlockService.h"
#include "../HistoryService/HistoryService.h"
#include "../FriendService/FriendService.h"
#include "../FriendRequestService/FriendRequestService.h"
#include "../../netlib/net/TcpConnection/TcpConnection.h"
#include "../../protocol/MsgId.h"
#include <iostream>
#include "../DeleteAccountService/DeleteAccountService.h"
#include "../ResetPasswordService/ResetPasswordService.h"
#include "../FileService/FileService.h"
#include "../../netlib/base/Logger.h"
using namespace std;

MessageDispatcher::MessageDispatcher(){}
MessageDispatcher::~MessageDispatcher(){}

void MessageDispatcher::dispatch(const json& js,TcpConnection* conn){
    if(!js.contains("msgid")){
        //cout<<"json no msgid"<<endl;
        Logger::instance().error("json message no msgid");
        return;
    }
    int msgid=js["msgid"];
    switch(msgid){

//1登录
        case LOGIN_MSG:{
            json response=LoginService::login(js,conn);
        //     cout<<"dispatcher send login:"
        // <<response.dump()
        // <<endl;
            //conn->send(response.dump());
            break;
        }

//2注册
        case REGISTER_MSG:{
            json response=RegisterService::registerUser(js);
            conn->send(response.dump());
            break;
        }

//3私聊
        case CHAT_MSG:{
            json response=ChatService::chat(js,conn);
            conn->send(response.dump());
            break;
        }


//4群聊
        case GROUP_CHAT_MSG:{
            json response=GroupChatService::groupChat(js,conn);
            conn->send(response.dump());
            break;
        }

//5添加好友

        case ADD_FRIEND_MSG:{
            json response=FriendService::addFriend(js);
            conn->send(response.dump());
            break;
        }


//6查看好友列表
        case FRIEND_LIST_MSG:{
            json response=FriendService::getFriendList(js);
            conn->send(response.dump());
            break;
        }


//7删除好友
        case DELETE_FRIEND_MSG:{
            json response=FriendService::deleteFriend(js);
            conn->send(response.dump());
            break;
        }


//8发送好友申请
        case SEND_FRIEND_REQUEST_MSG:{
            json response=FriendRequestService::sendRequest(js);
            conn->send(response.dump());
            break;
        }


//9查询好友申请列表
        case GET_FRIEND_REQUEST_MSG:{
            json response=FriendRequestService::getRequestList(js);
            conn->send(response.dump());
            break;
        }


//10处理好友申请(同意/不同意)
        case HANDLE_FRIEND_REQUEST_MSG:{
            json response=FriendRequestService::handleRequest(js);
            conn->send(response.dump());
            break;
        }


//11创建群聊       
        case CREATE_GROUP_MSG:{
            json response=GroupService::createGroup(js);
            conn->send(response.dump());
            break;
        }


//12加入群聊
        case JOIN_GROUP_MSG:{
            json response=GroupService::joinGroup(js);
            conn->send(response.dump());
            break;
        }


//13(主动)退出群聊
        case LEAVE_GROUP_MSG:{
            json response=GroupService::leaveGroup(js);
            conn->send(response.dump());
            break;
        }


//14获得某人的群聊列表
        case GROUP_LIST_MSG:{
            json response=GroupService::getGroupList(js);
            conn->send(response.dump());
            break;
        }


//15查看某群的成员
        case GROUP_MEMBER_MSG:{
            json response=GroupService::getGroupMembers(js);
            conn->send(response.dump());
            break;
        }

//16主动退出
        case LOGOUT_MSG:{
            json response=LogoutService::logout(js);
            conn->send(response.dump());
            break;
        }

//19注销账号
       case DELETE_ACCOUNT_MSG:{
            json response=DeleteAccountService::removeAccount(js);
            conn->send(response.dump());
            break;
       }
        
//20查看私聊聊天记录
        case GET_PRIVATE_HISTORY:{
            json res = HistoryService::getPrivateHistory(js);
            conn->send(res.dump());
            break;
}

//21查看群聊聊天记录
        case GET_GROUP_HISTORY:{
            json res = HistoryService::getGroupHistory(js);
            conn->send(res.dump());
            break;
}

//22踢人
        case KICK_MEMBER_MSG:{
            json res = GroupManageService::kickMember(js);
            conn->send(res.dump());
            break;
}

//23解散群
        case DELETE_GROUP_MSG:{
            json res = GroupManageService::deleteGroup(js);
            conn->send(res.dump());
            break;
}

//24添加群管理员
        case ADD_GROUP_ADMIN_MSG:{
            json res = GroupManageService::addAdmin(js);
            conn->send(res.dump());
            break;
}

//25踢群管理员
        case REMOVE_GROUP_ADMIN_MSG:{
            json res = GroupManageService::removeAdmin(js);
            conn->send(res.dump());
            break;
}
 
//26查看群聊申请列表
        case GET_GROUP_REQUEST_MSG:{
            json res = GroupRequestService::getRequestList(js);
            conn->send(res.dump());
            break;
}

//27处理申请入群
        case HANDLE_GROUP_REQUEST_MSG:{
            json res = GroupRequestService::handleGroupRequest(js);
            conn->send(res.dump());
            break;
}

//28请求验证码
        case SEND_VERIFY_CODE_MSG:{
            json res =VerifyCodeService::sendCode(js);
            conn->send(res.dump());
            break;
}

//29心跳检测
//现在,客户端手动输入29,可以检测心跳,发送消息回复它:收到
        case HEARTBEAT_MSG:{
            Logger::instance().info("receive heartbeat");
            //conn->updateActiveTime();
            json res;
            res["msgid"]=HEARTBEAT_ACK;
            res["message"]="heartbeat ack";
            conn->send(res.dump());
            break;
}

//30重置密码
        case RESET_PASSWORD_MSG:{
            json res = ResetPasswordService::resetPassword(js);
            conn->send(res.dump());
            break;
}

//31屏蔽好友消息
        case ADD_BLOCK_MSG:{
            json res = FriendBlockService::addBlock(js);
            conn->send(res.dump());
            break;
}

//32取消屏蔽好友
        case REMOVE_BLOCK_MSG:{
            json res = FriendBlockService::removeBlock(js);
            conn->send(res.dump());
            break;
}
//33发送文件请求
        case SEND_FILE_REQUEST_MSG:{
            json res=FileService::sendFileRequest(js,conn);
            conn->send(res.dump());
            break;
        }
//34接受发送文件请求
        case FILE_ACCEPT_MSG:{
            json res=FileService::acceptFile(js,conn);
            conn->send(res.dump());
            break;
        }
//35发送文件
        case FILE_DATA_MSG:{
            FileService::sendFileData(js,conn);
        }
//36发送完毕
        case FILE_FINISH_MSG:{
            FileService::finishFile(js,conn);
            break;
        }
        
        
        default:
            Logger::instance().error(
        "unknown msgid="+
        to_string(msgid)
    );
        //cout<<"unknown msgid:"<<msgid<<endl;
            break;
    }

}