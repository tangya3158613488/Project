#include "ImServer.hpp"

int main(int argc,char *argv[])
{
    //MysqlClient *mc = new MysqlClient();
    //mc->ConnectMysql();
    //mc->InsertUser("赵六","123");
    //delete mc;

    ImServer *im = new ImServer();
    im->InitServer();
    im->Start();
    return 0;
}
