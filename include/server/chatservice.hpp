#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include <mutex>

#include "json.hpp"

using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;

// 回调函数类型
using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // ChatService 单例模式
    // thread safe
    static ChatService* instance() {
        static ChatService service;
        return &service;
    }

    // 登录业务
    void loginHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注册业务
    void registerHandler(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注销业务
    void loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //添加群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // 获取对应消息的处理器
    MsgHandler getHandler(int msgId);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常业务重置方法
    void reset();
    


    
   

private:
    ChatService();
    // 存储消息id和其对应的业务处理方法   
    std::unordered_map<int, MsgHandler> _msgHandlerMap;
    // 数据操作类对象
    UserModel _userModel;
    // 存储在在线用户的通信连接     注意线程安全 
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    // 定义互斥锁 保证_userConnMap的线程安全
    mutex _connMutex;
    OfflineMsgModel _offlineMsgModel;
    friendModel _friendModel;
    GroupModel _groupModel; 
  
};

#endif // CHATSERVICE_H