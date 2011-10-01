
SERVER=130.237.218.85
PORT=12321
MODE=STANDALONE
OPTIONS=practice

all: build

build: client

client: *.cc *.h
	g++-mp-4.5 --std=gnu++0x $(CXXFLAGS) -o client *.cc

run: build
	./client $(SERVER) $(PORT) $(MODE) $(OPTIONS)

build-opt: client-opt

client-opt: *.cc *.h
	g++-mp-4.5 --std=gnu++0x $(CXXFLAGS) -O3 -o client-opt *.cc

run-opt: client-opt
	./client-opt $(SERVER) $(PORT) $(MODE) $(OPTIONS)

clean:
	rm -f client
	rm -f client-opt

#zip: demmel_cpp_hw2.zip

#demmel_cpp_hw2.zip: email.txt rulecode.txt questions.txt *.h *.cc
#	zip demmel_cpp_hw2.zip email.txt rulecode.txt questions.txt *.h *.cc

#runtest: test
#	./test	

#test: test.cpp
#	g++-mp-4.5 -o test test.cpp