#pragma once

#include <iostream>
#include <string>
#include <cstdio>
#include <sstream>
#include <signal.h>
#include "mysql.h"
#include "Util.hpp"
#include "mongoose.h"

#define IM_DB "im_db"
#define MY_PORT 3306
#define NUM 1024

#define SESSION_ID "im_sid"
#define SESSION_NAME "im_name"
#define SESSION_CHECK_INTERVAL 5.0
#define SESSION_TTL 1800.0

struct mg_serve_http_opts s_http_server_opts;

typedef struct session
{
    uint64_t id;
    string name;
    double created;
    double last_used;
}session_t;

class Session
{
private:
        session_t sessions[NUM];
public:
        Session()
        {
            for(auto i = 0; i < NUM; i++)
            {
                sessions[i].id = 0;
                sessions[i].name = "";
                sessions[i].created = 0.0;
                sessions[i].last_used = 0.0;
            }
        }
        bool GetSession(http_message *hm)
        {
            uint64_t sid;
            char ssid[64];
            char *s_ssid = ssid;

            struct mg_str *cookie_header = mg_get_http_header(hm,"cookie");
            if(nullptr == cookie_header)
            {
                return false;
            }
            string info = Util::mgStrToString(cookie_header);
            cout << "cookie: " << info << endl;
            if(!mg_http_parse_header2(cookie_header,SESSION_ID,&s_ssid,sizeof(ssid)))
            {
                return false;
            }

            sid = strtoull(ssid,NULL,10);
            for(auto i = 0; i < NUM; i++)
            {
                if(sessions[i].id == sid)
                {
                    sessions[i].last_used = mg_time();
                    return true;
                }
            }
            return false;
        }
        bool IsLogin(http_message *hm)
        {
            return GetSession(hm);
        }
        bool CreateSession(string name,uint64_t &id)
        {
            int i = 0;
            for(; i < NUM; i++)
            {
                if(sessions[i].id == 0)
                {
                    break;
                }
            }
            if(i == NUM)
            {
                return false;
            }
            sessions[i].id = (uint64_t)(mg_time()*1000000L);
            sessions[i].name = name;
            sessions[i].last_used = sessions[i].created = mg_time();

            id = sessions[i].id;
            return true;
        }
        void DestroySession(session_t *s)
        {
            s->id = 0;
        }
        void CheckSession()
        {
            double threadhold = mg_time() - SESSION_TTL;
            for(auto i = 0; i< NUM; i++)
            {
                if(sessions[i].id > 0 && sessions[i].last_used < threadhold)
                {
                    DestroySession(sessions+i);
                }
            }
        }
        ~Session()
        {}
};
//model
class MysqlClient
{
private:
        MYSQL *my;
public:
        bool ConnectMysql()
        {
            my = mysql_init(NULL);
            mysql_set_character_set(my,"utf8");
            if(!mysql_real_connect(my,"localhost","root","123456",IM_DB,MY_PORT,NULL,0))
            {
                cerr << "connect mysql error" << endl;
                return false;
            }
            cout << "connect mysql success" << endl;
            return true;
        }
public :
        MysqlClient()
        {}
        bool InsertUser(string name,string passwd)
        {
            ConnectMysql();
            string sql = "INSERT INTO user (name,passwd) values(\"";
            sql += name;
            sql += "\",\"";
            sql += passwd;
            sql += "\")";
            cout << sql << endl;
            if(0 == mysql_query(my,sql.c_str()))
            {
                return true;
            }
            mysql_close(my);
            return false;
        }
        bool SelectUser(string name,string passwd)
        {
            ConnectMysql();
            string sql = "SELECT * FROM user WHERE name=\"";
            sql += name;
            sql += "\" and passwd=\"";
            sql += passwd;
            sql += "\"";
            cout << sql << endl;
            if(0 != mysql_query(my,sql.c_str()))
            {
                return false;
            }
            cout << "select done" << endl;
            //judge result not empty
            MYSQL_RES *result = mysql_store_result(my);
            int num = mysql_num_rows(result);
            cout << num << endl;
            free(result);
            mysql_close(my);
            return num > 0 ? true : false;
        }
        ~MysqlClient()
        {
            mysql_close(my);
        }
};

//MVC
class ImServer{
private:
        string port;
        struct mg_mgr mgr;//事件管理器，它是将所有的mg_connection使用链表连接起来的
        struct mg_connection *nc;
        volatile bool quit;
        static MysqlClient mc;
        static Session sn;
public:
        ImServer(string _port = "8080"):port(_port) ,quit(false)
        {}
        static void Broadcast(struct mg_connection *nc,string msg)
        {
            struct mg_connection *c;
            for(c = mg_next(nc->mgr,NULL);c != NULL;c = mg_next(nc->mgr,c))
            {
                if(c == nc)
                    continue;
                mg_send_websocket_frame(c,WEBSOCKET_OP_TEXT,msg.c_str(),msg.size());
            }
        }
        static void RegisterHandler(mg_connection *nc,int ev,void *data)
        {
            string code = "0";
            string echo_json="{\"result\":";
            struct http_message *hm = (struct http_message*)data;
            string method = Util::mgStrToString(&(hm->method));
            if(method == "POST")
            {
                string body = Util::mgStrToString(&hm->body);
                cout << "login handler: " << body << endl;
                string name,passwd;
                if(Util::GetNameAndPasswd(body,name,passwd) && !name.empty() && !passwd.empty())
                {
                   if(mc.InsertUser(name,passwd))
                   {
                        code = "0";
                   }
                   else
                   {
                        code = "1";
                   }
                }
                else
                {   
                    code = "2";
                }
                echo_json += code;
                echo_json += "}";
                mg_printf(nc,"HTTP/1.1 200 OK\r\n");
                mg_printf(nc,echo_json.data());
            }
            else
            {
                mg_serve_http(nc,hm,s_http_server_opts);
            }
            nc->flags |= MG_F_SEND_AND_CLOSE;//响应完毕，完毕链接
        }
        static void LoginHandler(mg_connection *nc,int ev,void *data)
        {
            if(ev == MG_EV_CLOSE)
            {
                return;       
            }
            string code = "0";
            string echo_json ="{\"result\": ";
            string shead = "";
            struct http_message *hm = (struct http_message*)data;

            cout << "loginHandler ev: " << ev << endl;
            mg_printf(nc,"HTTP/1.1 200 OK\r\n");
            string method = Util::mgStrToString(&(hm->method));
            if(method == "POST")
            {
                string body = Util::mgStrToString(&hm->body);
                cout << "login handler: " << body << endl;
                string name,passwd;
                if(Util::GetNameAndPasswd(body,name,passwd) && !name.empty() && !passwd.empty())
                {
                    if(mc.SelectUser(name,passwd))
                    {
                        uint64_t id = 0;
                        if(sn.CreateSession(name,id))
                        {
                            stringstream ss;
                            ss << "Set-Cookie: " << SESSION_ID << "=" << id << "; path=/\r\n";
                            ss << "Set-Cookie: " << SESSION_NAME << "=" << name << "; path=/\r\n";
                            shead = ss.str();
                            mg_printf(nc,shead.data());
                            code = "0";
                        }
                        else
                        {
                            code = "3";
                        }
                    }
                    else
                    {
                        code = "1";
                    }
                }
                else
                {
                    code = "2";
                }
                echo_json += code;
                echo_json += "}";
                mg_printf(nc,"Content-Length: %lu\r\n\r\n",echo_json.size());
                mg_printf(nc,echo_json.data());
            }
            else
            {
                mg_serve_http(nc,hm,s_http_server_opts);
            }
            nc->flags |= MG_F_SEND_AND_CLOSE;//相应完毕，完毕链接
        }
        static void EventHandler(mg_connection *nc,int ev,void *data)
        {
            switch(ev)
            {
                case MG_EV_HTTP_REQUEST:
                    {
                        struct http_message *hm = (struct http_message*)data;
                        string uri = Util::mgStrToString(&hm->uri);
                        cout << "debug: "  << uri << endl;
                        if(uri.empty() || uri == "/" || uri == "index.html")
                        {
                            if(sn.IsLogin(hm))
                            {
                                mg_serve_http(nc,hm,s_http_server_opts);
                            }
                            else
                            {
                                mg_http_send_redirect(nc,302,mg_mk_str("/login.html"),mg_mk_str(NULL));
                            }
                        }
                        else
                        {
                            mg_serve_http(nc,hm,s_http_server_opts);
                        }
                        nc->flags |= MG_F_SEND_AND_CLOSE;
                    }
                     break;
                case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
                     {
                        Broadcast(nc,"somebody join...");
                     }
                     break;
                case MG_EV_WEBSOCKET_FRAME:
                     {
                        struct websocket_message *wm = (struct websocket_message*)data;
                        struct mg_str ms = {(const char*)wm->data,wm->size};
                        string msg = Util::mgStrToString(&ms);
                        Broadcast(nc,msg);
                     }
                     break;
                case MG_EV_CLOSE:
                     cout << "close link" << endl;
                     break;
                case MG_EV_TIMER:
                     sn.CheckSession();
                     mg_set_timer(nc,mg_time()+SESSION_CHECK_INTERVAL);

                default:
                     cout << "other ev: " << ev << endl;
                     break;
            }
        }
        void InitServer()
        {
            signal(SIGPIPE,SIG_IGN);
            mg_mgr_init(&mgr,NULL);
            nc = mg_bind(&mgr,port.c_str(),EventHandler);

            mg_register_http_endpoint(nc,"/LH",LoginHandler);
            mg_register_http_endpoint(nc,"/RH",RegisterHandler);

            mg_set_protocol_http_websocket(nc);
            s_http_server_opts.document_root = "web";

            mg_set_timer(nc,mg_time()+SESSION_CHECK_INTERVAL);
        }
        void Start()
        {
            int timeout = 10000;
            while(!quit)
            {
                mg_mgr_poll(&mgr,timeout);
                cout << "time out ..." << endl;
            }
        }
        ~ImServer()
        {
            mg_mgr_free(&mgr);
        }      
};
MysqlClient ImServer::mc;
Session ImServer::sn;
