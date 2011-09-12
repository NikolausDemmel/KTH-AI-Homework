all: build

build: client

client: *.h *.cc
	g++-mp-4.5 -o client *.cc

run: build
	./client 130.237.218.85 5556

clean:
	rm client