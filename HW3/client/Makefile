
SERVER=130.237.218.85
PORT=12321
MODE=STANDALONE
OPTIONS= #numbirds=180 numspecies=5 numturns=500 timeperturn=1

all: build

build: client

client: *.cc *.h
	g++-mp-4.5 --std=gnu++0x $(CXXFLAGS) -g -o client *.cc

run: build
	./client $(SERVER) $(PORT) $(MODE) $(OPTIONS)

run-practice: build
	./client $(SERVER) $(PORT) $(MODE) practice

test-practice:
	for i in {1..100} ; do echo $$i 1>&2 && $(MAKE) run-practice | grep "good guess"; done | wc -l

build-opt: client-opt

client-opt: *.cc *.h
	g++-mp-4.5 --std=gnu++0x $(CXXFLAGS) -O3 -o client-opt *.cc

run-opt: client-opt
	./client-opt $(SERVER) $(PORT) $(MODE) $(OPTIONS)

run-practice-opt: client-opt
	./client-opt $(SERVER) $(PORT) $(MODE) practice

test-opt:
	for i in {1..100} ; do echo $$i 1>&2 && $(MAKE) run-opt | tee -a test-games.log | grep "Score" ; done

test-practice-opt:
	for i in {1..100} ; do echo $$i 1>&2 && $(MAKE) run-practice-opt | grep "wrong" ; done  | wc -l 

clean:
	rm -f client
	rm -f client-opt

zip: demmel_cpp_hw3.zip

demmel_cpp_hw3.zip: *.h *.cc
	zip demmel_cpp_hw3.zip *.h *.cc

plot2:
	./plot.sh "./output.csv" "1:2"

plot3:
	./plot.sh "./output.csv" "1:3"

plot-mat:
	./plot-matrix.sh "./distances.csv"

#runtest: test1
#	./test	

#test: test.cpp
#	g++-mp-4.5 -o test test.cpp