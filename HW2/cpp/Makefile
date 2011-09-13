all: build

build: client

client: *.h *.cc
	g++-mp-4.5 -o client *.cc

run: build
	./client 130.237.218.85 5556

clean:
	rm client

zip: demmel_cpp_hw2.zip
	
demmel_cpp_hw2.zip: email.txt rulecode.txt questions.txt *.h *.cc
	zip demmel_cpp_hw2.zip email.txt rulecode.txt questions.txt *.h *.cc