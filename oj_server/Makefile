oj_server:oj_server.cc
	g++ -o $@ $^ -std=c++11 -I/home/xyt/ONLINEJUDGE/third_part/ctemplate/mysql_connector/include/jdbc \
	-L/usr/local/lib -L/usr/lib64/mysql \
	-Wl,-rpath,/home/xyt/ONLINEJUDGE/third_part/ctemplate/mysql_connector/lib64 \
	-lctemplate  -lpthread  -ljsoncpp  -lmysqlclient -lmysqlcppconn
.PHONY:clean
clean:
	rm -f oj_server
