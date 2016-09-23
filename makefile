socket: server client

server: udp_server.c
	gcc udp_server.c -o server

client: udp_client.c
	gcc udp_client.c -o client

clean:
	-rm -f server
	-rm -f client
