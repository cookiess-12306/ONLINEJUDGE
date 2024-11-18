oj_server:oj_server.cc
	g++ -o $@ $^ -std=c++11 -I/usr/local/include/ctemplate -I/usr/local/include/json -L/usr/local/lib64  -L/usr/local/lib -lpthread -lctemplate -ljsoncpp
.PHONY:clean
clean:
	rm -f oj_server
