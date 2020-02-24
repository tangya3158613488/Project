cc=g++
bin=ImServer
src=ImServer.cc mongoose/mongoose.c
include=-Imongoose/ -Imysql/include
lib=-Lmysql/lib
lib_name=-lmysqlclient -ljsoncpp 

$(bin):$(src)
	$(cc) -Wall -o $@ $^ -std=c++11 $(include) $(lib) $(lib_name) -g
.PHONY:clean
clean:
	rm -f $(bin)

