.PHONY:all
all:
	@cd ./compile_server;\
	make;\
	cd -;\
	cd ./oj_server;\
	make;\
	cd -;

.PHONY:clean
clean:
	@cd ./compile_server;\
	make clean;\
	cd -;\
	cd ./oj_server;\
	make clean;\
	cd -;