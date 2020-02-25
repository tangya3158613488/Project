#pragma once
#include<iostream>
#include<string>
#include<cstdio>
#include<sstream>
#include<unistd.h>
#include<stdlib.h>
#include<memory>
#include<fstream>
#include<map>
#include<unordered_map>
#include<pthread.h>
#include<json/json.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include"speech.h"
#include "base/http.h"

#define ASR_PATH "temp_file/asr.wav"//ASR表示自动语音识别,语音识别文件路径
//#define TTS_PATH "temp_file/tts.mp3"
#define LOG "log.txt"
#define CMD_ETC "command.etc"
#define TTS_PATH "temp_file/play.mp3"
//工具类
class Util
{
private:
       static pthread_t id;
public:
        static bool Exec(std::string command,bool is_print)
        {
            if(is_print)
            {
                command += ">/dev/null 2>&1";
            }
            FILE *fp = popen(command.c_str(),"r");
            if(nullptr == fp)
            {
                std::cerr << "popen exec\'" << command << "\' Error" << std::endl;
                return false;
            }
            if(is_print)
            {
                char ch;
                while(fread(&ch,1,1,fp) > 0)
                {
                    fwrite(&ch,1,1,stdout);
                }
            }
            pclose(fp);
            return true;
        }
        static void *ThreadRun(void *arg)
        {
            pthread_detach(pthread_self());
            const char *tips = (char*)arg;
            int i = 0;
            char bar[53] = {0};
            const char *lable = "|/-\\";
            for(;i <= 50;i++)
            {
                printf("%s[%-51s][%d%%][%c]\r",tips,bar,i*2,lable[i%4]);
                fflush(stdout);
                bar[i] = '=';
                bar[i+1] = '>';
                usleep(49000*2);
            }
            printf("\n");
        }
        static void PrintStart(std::string tips)
        {
            pthread_create(&id,NULL,ThreadRun,(void*)tips.c_str());
        }
        static void PrintEnd()
        {
            pthread_cancel(id);
        }
};
pthread_t Util::id;
//机器人类
class Robot
{
private: 
        std::string url;
        std::string api_key;
        std::string user_id;
        aip::HttpClient client;//HttpClient包含在speech/base/http.h中
private:
        bool IsCodeLegal(int code)
        {
            bool result = false;
            switch(code)
            {
             case 5000:
                 break;
             case 10004:
                 result = true;
                 break;
            }
            return result;
        }
        //将信息转换成json串
        std::string MessageToJson(std::string &message)
        {
            Json::Value root;
            Json::StreamWriterBuilder wb;
            std::ostringstream os;//os当作一段内存，将所有输入的信息放入os

            Json::Value item_;
            item_["text"] = message;

            Json::Value item;
            item["inputText"] = item_;

            root["reqType"] = 0;//0表示文本 text
            root["perception"] = item;

            item.clear();//清理之前的内容
            item["apiKey"] = api_key;
            item["userId"] = user_id;
            root["userInfo"] = item;
            
            std::unique_ptr<Json::StreamWriter> sw(wb.newStreamWriter());//使用智能指针的好处是不用考虑垃圾回收了，防止内存泄漏
            sw->write(root,&os);
            //测试json
            std::string json_string = os.str();
           // std::cout << "json串的内容: " << json_string << std::endl;
            return json_string;
        }
        //向图灵机器人发起http post请求
        std::string RequestTL(std::string &request_json)
        {
           std::string response;
           int status_code = client.post(url,nullptr,request_json,nullptr,&response);//客户端发起请求
           if(status_code != CURLcode::CURLE_OK)
           {
               std::cerr << "post error" << std::endl;
               return "";
           }
           return response;
        }
       //将 图灵机器人返回的json串转换为文本信息
        std::string JsonToEchoMessage(std::string &str)
        {

            //std::cout << "JsonToEchoMessage:" << str << std::endl;
            JSONCPP_STRING errs;
            Json::Value root;
            Json::CharReaderBuilder rb;

            std::unique_ptr<Json::CharReader> const cr(rb.newCharReader());
            //将创建好的cr对象调用parse接口解析str
            bool res = cr->parse(str.data(),str.data()+str.size(),&root,&errs);
             if(!res || !errs.empty())
            {
                  std::cerr << "parse error" << std::endl;
                  return "";
            } 
            int code = root["intent"]["code"].asInt();
            if(!IsCodeLegal(code))//根据退出码查看当前的返回信息是否正常
            {
               std::cerr << "response code error" << std::endl;
               return "";
            }
            Json::Value item = root["results"][0];
            std::string msg = item["values"]["text"].asString();
            return msg;
        }
    public:
        Robot(std::string id = "1")
        {
            this->url = "http://openapi.tuling123.com/openapi/api/v2";
            this->api_key = "2e92fea2fe204f95b4009b193101a09c";
            this->user_id = id;
        }
        std::string Talk(std::string message)
        {
            //massage转成Json
            std::string json = MessageToJson(message); 
            std::string response = RequestTL(json);//发起请求，请求图灵机器人
            std::string echo_message = JsonToEchoMessage(response);//返回给客户的消息
            return echo_message;
        }
        ~Robot()
        {
        }
};
//语音识别合成类
class SpeechRec
{
private:
        //填写appid,Api Key,Secret Key
        std::string app_id;
        std::string api_key;
        std::string secret_key;
        aip::Speech *client;//client是语音识别的C++客户端
private:
        bool IsCodeLegal(int code)
        {
            bool result = false;
            switch(code)
            {
                case 0:
                    result = true;
                    break;
                default:
                    std::cerr << "code error..." << std::endl;
                    break;
            }
            return result;
        }
public:
        SpeechRec()
        {
            app_id = "16869283";
            api_key = "HH0TlE9DLs6uYXFDiVvLgunl";
            secret_key = "utpSWmVzdeosdnHklE8F4Kr1gArAtTU1";
            client = new aip::Speech(app_id,api_key,secret_key);
        }
        //语音识别，向远程服务器上传整段语音进行识别
        bool ASR(std::string path,std::string &out)
        {
            std::map<std::string,std::string> options;
            options["dev_pid"] = "1536";//普通话
            std::string file_content;
            aip::get_file_content(ASR_PATH,&file_content);//要识别的语音文件
            Json::Value result = client->recognize(file_content,"wav",16000,options);
            //std::cout << "debug: " << result.toStyledString() << std::endl;//json格式的字符串
            int code = result["err_no"].asInt();
            if(!IsCodeLegal(code))
            {   
                std::cerr << "recongize error" << std::endl;
                return false;
            }
            out = result["result"][0].asString();
            return true;
        }
        //语音合成
        bool TTS(std::string message)
        {
            bool ret;
            std::ofstream ofile;//文本输出流;
            std::string ret_file;//返回的文件，语音合成之后的二进制语音流
            std::map<std::string,std::string> options;//语音库，填充选项
            options["spd"] = "5";//语速 0~15
            options["pit"] = "5";//音调0~15
            options["vol"] = "15";//音量0~15
            options["per"] = "4";//度小宇=1，度小美=0，度逍遥=3，度丫丫=4度博文=106，度小童=110，度小萌=111，度米朵=103，度小娇=5 
            options["aue"] = "6";
            ofile.open(TTS_PATH,std::ios::out | std::ios::binary);
            //语音合成，将文本转换成语音，放到指定的目录
            Json::Value result = client->text2audio(message,options,ret_file);
            if(!ret_file.empty())
            {
                ofile << ret_file;
                ofile.close();
                ret = true;
            }
            else
            {
                std::cerr << result.toStyledString() << std::endl;
                ret = false;
            }
            ofile.close();
            return ret;
        }
        ~SpeechRec()
        {
        
        }
};
//核心逻辑类
class Sakura
{
private:
        Robot rt;
        SpeechRec sr;
        std::unordered_map<std::string,std::string> commands;
private:
        bool Record()
        {
            std::cout << "debug: Record..." << std:: endl;
            std::string command = "arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE ";
            command += ASR_PATH;
            bool ret = Util::Exec(command,false);
           // std::cout << "debug: Record ... done" << std::endl;
            return ret;
        }
        bool Play()
        {
            std::string command = "cvlc --play-and-exit ";
            command += TTS_PATH;
            if(!Util::Exec(command,false))
            {
                std::cerr << "play error" << std::endl;
                return false;
            }
            return true;
        }
public:
        Sakura()
        {}
        bool LoadEtc()//加载命令文件
        {
            //1.读文件
            //2.解析内容
            //3.把解析的命令Key,Value值插入到map
            std::ifstream in(CMD_ETC);
            if(!in.is_open())
            {
                std::cerr << "open error" << std::endl;
                return false;
            }
            std::string sep = ":";//定义分隔符:
            char line[256];
            while(in.getline(line,sizeof(line)))
            {
                std::string str = line;
                std::size_t pos = str.find(sep);
                if(std::string::npos == pos)
                {
                    std::cerr << "not find :" << std::endl;
                    continue;
                }
                std::string k = str.substr(0,pos);//从0开始提取k，到pos处为止
                std::string v = str.substr(pos+sep.size());
                k += "。";                
                commands.insert(std::make_pair(k,v));
                //commands.insert({k,v});
            }
            std::cerr << "Load command etc done... success" << std::endl;
            in.close();
            return true;
        }
        bool IsCommand(std::string message,std::string &cmd)
        {
            auto iter = commands.find(message);
            if(iter == commands.end())
            {
                return false;
            }
            cmd = iter->second;
            return true;
        }
        //执行核心逻辑
        void Run()
        {
#ifdef _LOG_
            int fd = open(LOG,O_WRONLY|O_CREAT,0644);
            dup2(fd,2);//所有内容都往log中打
#endif
            volatile bool quit = false;
            while(!quit)
            {
                std::string tips = "Record...";
                Util::PrintStart(tips);
                if(this->Record())//录音成功
                {
                    std::string message;
                    if(sr.ASR(ASR_PATH,message))
                    {
                        //1.判断是否是命令
                        std::string cmd = "";
                        if(IsCommand(message,cmd))
                        {
                            std::cout << "[Sakura@localhost]$" << cmd << std::endl;
                            Util::Exec(cmd,true);
                            continue;
                        }
                        if(message == "你走吧。")
                        {
                            std::cout << "我# " << message <<std::endl;
                            std:: string quit_message = "那我就走了，记得没事来找我哦！";
                            std::cout << "Sakura# " << quit_message << std::endl;
                            if(sr.TTS(quit_message))
                            {
                                this->Play();
                            }
                            exit(0);
                        }
                        //2.将消息交给图灵机器人处理
                        std::cout << "我# " << message << std::endl;
                        std::string echo = rt.Talk(message);
                        std::cout << "Sakura# " << echo << std::endl;
                        if(sr.TTS(echo))
                        {
                            this->Play();
                        }
                    }
                    else
                    {
                        std::cerr << "Recognize error" << std::endl;
                    }
                }
                else//录音失败
                {
                    std::cerr << "Record error..." << std::endl;
                }
               Util::PrintEnd();
            }
#ifdef _LOG_
            close(fd);
#endif
        }
        ~Sakura()
        {
        }

};

