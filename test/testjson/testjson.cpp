#include<iostream>
#include"json.hpp"
using namespace std;
#include<vector>
#include<map>
#include<string>
using json=nlohmann::json;
// 示例1
string func1(){
    json js;
    // js添加数组
    js["id"]={1,2,3,4,5};
    // 添加key-value
    js["name"]="zhang san";
    // 添加对象
    js["msg"]["zhang san"]="hello world";
    js["msg"]["liu shuo"]="hello world";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"]={{"zhang san","hello world"},{"liu shuo","hello world"}};

    string sendBuf=js.dump();
    //cout<<sendBuf.c_str()<<endl;
    return sendBuf;
}

// json序列化示例代码
string func2(){
    json js;
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    js["list"]=vec;
    map<int,string> m;
    m.insert({1,"黄山"});
    m.insert({2,"华山"});
    m.insert({3,"泰山"});
    js["path"]=m;
    string sendBuf=js.dump();  // 将json对象转化成json字符串
    //cout<<sendBuf.c_str()<<endl;
    return sendBuf;

}

// json反序列化
void func3(){
    string recvBuf=func1();
    json jsbuf=json::parse(recvBuf);  // json 反序列化 数据对象

    cout<<jsbuf["id"][1]<<endl;
    cout<<jsbuf["name"]<<endl;
    cout<<jsbuf["msg"]["zhang san"]<<endl;
    // auto 自动类型推导
    jsbuf=json::parse(func2());


    vector<int> vec=jsbuf["list"];
    for(int &v : vec){
        cout<<v<<" ";
    }
    cout<<endl;


    
    map<int,string>  m=jsbuf["path"];
    for(auto &p:m){
        cout<<p.first<<" "<<p.second<<endl;
    }

}


int main(){

    func3();
    return 0;
}