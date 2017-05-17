CFLAG= -std=c++14 -W -Wall
all:server client

server:chatserverTCP.o
	g++  --std=c++14 chatserverTCP.o -lpthread -o server

chatserverTCP.o:chatserverTCP.cpp
	g++ ${CFLAG} -c chatserverTCP.cpp 

client:chatclientTCP.o
	g++  ${CFLAG} chatclientTCP.o -lpthread -o client

chatclientTCP.o:chatclientTCP.cpp
	g++ ${CFLAG} -c chatclientTCP.cpp 

clean:
	rm -rf *o server.* client.*
