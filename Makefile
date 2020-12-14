CPP = g++
CPPFLAGS = -Wall -o
OPTIONS = -lpthread

all: server

server : myqueue.o server.o
	$(CPP) -o server myqueue.o server.o $(OPTIONS)

myqueue.o : myqueue.h myqueue.cpp
	$(CPP) -c -o myqueue.o myqueue.cpp

server.o : server.cpp myqueue.h
	$(CPP) -c -o server.o server.cpp $(OPTIONS)

client : 
	$(CPP) $(CPPFLAGS) client client.cpp $(OPTIONS)