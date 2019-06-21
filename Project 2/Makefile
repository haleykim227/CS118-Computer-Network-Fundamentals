CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=904826309
CLASSES=

all: server client

SERVER_SRC = server.cpp servHelper.cpp header.cpp
SERVER_OBJ = $(subst .cpp,.o,$(SERVER_SRC))

CLIENT_SRC = client.cpp header.cpp
CLIENT_OBJ = $(subst .cpp,.o,$(CLIENT_SRC))

server: $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(SERVER_OBJ)

client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(CLIENT_OBJ)

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .
