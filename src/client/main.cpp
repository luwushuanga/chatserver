#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json= nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;


// 显示当前登录用户的基本信息
void showCurrentUserData();
// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int);

// 聊天客户端程序实现  main线程用做发送线程  子线程用来接收线程
int main(int argc ,char** argv){                                               
    if (argc <3){
        cerr<<"command invalid example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    // 解析通过命令行参数传递ip和port
    char * ip=argv[1];
    uint16_t port=atoi(argv[2]);
    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    // 创建 client 端的socket
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);

    // client和server连接
    if(-1==connect(clientfd,(sockaddr*)&server,sizeof(sockaddr_in))){
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }
    for(;;){
        cout<<"===================="<<endl;
        cout<<"1.login"<<endl;
        cout<<"2.register"<<endl;
        cout<<"3.quit"<<endl;
        cout<<"===================="<<endl;
        cout<<"choice:";
        int choice=0;
        cin>>choice;
        cin.get(); // 读掉缓冲区的回车
        switch(choice){

            case 1:
            {
                int id=0;
                char pwd[50]={0};
                cout<<"useid:";
                cin>>id;
                cin.get();
                cout<<"password:";
                cin.getline(pwd,50);
                 json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error" << request << endl;
            }else{
                char buffer[1024]={0};
                len=recv(clientfd ,buffer,1024,0);
                if(-1==len){
                    cerr<<"recv login response error"<<endl;

                }else{
                    json responsejs=json::parse(buffer);

                    // 登录失败  打印错误
                    if(0!=responsejs["errno"].get<int>())
                    {
                        cerr<<responsejs["errmsg"]<<endl;
                    }
                    else
                    {
                        // 记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);
                        // 记录当前登录用户的好友列表信息  防止再次登录信息重复
                        if(responsejs.contains("friends")){
                            // 初始化操作
                            g_currentUserFriendList.clear();
                            vector<string> vec =responsejs["friends"];
                            for (string &str : vec){
                                json js=json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }
                        // 记录当前登录用户的群组列表信息
                        if(responsejs.contains("groups")){
                            vector<string> vec1=responsejs["groups"];
                            //初始化操作
                            g_currentUserGroupList.clear();
                            for(string &groupstr: vec1){
                                Group g;
                                json groupjs =json::parse(groupstr);
                                g.setId(groupjs["id"].get<int>());
                                g.setName(groupjs["groupname"]);
                                g.setDesc(groupjs["groupdesc"]);
                                if(groupjs.contains("users")){    // 错误这里的一定是groupjs 而不是js
                                vector<string> vec2=groupjs["users"];   
                                for(string &userstr: vec2){
                                    GroupUser user;
                                    json js=json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    g.getUsers().push_back(user);
                                }
                                }
                                
                                g_currentUserGroupList.push_back(g);
                            }
                        }
                        //显示用户的基本信息
                        showCurrentUserData();
                        // 显示用户的离线消息 个人消息或者群组消息
                        if(responsejs.contains("offlinemsg")){
                            vector<string> vec =responsejs["offlinemsg"];
                            for(string &str :vec){
                                json js =json::parse(str);
                                if(ONE_CHAT_MSG ==js["msgid"].get<int>()){
                                    cout<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
                                    
                                }else{
                                     cout<<"群消息["<<js["groupid"]<<"] "<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
                                   
                                }
                            } 
                             

                        }
                        // 登录成功后 启动接收线程负责接收数据  该线程只启动一次  每个客户端的读线程只用启动一次
                        static int readthreadnumber=0;
                        if (readthreadnumber == 0){
                        std::thread readTask(readTaskHandler,clientfd);  // 底层调用了pthread_create
                        readTask.detach();   // 调用pthread_detach
                        readthreadnumber++;
                        }
                        
                        isMainMenuRunning=true;
                        // 进入聊天主菜单页面
                        mainMenu(clientfd);

                    }
                }

            }
            }
            break;
            case 2:
            {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REGISTER_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else{
                char buffer [1024]={0};
                len=recv(clientfd,buffer,1024,0);
                if(-1==len){
                    cerr<<"recv reg response error"<<endl;

                }else{
                    json responsejs=json::parse(buffer);
                    if(0!=responsejs["errno"].get<int>()){
                        cerr<<name<<"is already exist,register error"<<endl;
                    }else // 注册成功
                    {
                        cout<<name<<"register success ,userid is"<<responsejs["id"]<<",do not forget it"<<endl;
                    }
                }   
            }     
        }
        break;
        case 3: // quit业务
            {
                        close(clientfd);
                       
                        exit(0);
            }
        default:
            {
                cerr << "invalid input!" << endl;  
             }
            break;

        }
       
        
    }
    return 0;
}
// 各个主界面命令的处理函数
void help(int fd=0,string str="");

void chat(int,string);

void addfriend(int,string);

void creategroup(int,string);

void addgroup(int,string);

void groupchat(int,string);

void loginout(int,string);


// 系统支持的客户端命令列表
unordered_map<string,string> commandMap={
    {"help","显示所有支持的命令，格式help"},
    {"chat","一对一聊天，格式chat:friendid:message"},
    {"addfriend","添加好友格式 addfriend:friendid"},
    {"creategroup","创建群组，格式 creategroup:groupname:groupdesc"},
    {"addgroup","添加群组 格式 addgroup:groupid"},
    {"groupchat","群聊 格式 groupchat:groupid:message"},
    {"loginout","注销，格式 loginout"}
};

// 注册系统支持的客户端命令处理
unordered_map<string,function<void(int,string)>> commandHandlerMap={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}

};


void mainMenu(int clientfd){
    help();
    char buffer[1024]={0};
   while(isMainMenuRunning){
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx =commandbuf.find(":");
        if(-1==idx){
            command=commandbuf;
        }else{
            command=commandbuf.substr(0,idx);
        }
        auto it =commandHandlerMap.find(command);
        if(it==commandHandlerMap.end()){
            cerr<<"invalid input command!!"<<endl;
            continue;
        }
        // 调用相应的命令时间的处理回调  mainmenu对修改封闭，添加功能不需要修改该函数
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));   // 调用命令处理方法
       
    }


}


// 子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    for(;;){
        char buffer [1024]={0};
        int len=recv(clientfd,buffer,1024,0);   // 阻塞了
        if(-1==len||0==len){
            close(clientfd);
            exit(-1);
        }
        // 接收chatserver 转发的数据，反序列化生成json数据对象  然后输出在客户端
        json js =json::parse(buffer);
        int msgtype=js["msgid"].get<int>();
        if(ONE_CHAT_MSG ==msgtype){
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
            continue;
        } 
        if(GROUP_CHAT_MSG ==msgtype){
            cout<<"群消息["<<js["groupid"]<<"] "<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
    }
 
}
// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}


void help(int clientfd,string str){
    cout<<"show command list>>>>"<<endl;
    for(auto &p:commandMap){
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}


void addfriend(int clientfd,string str){
    int friendid =atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);   // 参数列表  套接字   发送长度  发送标志
    if(-1==len)
    {
        cerr<<"send addfriend msg err"<<buffer<<endl;
    }
}

// "chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

void creategroup(int clientfd,string str)
{
    int idx = str.find(":"); // groupname:groupdesc
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname=str.substr(0,idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }

}

void addgroup(int clientfd,string str)
{
    int groupid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);   // 参数列表  套接字   发送长度  发送标志
    if(-1==len)
    {
        cerr<<"send addgroup msg err"<<buffer<<endl;
    }
   

}

void groupchat(int clientfd,string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }

}
// 注销程序
void loginout(int clientfd,string str)
{
    int groupid=atoi(str.c_str());
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);   // 参数列表  套接字   发送长度  发送标志
    if(-1==len)
    {
        cerr<<"send loginout msg err"<<buffer<<endl;
    }else{
        isMainMenuRunning=false;
    }
    

}


// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
