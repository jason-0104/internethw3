all: tcp_client tcp_server
tcp_client:tcp_client.c 
	gcc -g tcp_client.c info.h -o tcp_client
tcp_server:tcp_server.c
	gcc -g tcp_server.c info.h -o tcp_server

clean: 
	rm -f *.o tcp_server tcp_client

