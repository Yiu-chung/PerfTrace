all: udp_owamp_client udp_owamp_server

udp_owamp_client: udp_owamp_client.c
	gcc udp_owamp_client.c -o udp_owamp_client -O2 -lpthread -lm 
udp_owamp_server: udp_owamp_server.c
	gcc udp_owamp_server.c -o udp_owamp_server  -O2 -lpthread -lm 

.PHONY : clean
clean:
	rm -f  udp_owamp_client udp_owamp_server
