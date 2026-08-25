#include <bits/stdc++.h>
using namespace std;

class FriendManager{
public:

    static FriendManager& instance();

    //加载用户好友
    void loadFriendList(const string& username);

    //判断好友
    bool isFriend(
        const string& user1,
        const string& user2
    );


    //添加好友
    void addFriend(
        const string& user1,
        const string& user2
    );


    //删除好友
    void removeFriend(
        const string& user1,
        const string& user2
    );

    bool isBlocked(
    const string& blocker,
    const string& target
);


void addBlock(
    const string& blocker,
    const string& target
);


void removeBlock(
    const string& blocker,
    const string& target
);
void loadBlockList(const string& username);
private:

    //username -> 好友集合
    unordered_map<string,unordered_set<string>> friends_;
    unordered_map<string,unordered_set<string>> blocks_;

    mutex mutex_;

};