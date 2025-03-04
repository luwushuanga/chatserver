#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include "user.hpp"
#include<vector>
using namespace std;

class friendModel{
public:
// 添加好友关系
 void insert(int userid ,int friendid);
//返回用户好友列表 friendid 联合查询 
vector<User> query(int userid);





};


#endif