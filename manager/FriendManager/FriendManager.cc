#include "FriendManager.h"
#include "../../model/FriendModel/FriendModel.h"
#include "../../model/FriendBlockModel/FriendBlockModel.h"


using namespace std;


FriendManager& FriendManager::instance(){
    static FriendManager instance;
    return instance;
}



void FriendManager::loadFriendList(const string& username)
{
    FriendModel friendModel;

    unordered_set<string> friends =friendModel.getFriends(username);


    lock_guard<mutex> lock(mutex_);


    friends_[username].clear();


    for(const auto& friendName : friends){
        friends_[username].insert(friendName);
    }
}



bool FriendManager::isFriend(
    const string& user1,
    const string& user2
)
{

    lock_guard<mutex> lock(mutex_);


    auto it=friends_.find(user1);


    if(it==friends_.end())
    {
        return false;
    }


    return it->second.count(user2)>0;
}




void FriendManager::addFriend(
    const string& user1,
    const string& user2
)
{

    lock_guard<mutex> lock(mutex_);


    friends_[user1].insert(user2);


    friends_[user2].insert(user1);

}




void FriendManager::removeFriend(
    const string& user1,
    const string& user2
)
{

    lock_guard<mutex> lock(mutex_);


    auto it1=friends_.find(user1);


    if(it1!=friends_.end())
    {
        it1->second.erase(user2);
    }



    auto it2=friends_.find(user2);


    if(it2!=friends_.end())
    {
        it2->second.erase(user1);
    }

}


void FriendManager::loadBlockList(const string& username)
{

    FriendBlockModel model;


    vector<string> blocks =
        model.getBlockList(username);



    lock_guard<mutex> lock(mutex_);



    blocks_[username].clear();



    for(auto& name:blocks)
    {
        blocks_[username].insert(name);
    }


}
bool FriendManager::isBlocked(
    const string& blocker,
    const string& target
)
{

    lock_guard<mutex> lock(mutex_);


    auto it=blocks_.find(blocker);


    if(it==blocks_.end())
    {
        return false;
    }


    return it->second.count(target)>0;

}
void FriendManager::addBlock(
    const string& blocker,
    const string& target
)
{

    lock_guard<mutex> lock(mutex_);


    blocks_[blocker].insert(target);

}
void FriendManager::removeBlock(
    const string& blocker,
    const string& target
)
{

    lock_guard<mutex> lock(mutex_);


    auto it=blocks_.find(blocker);


    if(it!=blocks_.end())
    {
        it->second.erase(target);
    }

}