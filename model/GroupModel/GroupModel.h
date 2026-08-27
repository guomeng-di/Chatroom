 #pragma once
 #include <string>
 #include <unordered_map>
 #include <unordered_set>
 class GroupModel{
     public:
       GroupModel();
       ~GroupModel();
       // 创建群
       bool createGroup(const std::string& groupName,const std::string& owner,const std::unordered_set<std::string>& members);
       // 用户加入群
       bool addMember(const std::string& groupName,const std::string& username);
       // 用户退出群
       bool leaveGroup(const std::string& groupName,const std::string& username);
       // 获取群成员
       std::unordered_set<std::string> getMembers(const std::string& groupName);
       // 获取用户加入的所有群
       std::unordered_set<std::string> getGroups(const std::string& username);
       //判断群是否存在
       bool groupExist(const std::string& groupName);
       //判断用户是否已经在群里
       bool isMember(const std::string& groupName,const std::string& username);
       //判断是否是群主
       bool isOwner(const std::string& groupName,const std::string& username);
       //判断是否是管理员
       bool isAdmin(const std::string& groupName,const std::string& username);
       //踢人
       bool removeMember(const std::string& groupName,const std::string& username);
       //解散群
       bool deleteGroup(const std::string& groupName);
       //添加管理员
       bool addAdmin(const std::string& groupName,const std::string& username);
       //删除管理员(身份)
       bool removeAdmin(const std::string& groupName,const std::string& username);
       //得知群主是谁(有好友申请了,需要通知)
       std::string getOwner(const std::string& groupname);
       //得知管理员
       std::unordered_set<std::string> getAdmins(const std::string& groupname);
       //group_member:删除某人所在的所有的群的记录(注销账号)
       bool removeAllGroups(const std::string& username);
       //chat_group:删除用户创建的群
       bool removeOwnerGroups(const std::string& username);
       //group_admin:删除管理员身份(所有群)
       bool removeAdmin_(const std::string& username);
       //踢出去
       bool deleteAdmin(const std::string& groupName,const std::string& username);
 };