clean:
	rm server send

send:
	gcc send_message.c -o send

server:
	gcc server.c -o server

build: send server

rebuild: clean send server

test: build
	./server 8080 8081
