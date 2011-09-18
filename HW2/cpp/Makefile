all: build

build: client

client: *.h *.cc
	g++-mp-4.5 -o client *.cc

run: build
	./client 130.237.218.85 5559

clean:
	rm client

zip: demmel_cpp_hw2.zip
	
demmel_cpp_hw2.zip: email.txt rulecode.txt questions.txt *.h *.cc
	zip demmel_cpp_hw2.zip email.txt rulecode.txt questions.txt *.h *.cc
	
runtest: test
	./test
	
test: test.cpp
	g++-mp-4.5 -o test test.cpp