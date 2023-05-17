server:
	gcc server.c -o srv

clean:
	rm srv

build: server
