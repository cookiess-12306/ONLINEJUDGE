oj_server:oj_server.cc
	g++ -o $@ $^ -std=c++11 -L/usr/local/lib -L/usr/lib64/mysql -lctemplate  -lpthread  -ljsoncpp  -lmysqlclient
.PHONY:clean
clean:
	rm -f oj_server
