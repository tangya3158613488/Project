#pragma once

#include <iostream>
#include <string>
#include "mongoose.h"
#include <json/json.h>
#include <memory>
using namespace std;

class Util{
public:
        static string mgStrToString(struct mg_str* ms)
        {
            string str = "";
            for(unsigned int i = 0;i < ms->len;i++)
            {
                str.push_back(ms->p[i]);
            }
            return str;
        }
        static bool GetNameAndPasswd(string info,string &name,string &passwd)
        {
            bool result;
            JSONCPP_STRING errs;
            Json::Value root;
            Json::CharReaderBuilder cb;
            std::unique_ptr<Json::CharReader> const cr(cb.newCharReader());
            result = cr->parse(info.data(),info.data()+info.size(),&root,&errs);
            if(!result || !errs.empty())
            {
                cerr << "parse error" << endl;
                return false;
            }
            name = root["name"].asString();
            passwd = root["passwd"].asString();
            return true;
        }
};
