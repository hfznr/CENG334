all: writeOutput.o the.o
	g++ -c the.cpp
	g++ -c writeOutput.c
	g++ -o simulator the.o writeOutput.o -lpthread
