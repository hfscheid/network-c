network:
	gcc -c network.c -o network.o

clean:
	rm nw 

build:
	gcc server.c -o server

test: build
	./server 8080 8081
