client:
	gcc client.c -o cli

server:
	gcc server.c -o srv

clean:
	rm cli srv
build: client server
