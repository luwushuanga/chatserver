#include "friendmodel.hpp"
#include "db.hpp"




// 添加好友关系
 void friendModel::insert(int userid ,int friendid){

     // 组装sql语句
    char sql[1024]={0};
    sprintf(sql,"insert into friend values (%d,%d)",userid,friendid);
    MySQL mysql;
    if(mysql.connect()){
        
        mysql.update(sql);
           
           
        
    }
   

 }
//返回用户好友列表 friendid 联合查询 
vector<User> friendModel::query(int userid){

  char sql[1024]={0};
// 多表联合查询
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d",userid);
    vector<User> vec;
    MySQL mysql;
    if(mysql.connect()){
       
       MYSQL_RES *res=mysql.query(sql);
       if(res!=nullptr){
        MYSQL_ROW row=mysql_fetch_row(res);
        // 把userid所有的离线消息放入vector 返回
        while(row!=nullptr){
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
            row=mysql_fetch_row(res);
        }
        mysql_free_result(res);
        return vec;
        
       }


    }
    return vec;

}