CC=g++
INCLUDE= -Ispeech/
LIB= -ljsoncpp -lcurl -lcrypto -lpthread
bin=Sakura
src=Sakura.cc
FLAGS=-D_LOG_

$(bin):$(src)
	$(CC)  -o $@ $^ $(FLAGS) $(INCLUDE) $(LIB) -std=c++11 
.PHONY:clean
clean:
	rm -f $(bin)
