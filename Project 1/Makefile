CXX=gcc
CXXFLAGS= -g -Wall
USERID= 304937942

default: server

server: 
	$(CXX) -o webserver $^ $(CXXFLAGS) webserver.c

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM webserver *.tar.gz

dist: clean
	tar -cvzf $(USERID).tar.gz webserver.c Makefile report.pdf README.md

