#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;
/*
  muduo网络库给用户提供了两个主要的类
  Tcpserver  用于编写服务器程序的
  TcpClient 用于编写客户端程序的
  epoll + 线程池  
  好处  能够把网络I/O的代码和业务代码分开
                用户的连接和断开   用户的可读写事件
*/
/*
  基于muduo网络库开发服务器程序
  1 组合TcpServer对象
  2 创建eventLoop事件循环对象的指针
  3 明确TcpServer构造函数需要什么参数，输出ChatServer的构造参数
  4. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
  5. 设置合适的服务器的线程数量  muduo库回自己分配io线程和工作线程

  */

class ChatServer
{
  public:

  // 构造函数
    ChatServer(EventLoop *loop,  // 事件循环
              const InetAddress &listenAddr,  // ip地址加端口
              const string &nameArg)     // 服务器的名称
              :_server(loop,listenAddr,nameArg),_loop(loop)   // 初始化列表初始化成员变量
    {
      // 给服务器注册用户连接和断开回调   
      _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
      
      // 给服务器注册用户读写回调
      _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));// 参数占位符

      // 设置服务器端的线程数量
      _server.setThreadNum(4);  // 1个io线程 3个工作线程


    }
    void start(){
      _server.start();  // 开启事件循环
    }


  private:
  //专门处理用户的连接和断开   epoll listenfd accept
    void onConnection(const TcpConnectionPtr& conn){

      
        if(conn->connected()){
          cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<" state:online"<<endl;
        }else{
          cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<" state:offline"<<endl;
          conn->shutdown();// close(fd)
          //_loop->quit(); 连接断开
        }

    }
    //专门处理用户读写事件
    void onMessage(const TcpConnectionPtr &conn,// 连接
                        Buffer* buff,//缓冲区 
                         Timestamp time) // 接收数据的事件信息
    {
        string buf=buff->retrieveAllAsString();
        cout<<"reciver data:"<<buf<<" time:"<<time.toString()<<endl;
        conn->send(buf);

    }


    TcpServer _server;   //#1
    EventLoop *_loop;   // #2 epoll

};



int  main(){

  EventLoop loop;
  InetAddress addr("127.0.0.1",6000);
  ChatServer server(&loop,addr,"ChatServer");
  server.start();  // 启动服务器  listenfd  epoll_ctl=>epoll
  loop.loop();  // epoll_wait 以阻塞的方式等待新用户连接，已连接用户的读写事件等操作



  return 0;


}

