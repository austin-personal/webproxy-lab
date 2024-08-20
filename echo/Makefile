# 타겟 실행 파일의 이름과 오브젝트 파일 목록을 정의합니다.
SERVER_TARGET = echo-server
CLIENT_TARGET = echo-client
OBJS_SERVER = echo-server.o ../csapp.o
OBJS_CLIENT = echo-client.o ../csapp.o

# 모든 .o 파일을 생성하는 규칙입니다.
%.o: %.c
	gcc -c $<

# echo-server 실행 파일을 빌드하는 규칙입니다.
$(SERVER_TARGET): $(OBJS_SERVER)
	gcc -o $(SERVER_TARGET) $(OBJS_SERVER)

# echo-client 실행 파일을 빌드하는 규칙입니다.
$(CLIENT_TARGET): $(OBJS_CLIENT)
	gcc -o $(CLIENT_TARGET) $(OBJS_CLIENT)

# clean 타겟은 빌드 아티팩트를 정리합니다.
clean:
	rm -f *.o
	rm -f $(SERVER_TARGET)
	rm -f $(CLIENT_TARGET)
