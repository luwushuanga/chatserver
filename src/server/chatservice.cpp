#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include<iostream>
using namespace muduo;
using namespace std;

// int getUserId(json& js) { return js["id"].get<int>(); }
// std::string getUserName(json& js) { return js["name"]; }
// 构造函数将消息信息与对应的处理器绑定
ChatService::ChatService()
{
    // 对各类消息处理方法的注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::loginHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({REGISTER_MSG, std::bind(&ChatService::registerHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});
    
    
   
}


MsgHandler ChatService::getHandler(int msgId)
{
    // 找不到对应处理器的情况
    auto it = _msgHandlerMap.find(msgId);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器(lambda匿名函数，仅仅用作提示)
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgId: " << msgId << " can not find handler!";
        }; 
    }
  
    return _msgHandlerMap[msgId];
}

// 处理登录业务  业务层操作的都是对象  数据层DAO具体数据库操作
void ChatService::loginHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id =js["id"];
    string pwd=js["password"];
    User user=_userModel.query(id);
    if(user.getPwd()==pwd&&user.getId()==id){

        if (user.getState()=="online"){
            // 该用户已经登录，不允许重复登录
        json response;
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=2;
        response["errmsg"]="this account is using ,input another";
        conn->send(response.dump());
        }else{
        // 登录成功，记录用户的连接信息  
       {
        lock_guard<mutex> lock(_connMutex);
        _userConnMap.insert({id,conn});

       } 
        // 登录成功  
        // 更新状态信息
        user.setState("online");
        _userModel.updateState(user);
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        response["name"]=user.getName();
        
        // 检查用户是否有离线消息  
        vector<string> vec;
        vec=_offlineMsgModel.query(id);
       if(!vec.empty()){

            response["offlinemsg"]=vec;
            
            _offlineMsgModel.remove(id);
       }
       // 查询该用户的好友信息并返回
        vector<User> userVec =_friendModel.query(id);
        if(!userVec.empty()){

            vector<string> vec2;
            for(User &user :userVec){
                json js;
                js["id"]=user.getId();
                js["name"]=user.getName();
                js["state"]=user.getState();
                vec2.push_back(js.dump());
            }
            response["friends"]=vec2;

        }
        // 查询用户的群组信息
        vector<Group> groupuserVec = _groupModel.queryGroups(id);
           if (!groupuserVec.empty())
             {
                 vector<string> groupV;
                 for (Group &group : groupuserVec)
                {
                     json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName(); 
                    grpjson["groupdesc"] = group.getDesc();
                     vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                {
                         json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                     grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                 response["groups"] = groupV;
                
 }

             conn->send(response.dump());
         }
 }
    else{ 
        // 登录失败
        json response;

        if(user.getId()==-1){
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=3;
        response["errmsg"]="this account is not exist";
        }else{
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]="id or password error";
        }

        
        
        conn->send(response.dump());

    }


    
}

// 注销业务
void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time){
    User u;
    int id=js["id"].get<int>();
    u.setId(id);
    {
                 lock_guard<mutex> lock(_connMutex);
                for(auto it=_userConnMap.begin();it!=_userConnMap.end();it++){
                    if(it->second==conn){
                        // 1 把用户的连接从map表中删除
                        _userConnMap.erase(it); 
                       

                        break;
                    }
                }
            } 
            _userModel.updateState(u);
            u.setState("offline");
            
    }

// 注册业务  name password
void ChatService::registerHandler(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name=js["name"];
    string pwd=js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state=_userModel.insert(user);
    if(state){
        // 注册成功
        json response;
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }else{
        // 注册失败
        json response;
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());

    }

   
}
// 服务器异常退出，业务重置方法
void ChatService::reset(){
    // 将online状态的用户全部重置为offline
    _userModel.resetState();

}
// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)

    {
            User user;

            {
                 lock_guard<mutex> lock(_connMutex);
                for(auto it=_userConnMap.begin();it!=_userConnMap.end();it++){
                    if(it->second==conn){
                        user.setId(it->first);
                        // 1 把用户的连接从map表中删除
                        _userConnMap.erase(it); 
                       

                        break;
                    }
                }
            }
           
             // 2 将用户的状态改为离线
             if(user.getId()!=-1){
            user.setState("offline");
             _userModel.updateState(user);
             }
              
    }

// 一对一聊天业务

 void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
        //获取 toid  
        int toid =js["toid"].get<int>();   // 这里的key一定得对应上
        {
            lock_guard<mutex> lock(_connMutex);
            auto it =_userConnMap.find(toid);
            if(it!=_userConnMap.end()){
                // 在线 发送消息
                // dump 将json对象序列化成字符串的函数
                it->second->send(js.dump());


                return ;
                
            
            }
        }
        // 不在线 存储离线消息

        _offlineMsgModel.insert(toid,js.dump());

}

// 添加好友业务  msgid   id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    
    int id=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    //存储好友信息
    _friendModel.insert(id,friendid);
}


// 创建群组业务
    void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
        int userid=js["id"].get<int>();
        string name=js["groupname"];
        string desc=js["groupdesc"];
        // 存储新创建的群组信息
        Group group;
        group.setName(name);
        group.setDesc(desc);
        if(_groupModel.createGroup(group)){
            _groupModel.addGroup(userid,group.getId(),"creator");
        }

    }
    //添加群组
    void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
        int userid=js["id"].get<int>();
        int groupid=js["groupid"].get<int>();
        _groupModel.addGroup(userid,groupid,"normal");

    }
    // 群组聊天
    void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
        int userid =js["id"].get<int>();
        int groupid=js["groupid"].get<int>();
        vector<int> useridvec=_groupModel.queryGroupUsers(userid,groupid);
       { lock_guard<mutex> lock(_connMutex);
        for (int id : useridvec){
            auto it=_userConnMap.find(id);
            if(it!=_userConnMap.end()){
                // 转发消息
                it->second->send(js.dump());
            }else{
                // 存储离线消息
                _offlineMsgModel.insert(id,js.dump());
            }
        }
       }

    }








