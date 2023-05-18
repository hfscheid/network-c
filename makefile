network:
	gcc network.c -o nw

clean:
	rm nw 

main:
	gcc network.o main.c -o run

build: network

test: build
	./nw 8080 8081
