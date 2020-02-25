#include<iostream>
#include<json/json.h>
#include<memory>
#include<cstdio>
#include<sstream>

using namespace std;
int main()
{
    //string cmd = "while :; do echo hello;sleep 1; done";//可以执行脚本
    //string cmd = "ls -a -l";
    string cmd = "arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE demo.wav";//录音时如果不想让此信息显示出来可以将此信息重定向到文件或/dev/null里
    FILE *fp = popen(cmd.c_str(),"r");
    if(nullptr == fp)
    {              
        perror("popen");
        return 1;  
    }           
    char c;        
    while(fread(&c,1,1,fp) > 0)//从fp里面一次读取一个字节，读到的内容放到c里
    {
        fwrite(&c,1,1,stdout);//将c里的内容打到显示器上
    }
    pclose(fp);
    return 0;
}
//Json的序列化（将多个字符串转成一个json串）此处用的json是静态库
//int main()
//{
//    Json::Value root;
//    Json::StreamWriterBuilder wb;
//    std::ostringstream ss;//包含头文件<sstream>
//    
//    Json::Value item;
//    item["school"] = "清华大学";
//    item["high"] = 183.8f;
//
//    root["nick_name"] = "dream";
//    root["date"]="2019-10-14";
//    root["message"] = "你好";
//    root["age"]=20;
//   // root["info"] = item;//Json里面嵌套Json
//    root["info"][0] = item;//json嵌套数组
//    root["info"][1] = item;
//
//    unique_ptr<Json::StreamWriter> sw(wb.newStreamWriter());//相当于jsonwriter* js=wb.newStreamWriter
//    sw->write(root,&ss);//将root序列化放进ss中
//    string str = ss.str();//将流的信息转成字符串格式
//    cout << str << endl;
//    return 0;
//}
//Json的反序列化(将一个json串转化成root，再从root里提取)
//void JsonParse(std::string &s)//解析字符串
//{
//    JSONCPP_STRING errs;
//    Json::Value root;
//    Json::CharReaderBuilder rb;
//    unique_ptr<Json::CharReader> const cr(rb.newCharReader());
//    bool res = cr->parse(s.data(),s.data()+s.size(),&root,&errs);
//    if(!res || !errs.empty())
//    {
//        cerr << "parse Error!" << endl;
//        return;
//    }
//    cout << "age: " << root["age"].asInt() << endl;
//    cout << "date: " << root["date"].asString() << endl; 
//    cout << "meaasge: " << root["meaasge"].asString() << endl;
//    cout << "high: " << root["high"].asFloat() << endl;
//}
//int main()
//{
//    std::string s = "{\"age\" : 12,\"date\" : \"2019-10-14\",\"message\" : \"你好\",\"high\" : 183.8}";
//    JsonParse(s); 
//    return 0;
//}
//
